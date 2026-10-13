# IMU Visualizer
This program collects IMU motion data (angular velocity from gyroscope, acceleration from accelerometer) and renders 3D UAV model with estimated orientation. 
### Keys 
- `R` reset UAV orientation
- `F` enter free camera mode (use `WASD` to move camera). Press `ESC` to exit free camera mode. 
- `Z` center camera at the UAV.
# Demo 

https://github.com/user-attachments/assets/feef701d-0188-420d-a5e2-cf75e32a0a3f

## Hardware used 
- STM32 Nucleo-G474RE development board which transmits IMU motion data to the host PC via USB.
- GY-521 IMU Module (which uses MPU-6050 gyroscope).

See [STM32 Nucleo-G474RE firmware for this project.](./stm32-imu/)

# Resources
This project uses:
- WW1 Plane Model by GiaHanLam (taken from raylib [example](https://www.raylib.com/examples/models/loader.html?name=models_yaw_pitch_roll)).
- [Inter Font](https://rsms.me/inter/). 
