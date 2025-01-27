/**
 * @file basic_example.cpp
 * @brief FastDiode库基础示例
 * @details 演示FastDiode库的各种LED控制效果，包括：
 *          - 开关控制
 *          - 亮度调节
 *          - 渐亮渐暗
 *          - 闪烁效果
 *          - 呼吸灯效果
 *
 * @author CHIYOOOO
 * @date 2025-01-26
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025
 *
 * @note 硬件连接：
 *       - LED: GPIO12 (低电平点亮)
 */

#include <Arduino.h>
#include "FastDiode.h"

#define LED_PIN 12

// 使用普通 GPIO 方式
FastDiode led(LED_PIN, EPinPolarity::ACTIVE_LOW, "led");

void setup()
{
    Serial.begin(115200);
    Serial.println("Hello, FastDiode!");
    // 不调用init()，默认使用普通GPIO方式，默认GPIO形式PWM频率被设置为1000Hz
    // 调用init()，使用LEDC PWM方式，默认PWM频率被设置为5000Hz，PWM过低会导致LED在低亮度时闪烁
    // led.init(CHANNEL_0);
}

void loop()
{
    // 1. 开关测试
    Serial.println("1. 开关测试");
    led.open(); // 打开
    delay(1000);
    led.close(); // 关闭
    delay(1000);

    // 2. 亮度渐变
    Serial.println("2. 亮度测试");
    led.setBrightness(50); // 20%亮度
    delay(1000);
    led.setBrightness(125); // 50%亮度
    delay(1000);
    led.setBrightness(255); // 100%亮度
    delay(1000);

    // 3. 渐亮效果
    Serial.println("3. 渐亮效果");
    led.fodeOn(2000); // 2秒内渐亮
    delay(2000);

    // 4. 渐暗效果
    Serial.println("4. 渐暗效果");
    led.fodeOff(2000); // 2秒内渐暗
    delay(2000);

    // 6. 呼吸灯效果
    Serial.println("5. 呼吸灯效果");
    led.breathing(500); // 呼吸周期1秒
    delay(5000);        // 显示5秒呼吸效果

    // 5. 闪烁效果
    Serial.println("6. 闪烁效果 闪烁3次后重新回到呼吸灯效果");
    led.flickering(200, 3); // 闪烁3次后重新回到呼吸灯效果
    delay(3000);

    // 暂停一下，准备开始下一轮演示
    led.close();
    delay(2000);
    Serial.println("------- 重新开始演示 -------\n");
}