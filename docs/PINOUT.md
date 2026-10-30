# Hardware Pinout & Wiring Reference

This document provides a comprehensive connection table and electrical schematic reference for the **Arduino 2WD Line Follower Robot**.

---

## 1. Electrical Architecture Overview

```
                          +------------------------+
                          |   13.2V Battery Pack   |
                          | (3x 18650 Li-ion Cells)|
                          +-----------+------------+
                                      |
                                [SPST Switch]
                                      |
                     +----------------+----------------+
                     | (+13.2V)                        | (+13.2V)
                     v                                 v
            +-----------------+               +-----------------+
            |   Arduino VIN   |               |  L293D VCC2/12V |
            | (Internal Reg.) |               | (Motor Power)   |
            +--------+--------+               +--------+--------+
                     | (Regulated 5V)                  |
                     +----------------> L293D VCC1/5V  | (Dual H-Bridge)
                     |                  (Logic Supply) |
                     v                                 |
              [ 2x IR Sensors ]                        +--------+--------+
                  (VCC)                                |                 |
                                                       v                 v
                                                 [Left Motor]      [Right Motor]
```

> [!IMPORTANT]
> **Common Ground (GND)**: All GND lines—Battery (-), Arduino GND, L293D GND, and IR Sensor GND—**must be bonded together**. Without a shared reference ground, logic signals cannot be interpreted correctly and motor behavior will be erratic.

---

## 2. Pin Connection Tables

### A. Arduino Uno to L293D Motor Driver

| Arduino Uno Pin | Pin Type | L293D Pin / Terminal | Function | Description |
| :--- | :--- | :--- | :--- | :--- |
| **D5** | Digital (PWM) | **IN1** | Left Motor Dir 1 | Motor A Direction Control |
| **D6** | Digital (PWM) | **IN2** | Left Motor Dir 2 | Motor A Direction Control |
| **D9** | Digital (PWM) | **ENA** | Left Motor Speed | PWM Speed Modulation (Motor A) |
| **D10** | Digital (PWM) | **IN3** | Right Motor Dir 1 | Motor B Direction Control |
| **D11** | Digital (PWM) | **IN4** | Right Motor Dir 2 | Motor B Direction Control |
| **D3** | Digital (PWM) | **ENB** | Right Motor Speed | PWM Speed Modulation (Motor B) |
| **5V** | Power | **VCC1 / 5V** | Logic Power | Powers internal L293D logic gates |
| **GND** | Power | **GND** | Common Ground | Shared system reference |

*Note: If your L293D board has onboard jumpers on ENA and ENB connecting them directly to 5V, you can either remove the jumpers and connect pins D9 and D3, or set `#define MOTOR_DRIVER_HAS_EN_PINS false` in the sketch to run direct PWM on IN1-IN4.*

---

### B. Arduino Uno to IR Line Sensors (LM393)

| IR Sensor Module | Module Pin | Arduino Uno Pin | Wire Color (Ref Photo) | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Left Sensor** | **VCC** | **5V** | Yellow (Bridged) | 5V Regulated Sensor Power |
| **Left Sensor** | **GND** | **GND** | Blue (Bridged) | Ground Return |
| **Left Sensor** | **OUT / D0** | **A0** | Green / Grey | Digital Reflectance Output |
| **Right Sensor** | **VCC** | **5V** | Yellow (Bridged) | 5V Regulated Sensor Power |
| **Right Sensor** | **GND** | **GND** | Blue (Bridged) | Ground Return |
| **Right Sensor** | **OUT / D0** | **A1** | Green / White | Digital Reflectance Output |

---

### C. Power & Motor Connections

| Source | Output Pin | Destination | Purpose |
| :--- | :--- | :--- | :--- |
| **Battery (+) (13.2V)** | Lead Wire | **Rocker Switch (Pin 1)** | Master Power In |
| **Rocker Switch** | Switched Pin | **Arduino VIN** | Powers Arduino onboard 5V regulator |
| **Rocker Switch** | Switched Pin | **L293D VCC2 / 12V** | Powers H-bridge motor outputs |
| **Battery (-) (GND)** | Lead Wire | **L293D GND** & **Arduino GND** | Shared Common Ground Bus |
| **L293D OUT1, OUT2** | Screw Terminals | **Left BO Motor (TT)** | Drives Left Wheel |
| **L293D OUT3, OUT4** | Screw Terminals | **Right BO Motor (TT)** | Drives Right Wheel |

---

## 3. High-Voltage Protection Note

The 13.2V battery pack exceeds the standard 3V–6V rating of hobby BO gearmotors. The firmware controls average terminal voltage via high-speed Pulse Width Modulation (PWM):

$$\text{Effective } V_{\text{motor}} \approx (V_{\text{batt}} - V_{\text{drop}}) \times \left(\frac{\text{PWM}}{255}\right)$$

With $V_{\text{batt}} = 13.2\text{V}$, $V_{\text{drop}} \approx 1.8\text{V}$ (L293D bipolar Darlington drop), and $\text{PWM} = 130$:

$$\text{Effective } V_{\text{motor}} \approx (13.2 - 1.8) \times \left(\frac{130}{255}\right) \approx 11.4 \times 0.51 \approx 5.8\text{V}$$

This keeps motor wear minimal, avoids burning motor brushes, and ensures smooth non-oscillating line tracking.
