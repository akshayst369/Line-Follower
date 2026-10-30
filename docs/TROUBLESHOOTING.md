# Troubleshooting & FAQ

Use this troubleshooting matrix to quickly diagnose and fix common hardware and software issues with the Line Follower robot.

---

## 1. Diagnostic Matrix

### Issue 1: Motors spin in reverse (Robot moves backward)
- **Cause**: Motor wires are connected with inverted polarity to L293D terminal blocks `OUT1/OUT2` or `OUT3/OUT4`.
- **Fix (Hardware)**: Swap the two wires of the affected motor at the L293D terminal block.
- **Fix (Firmware)**: Alternatively, invert the logic pins in `LineFollower.ino`:
  ```cpp
  // Swap LM_IN1 and LM_IN2 assignments
  const uint8_t LM_IN1 = 6;
  const uint8_t LM_IN2 = 5;
  ```

---

### Issue 2: Robot turns away from the line instead of following it
- **Cause**: Left and Right sensor pins are reversed, or Left and Right motor driver channels are swapped.
- **Fix**:
  1. Verify pin assignments in `LineFollower.ino`:
     - Left Sensor -> Pin `A0`
     - Right Sensor -> Pin `A1`
  2. If the robot veers right when the left sensor touches black, swap `LEFT_SENSOR_PIN` and `RIGHT_SENSOR_PIN` in code:
     ```cpp
     const uint8_t LEFT_SENSOR_PIN  = A1;
     const uint8_t RIGHT_SENSOR_PIN = A0;
     ```

---

### Issue 3: Arduino resets or reboots when motors start moving
- **Cause**: Electrical noise / voltage sag (brownout reset). When DC motors start under load, they draw a sharp inrush current, causing battery voltage to momentarily dip below the Arduino onboard regulator's minimum dropout threshold (~6.5V).
- **Fix**:
  1. Ensure the 13.2V battery pack is fully charged (>11.5V under load).
  2. Solder a **0.1 µF (104) ceramic capacitor** directly across each BO motor's positive and negative terminals to suppress RF brush arcing noise.
  3. Place a **100 µF to 470 µF electrolytic capacitor** across the L293D power input (`VCC2` and `GND`) as a local reservoir.

---

### Issue 4: L293D chip gets very hot to the touch
- **Cause**: Continuous high current draw or driving motors at 100% duty cycle on 13.2V.
- **Fix**:
  1. The L293D has an internal thermal shutdown mechanism at ~150°C.
  2. Ensure firmware uses PWM duty cycle regulation (`BASE_SPEED` between 120 and 150).
  3. Attach an adhesive aluminum heatsink to the L293D IC or module.

---

### Issue 5: Sensors trigger randomly or fail to detect black line
- **Cause**: Ambient infrared interference (sunlight or halogen bulbs) or improper potentiometer threshold.
- **Fix**:
  1. Follow the procedure in [CALIBRATION.md](file:///Volumes/Drive_Ak/Akshay/E-Projects/Line%20Follower/docs/CALIBRATION.md).
  2. Shield the sensors from direct overhead light using a piece of black tape or cardboard shroud.
  3. If your sensors output `LOW` on black instead of `HIGH`, change `#define BLACK_LINE_LOGIC LOW` in the sketch.

---

## 2. Real-Time Serial Diagnostics

To view live telemetry from the robot while connected to a computer:

1. Connect the Arduino Uno to your PC via USB cable.
2. Keep the robot chassis lifted so the wheels do not run off your desk.
3. Open the **Arduino IDE Serial Monitor** at **9600 baud**.
4. Slide a piece of white paper and black tape under each sensor. You will see real-time output:
   ```text
   Sensors [L: WHITE | R: WHITE] -> Action: FORWARD
   Sensors [L: BLACK | R: WHITE] -> Action: CORRECT LEFT
   Sensors [L: WHITE | R: BLACK] -> Action: CORRECT RIGHT
   ```
