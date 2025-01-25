#include <Arduino.h>
#include "FastDiode.h"

// 使用LEDC方式，频率5000Hz，通道0
FastDiode D5(13, 5000, CHANNEL_0, ACTIVE_LOW, "D5");

// 使用analogWrite方式（向后兼容）
FastDiode D4(12, ACTIVE_LOW, "D4");

// 按键连接到GPIO 9
const uint8_t BUTTON_PIN = 9;

void setup()
{
    Serial.begin(115200);
    Serial.println("Hello, ESP32!");

    // 对于使用LEDC的LED，需要调用begin()
    D5.begin();
    D5.breathing(2000);
    // D5.flickering(1000);

    // 设置按键为上拉输入
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop()
{
    static uint8_t demoMode = 0;

    if (digitalRead(BUTTON_PIN) == LOW)
    {
        // 按键按下时，切换演示模式
        demoMode = (demoMode + 1) % 3; // 改为循环切换8种效果

        // 根据当前模式执行相应效果
        switch (demoMode)
        {
        case 0: // 开灯
            Serial.println("开灯");
            D4.open();
            break;
        // case 1: // 关灯
        //     Serial.println("关灯");
        //     D4.close();
        //     break;
        // case 2: // 设置亮度
        //     Serial.println("设置亮度");
        //     D4.setBrightness(122);
        //     break;
        // case 3: // 闪烁
        //     Serial.println("闪动效果");
        //     D4.flickering(500);
        //     break;
        // case 4: // 渐亮
        //     Serial.println("逐渐变亮");
        //     D4.fodeOn(2000);
        //     break;
        // case 5: // 渐暗
        //     Serial.println("逐渐变暗");
        //     D4.fodeOff(2000);
        //     break;
        case 1: // 呼吸灯
            Serial.println("呼吸灯效果");
            D4.breathing(500);
            break;
        case 2: // 闪动几次后恢复
            Serial.println("闪动几次后回到闪动前的状态(即回到呼吸状态)");
            D4.flickering(200, 2);
            D5.flickering(200, 2);
            break;
        }

        delay(300); // 按键消抖
    }
}
