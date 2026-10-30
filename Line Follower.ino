/* =========================================================================================
 * Project: Arduino 2WD Line Follower Robot
 * Microcontroller: Arduino Uno (ATmega328P)
 * Motor Driver: Dual H-Bridge L293D
 * Sensors: 2x IR Obstacle / Line Tracking Modules (LM393)
 * Motors: 2x Geared TT BO Motors (Yellow Toy Motors)
 * Power Supply: 13.2V Battery Pack (with PWM voltage/speed limiting)
 * 
 * Description:
 * Autonomous differential-drive line tracking robot using two infrared reflectance
 * sensors and an L293D dual H-bridge motor driver. Includes PWM speed regulation to
 * protect 3-6V BO motors against high battery pack voltages (13.2V) and prevent
 * line-overshoot. Features real-time state machine logic and serial diagnostics.
 * ========================================================================================= */

// =========================================================================================
// 1. PIN CONFIGURATION
// =========================================================================================

// IR Sensor Pins (Default: Analog pins used as digital inputs to preserve PWM pins)
const uint8_t LEFT_SENSOR_PIN  = A0;   // Left IR Sensor Digital Output (OUT)
const uint8_t RIGHT_SENSOR_PIN = A1;   // Right IR Sensor Digital Output (OUT)

// Left Motor (Motor A) Driver Pins
const uint8_t LM_IN1 = 5;              // Arduino D5  -> L293D IN1 (Direction / PWM)
const uint8_t LM_IN2 = 6;              // Arduino D6  -> L293D IN2 (Direction / PWM)
const uint8_t LM_ENA = 9;              // Arduino D9  -> L293D ENA (Left Speed PWM)

// Right Motor (Motor B) Driver Pins
const uint8_t RM_IN3 = 10;             // Arduino D10 -> L293D IN3 (Direction / PWM)
const uint8_t RM_IN4 = 11;             // Arduino D11 -> L293D IN4 (Direction / PWM)
const uint8_t RM_ENB = 3;              // Arduino D3  -> L293D ENB (Right Speed PWM)

// =========================================================================================
// 2. HARDWARE & OPERATING MODES
// =========================================================================================

/**
 * MOTOR_DRIVER_HAS_EN_PINS:
 * - Set to TRUE if using dedicated ENA and ENB pins connected to Arduino PWM pins (9 & 3).
 * - Set to FALSE if ENA/ENB jumpers are permanently hardwired to 5V on the L293D board.
 *   In FALSE mode, PWM speed control is applied directly to IN1-IN4.
 */
#ifndef MOTOR_DRIVER_HAS_EN_PINS
#define MOTOR_DRIVER_HAS_EN_PINS true
#endif

/**
 * SENSOR LOGIC CONFIGURATION:
 * Most LM393 IR modules output:
 * - LOW (0) when detecting a reflective white surface (IR reflected).
 * - HIGH (1) when detecting a non-reflective black line (IR absorbed).
 * If your sensor outputs the opposite, toggle this value to LOW.
 */
#ifndef BLACK_LINE_LOGIC
#define BLACK_LINE_LOGIC HIGH
#endif
#define WHITE_SURFACE_LOGIC (!BLACK_LINE_LOGIC)

/**
 * SERIAL DEBUGGING:
 * Set to true to view real-time sensor readings and motor decisions in Serial Monitor.
 * Set to false during competition or standalone runs for maximum loop execution speed.
 */
#define DEBUG_MODE true
#define SERIAL_BAUD 9600

// =========================================================================================
// 3. SPEED & VOLTAGE MANAGEMENT (CRITICAL FOR 13.2V BATTERY)
// =========================================================================================
/**
 * ⚠️ IMPORTANT NOTE REGARDING 13.2V BATTERY:
 * Yellow BO motors are rated for 3V to 6V DC (absolute max ~9V).
 * The L293D incurs an internal transistor drop of approximately 1.5V - 2.0V.
 * To keep the effective motor voltage within the safe 4.5V - 6.0V range and prevent
 * violent overspeeding or stripping plastic gears:
 * - PWM values are scaled (0 - 255).
 * - Target duty cycle: ~45% - 60% (PWM 110 - 150).
 */
const uint8_t BASE_SPEED   = 130;      // Cruising speed on straight track (0-255)
const uint8_t TURN_SPEED   = 150;      // Speed of outer wheel when correcting a drift
const uint8_t PIVOT_SPEED  = 120;      // Speed during tight differential pivot turns
const uint8_t MIN_SPEED    = 80;       // Minimum PWM duty cycle to overcome motor stall

// Track last known turn direction for edge recovery (0: Straight, 1: Left, 2: Right)
uint8_t lastKnownTurn = 0;

// Telemetry timer to prevent serial flooding
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL_MS = 150;

// =========================================================================================
// 4. FUNCTION DECLARATIONS
// =========================================================================================
void setMotorSpeed(uint8_t leftSpeed, uint8_t rightSpeed);
void moveForward(uint8_t speed);
void smoothTurnLeft(uint8_t speed);
void smoothTurnRight(uint8_t speed);
void pivotLeft(uint8_t speed);
void pivotRight(uint8_t speed);
void stopMotors();
void runDiagnostics();

// =========================================================================================
// 5. SETUP
// =========================================================================================
void setup() {
  // Initialize IR Sensor pins
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);

  // Initialize Motor Driver Control pins
  pinMode(LM_IN1, OUTPUT);
  pinMode(LM_IN2, OUTPUT);
  pinMode(RM_IN3, OUTPUT);
  pinMode(RM_IN4, OUTPUT);

#if MOTOR_DRIVER_HAS_EN_PINS
  pinMode(LM_ENA, OUTPUT);
  pinMode(RM_ENB, OUTPUT);
#endif

  // Ensure motors start safely in OFF state
  stopMotors();

  if (DEBUG_MODE) {
    Serial.begin(SERIAL_BAUD);
    Serial.println(F("========================================"));
    Serial.println(F("  ARDUINO 2WD LINE FOLLOWER ROBOT"));
    Serial.println(F("  Motor Driver: L293D | Battery: 13.2V"));
    Serial.println(F("========================================"));
    Serial.println(F("[SYSTEM] Running motor startup check..."));
    runDiagnostics();
    Serial.println(F("[SYSTEM] Ready! Place robot on track."));
  }

  // Brief stabilization delay before autonomous navigation begins
  delay(1000);
}

// =========================================================================================
// 6. MAIN CONTROL LOOP
// =========================================================================================
void loop() {
  // Read sensor inputs (Active HIGH for Black Line by default)
  int leftVal  = digitalRead(LEFT_SENSOR_PIN);
  int rightVal = digitalRead(RIGHT_SENSOR_PIN);

  bool leftOnLine  = (leftVal == BLACK_LINE_LOGIC);
  bool rightOnLine = (rightVal == BLACK_LINE_LOGIC);

  /*
   * DECISION TRUTH TABLE:
   * ---------------------------------------------------------------------------------
   * Left Sensor | Right Sensor | Condition                    | Action Taken
   * ---------------------------------------------------------------------------------
   * 0 (White)   | 0 (White)    | Robot is centered on line    | Move Forward
   * 1 (Black)   | 0 (White)    | Robot drifted to the right   | Turn Left (realign)
   * 0 (White)   | 1 (Black)    | Robot drifted to the left    | Turn Right (realign)
   * 1 (Black)   | 1 (Black)    | Intersection / T-junction    | Custom (Move Forward / Stop)
   * ---------------------------------------------------------------------------------
   */

  if (!leftOnLine && !rightOnLine) {
    // Both sensors detect white: Line is centered between the two sensors
    moveForward(BASE_SPEED);
    lastKnownTurn = 0;

  } else if (leftOnLine && !rightOnLine) {
    // Left sensor triggered: Robot is deviating right -> Turn Left
    smoothTurnLeft(TURN_SPEED);
    lastKnownTurn = 1;

  } else if (!leftOnLine && rightOnLine) {
    // Right sensor triggered: Robot is deviating left -> Turn Right
    smoothTurnRight(TURN_SPEED);
    lastKnownTurn = 2;

  } else {
    // Both sensors detect black (T-junction, crossroad, or end of line)
    // Option A: Stop at end line
    // Option B: Continue forward to cross intersection
    moveForward(BASE_SPEED - 20); // Continue forward cautiously
  }

  // Periodic Serial telemetry
  if (DEBUG_MODE && (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL_MS)) {
    lastTelemetryTime = millis();
    Serial.print(F("Sensors [L: "));
    Serial.print(leftOnLine ? F("BLACK") : F("WHITE"));
    Serial.print(F(" | R: "));
    Serial.print(rightOnLine ? F("BLACK") : F("WHITE"));
    Serial.print(F("] -> Action: "));
    
    if (!leftOnLine && !rightOnLine)      Serial.println(F("FORWARD"));
    else if (leftOnLine && !rightOnLine)  Serial.println(F("CORRECT LEFT"));
    else if (!leftOnLine && rightOnLine)  Serial.println(F("CORRECT RIGHT"));
    else                                  Serial.println(F("INTERSECTION"));
  }
}

// =========================================================================================
// 7. MOTOR CONTROL PRIMITIVES
// =========================================================================================

/**
 * Drive both motors forward at specified speed.
 */
void moveForward(uint8_t speed) {
#if MOTOR_DRIVER_HAS_EN_PINS
  digitalWrite(LM_IN1, HIGH);
  digitalWrite(LM_IN2, LOW);
  digitalWrite(RM_IN3, HIGH);
  digitalWrite(RM_IN4, LOW);
  analogWrite(LM_ENA, speed);
  analogWrite(RM_ENB, speed);
#else
  analogWrite(LM_IN1, speed);
  digitalWrite(LM_IN2, LOW);
  analogWrite(RM_IN3, speed);
  digitalWrite(RM_IN4, LOW);
#endif
}

/**
 * Smooth Left Correction:
 * Slow down or idle the left inner wheel while running the right outer wheel forward.
 */
void smoothTurnLeft(uint8_t speed) {
#if MOTOR_DRIVER_HAS_EN_PINS
  digitalWrite(LM_IN1, LOW);
  digitalWrite(LM_IN2, LOW);
  digitalWrite(RM_IN3, HIGH);
  digitalWrite(RM_IN4, LOW);
  analogWrite(LM_ENA, 0);
  analogWrite(RM_ENB, speed);
#else
  digitalWrite(LM_IN1, LOW);
  digitalWrite(LM_IN2, LOW);
  analogWrite(RM_IN3, speed);
  digitalWrite(RM_IN4, LOW);
#endif
}

/**
 * Smooth Right Correction:
 * Slow down or idle the right inner wheel while running the left outer wheel forward.
 */
void smoothTurnRight(uint8_t speed) {
#if MOTOR_DRIVER_HAS_EN_PINS
  digitalWrite(LM_IN1, HIGH);
  digitalWrite(LM_IN2, LOW);
  digitalWrite(RM_IN3, LOW);
  digitalWrite(RM_IN4, LOW);
  analogWrite(LM_ENA, speed);
  analogWrite(RM_ENB, 0);
#else
  analogWrite(LM_IN1, speed);
  digitalWrite(LM_IN2, LOW);
  digitalWrite(RM_IN3, LOW);
  digitalWrite(RM_IN4, LOW);
#endif
}

/**
 * Sharp Pivot Left:
 * Left wheel spins backward, Right wheel spins forward (zero-radius spin).
 */
void pivotLeft(uint8_t speed) {
#if MOTOR_DRIVER_HAS_EN_PINS
  digitalWrite(LM_IN1, LOW);
  digitalWrite(LM_IN2, HIGH);
  digitalWrite(RM_IN3, HIGH);
  digitalWrite(RM_IN4, LOW);
  analogWrite(LM_ENA, speed);
  analogWrite(RM_ENB, speed);
#else
  digitalWrite(LM_IN1, LOW);
  analogWrite(LM_IN2, speed);
  analogWrite(RM_IN3, speed);
  digitalWrite(RM_IN4, LOW);
#endif
}

/**
 * Sharp Pivot Right:
 * Left wheel spins forward, Right wheel spins backward (zero-radius spin).
 */
void pivotRight(uint8_t speed) {
#if MOTOR_DRIVER_HAS_EN_PINS
  digitalWrite(LM_IN1, HIGH);
  digitalWrite(LM_IN2, LOW);
  digitalWrite(RM_IN3, LOW);
  digitalWrite(RM_IN4, HIGH);
  analogWrite(LM_ENA, speed);
  analogWrite(RM_ENB, speed);
#else
  analogWrite(LM_IN1, speed);
  digitalWrite(LM_IN2, LOW);
  digitalWrite(RM_IN3, LOW);
  analogWrite(RM_IN4, speed);
#endif
}

/**
 * Emergency Stop / Brake:
 * Both motors powered down.
 */
void stopMotors() {
  digitalWrite(LM_IN1, LOW);
  digitalWrite(LM_IN2, LOW);
  digitalWrite(RM_IN3, LOW);
  digitalWrite(RM_IN4, LOW);

#if MOTOR_DRIVER_HAS_EN_PINS
  analogWrite(LM_ENA, 0);
  analogWrite(RM_ENB, 0);
#endif
}

/**
 * Motor Diagnostics Test (executed briefly on startup in DEBUG_MODE).
 * Verifies motor wiring and direction.
 */
void runDiagnostics() {
  Serial.println(F("[TEST] Left wheel pulse..."));
  smoothTurnRight(120); // Left wheel only
  delay(250);
  stopMotors();
  delay(150);

  Serial.println(F("[TEST] Right wheel pulse..."));
  smoothTurnLeft(120);  // Right wheel only
  delay(250);
  stopMotors();
  delay(150);
  Serial.println(F("[TEST] Diagnostic complete."));
}
