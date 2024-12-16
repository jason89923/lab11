#include <wiringPi.h>
#include <iostream>

using namespace std;

const int motor = 1;   // WiringPi 編號 1，對應 GPIO18
const int minPwm = 50; // 對應 1ms 脈寬（50）
const int maxPwm = 250; // 對應 2ms 脈寬（250）
const int maxAngle = 168; // 校準後的最大角度範圍

// 非線性校準數據點
const int calibrationPoints[5][2] = {
    {0, 0},   // 0° 對應 0 輸入
    {45, 30}, // 45° 實際需要輸入 42
    {90, 80}, // 90° 實際需要輸入 85
    {135, 120}, // 135° 實際需要輸入 126
    {180, 168}  // 180° 對應 168
};

// 線性插值計算
int interpolate(int angle) {
    for (int i = 0; i < 4; ++i) {
        if (angle >= calibrationPoints[i][0] && angle <= calibrationPoints[i + 1][0]) {
            int x1 = calibrationPoints[i][0];
            int x2 = calibrationPoints[i + 1][0];
            int y1 = calibrationPoints[i][1];
            int y2 = calibrationPoints[i + 1][1];
            // 線性插值公式
            return y1 + (angle - x1) * (y2 - y1) / (x2 - x1);
        }
    }
    return angle; // 如果角度超出範圍，返回原值（理論上不會發生）
}

void setServoAngle(int angle) {
    if (angle < 0 || angle > 180) {
        cout << "Invalid angle! Please enter a value between 0 and 180." << endl;
        return;
    }

    // 使用校準後的角度
    int calibratedAngle = interpolate(angle);

    // 將校準後的角度映射到 PWM 值
    int pwmValue = minPwm + (calibratedAngle * (maxPwm - minPwm) / 180);
    pwmWrite(motor, pwmValue);
    cout << "Servo angle set to " << angle << " degrees (Calibrated: " 
         << calibratedAngle << ", PWM: " << pwmValue << ")" << endl;
}

int main(void) {
    cout << "SG90 Servo Motor Angle Control with Hardware PWM (Non-linear Calibration)" << endl;

    if (wiringPiSetup() == -1) {
        cout << "WiringPi setup failed!" << endl;
        return 1;
    }

    pinMode(motor, PWM_OUTPUT); // 設定 GPIO18（WiringPi 1）為硬體 PWM

    // 設定硬體 PWM 時鐘和範圍
    pwmSetMode(PWM_MODE_MS);    // 使用標準 PWM 模式
    pwmSetRange(2000);          // 設定範圍為 2000（對應 20ms 週期）
    pwmSetClock(192);           // 設定時鐘為 192（19.2MHz / 192 = 100kHz）

    while (true) {
        int angle;
        cout << "Enter the servo angle (0-180): ";
        cin >> angle;

        setServoAngle(angle);
        delay(500); // 延遲 0.5 秒
    }

    return 0;
}