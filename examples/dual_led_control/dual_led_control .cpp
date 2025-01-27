/**
 * @file dual_led_control.cpp
 * @brief FastDiode库双LED独立控制示例
 * @details 演示FastDiode库控制两个LED独立运行不同效果：
 *          LED1演示效果：
 *          - 开关控制
 *          - 亮度调节
 *          - 渐亮渐暗
 *          - 闪烁效果
 *
 *          LED2同时进行：
 *          - 呼吸灯效果
 *          - 其他独立控制
 *
 * @author Mushroom
 * @date 2024-01-15
 * @version 1.0
 *
 * @copyright Copyright (c) 2024
 *
 * @note 硬件连接：
 *       - LED1: GPIO12 (低电平点亮)
 *       - LED2: GPIO13 (低电平点亮)
 */
#include <Arduino.h>
#include "FastDiode.h"

FastDiode D4(12, EPinPolarity::ACTIVE_LOW); // LED 使用引脚 12
FastDiode D5(13, EPinPolarity::ACTIVE_LOW); // LED 使用引脚 13
#define BUTTON_PIN 9                        // 按键连接到引脚 9

bool lastButtonState = HIGH;
int effectMode = 0; // 灯效模式

void setup()
{
    Serial.begin(115200);
    Serial.println("Hello, FastDiode!");
    D4.init(CHANNEL_0);
    D5.breathing(1000, 125);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop()
{
    bool buttonState = digitalRead(BUTTON_PIN);

    // 按键按下后释放时切换效果
    if (buttonState == HIGH && lastButtonState == LOW)
    {
        effectMode = (effectMode + 1) % 6; // 循环切换效果
        switch (effectMode)
        {
        case 0:
            Serial.println("普通开关灯");
            D4.open();
            break;
        case 1:
            Serial.println("设置亮度");
            D4.setBrightness(122);
            break;
        case 2:
            Serial.println("渐亮效果");
            D4.fodeOn(2000, 20);
            break;
        case 3:
            Serial.println("渐暗效果");
            D4.fodeOff(2000, 20);
            break;
        case 4:
            Serial.println("呼吸灯效果");
            D4.breathing(500);
            // D4.close();
            break;
        case 5:
            Serial.println("闪烁效果");
            D4.flickering(500, 2);
            D5.flickering(500, 2);
            break;
        }
    }

    lastButtonState = buttonState;
    delay(50); // 消抖延时
}
