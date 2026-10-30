# Contributing Guidelines

Thank you for your interest in contributing to the **Arduino 2WD Line Follower Robot** project!

## How to Contribute

1. **Fork the Repository**:
   Click the "Fork" button at the top right of this page.

2. **Clone your Fork**:
   ```bash
   git clone https://github.com/<your-username>/Line-Follower-Robot.git
   cd Line-Follower-Robot
   ```

3. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/pid-control-enhancement
   ```

4. **Code Quality Guidelines**:
   - Keep pin configurations modular at the top of the sketch.
   - Comment critical timing delays and PWM calculations.
   - Test sketch compilation for standard **Arduino Uno** boards before submitting.

5. **Submit a Pull Request**:
   - Provide a clear summary of your changes.
   - Mention any hardware differences tested (e.g., L298N vs L293D, 5-channel sensor array).
