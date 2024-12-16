#include <wiringPi.h>
#include <iostream>
#include <sqlite3.h>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

const int motor = 1;   // WiringPi 編號 1，對應 GPIO18
const int minPwm = 50; // 對應 1ms 脈寬（50）
const int maxPwm = 250; // 對應 2ms 脈寬（250）

void setServoAngle(int angle) {
    if (angle < 0 || angle > 180) {
        cout << "Invalid angle! Please enter a value between 0 and 180." << endl;
        return;
    }

    // 將校準後的角度映射到 PWM 值
    int pwmValue = minPwm + (angle * (maxPwm - minPwm) / 180);
    pwmWrite(motor, pwmValue);
    cout << "Servo angle set to " << angle << ", PWM: " << pwmValue << endl;
}

void store_to_db(int angle) {
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    // 打開資料庫
    rc = sqlite3_open("motor.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // 建立資料表（如果不存在）
    const char *sqlCreateTable = "CREATE TABLE IF NOT EXISTS motor ("
                                 "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "Angle INT NOT NULL, "
                                 "Time TEXT NOT NULL);";
    rc = sqlite3_exec(db, sqlCreateTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    // 獲取當前時間
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    std::string currentTime = ss.str();

    // 插入資料
    const char *sqlInsert = "INSERT INTO motor (Angle, Time) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sqlInsert, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, angle);
    sqlite3_bind_text(stmt, 2, currentTime.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
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
        store_to_db(angle);
        delay(500); // 延遲 0.5 秒
    }

    return 0;
}
