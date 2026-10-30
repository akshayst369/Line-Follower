# Line Follower Calibration & Tuning Guide

Achieving stable, high-speed line tracking requires proper mechanical alignment, electrical calibration of the IR sensors, and firmware speed tuning. Follow this guide to optimize performance.

---

## 1. IR Sensor Mechanical Alignment

The physical placement of the IR sensors directly impacts tracking accuracy:

1. **Sensor Ground Clearance**:
   - Optimal distance between the IR LEDs/photodiodes and the track surface is **5 mm to 10 mm** (roughly the thickness of a pencil).
   - If mounted too high (>15 mm), ambient light will saturate the sensor and reduce contrast.
   - If mounted too low (<3 mm), the sensor will scrape the paper or tape seams.

2. **Sensor Spacing (Width)**:
   - For a standard **18 mm to 20 mm wide black electrical tape line**, the ideal spacing between the two sensor centers is **25 mm to 30 mm**.
   - **Target Behavior**: Both sensors should straddle the black line, sitting over the white surface while moving straight.
   - When the robot veers right, the left sensor moves over the black line, triggering a left turn correction.

---

## 2. Sensor Potentiometer Electrical Tuning

Each LM393 sensor module contains a multi-turn blue trimmer potentiometer and two LEDs:
- **Power LED** (Red): Illuminates whenever 5V power is supplied.
- **Signal / Indicator LED** (Red or Green): Illuminates when the surface triggers the comparator threshold.

### Step-by-Step Sensor Adjustment:

1. Power on the robot with the sensors placed over your **white surface**.
2. Rotate the potentiometer using a small screwdriver until the Signal LED turns **OFF** (or turns ON, depending on active logic).
3. Move the sensor directly over the **black electrical tape**.
4. The Signal LED should change state immediately (turn **ON** over black, turn **OFF** over white).
5. Slide the robot back and forth between white and black several times. There should be a sharp, clear transition right at the line boundary with zero flickering.
6. Repeat the exact same calibration procedure for the second sensor.

---

## 3. Firmware Speed & PWM Tuning

Open `LineFollower/LineFollower.ino` and locate Section 3:

```cpp
const uint8_t BASE_SPEED   = 130;  // Straight-line cruise speed (0-255)
const uint8_t TURN_SPEED   = 150;  // Corrective steering speed (0-255)
const uint8_t PIVOT_SPEED  = 120;  // Pivot turn speed (0-255)
```

### Tuning Strategy:

| Symptom | Cause | Solution |
| :--- | :--- | :--- |
| **Robot oscillates violently back and forth ("fish-tailing")** | `TURN_SPEED` is too high or `BASE_SPEED` is too close to `TURN_SPEED` | Decrease `TURN_SPEED` by 15-20 points, or widen the sensor distance. |
| **Robot overshoots sharp 90° corners** | `BASE_SPEED` is too high for the sensor response time | Decrease `BASE_SPEED` to 110-120 and increase `TURN_SPEED` to 160. |
| **Motors hum/buzz but don't move** | PWM duty cycle is below the motor's minimum stall torque | Increase `BASE_SPEED` to at least 110 (minimum stall overcoming speed). |
| **Robot drifts to one side when moving forward** | Slight manufacturing variance in BO motor gear friction | Add a slight speed offset to the faster motor in `moveForward()`. |

---

## 4. Track Surface Best Practices

- **Tape**: Standard non-reflective matte black vinyl electrical tape (18 mm to 24 mm width).
- **Surface**: White poster board, flex banner, or foam board with a matte (non-glossy) finish.
- **Lighting**: Avoid placing track under direct sunlight or harsh incandescent spotlights, as strong infrared light can overwhelm IR receivers.
