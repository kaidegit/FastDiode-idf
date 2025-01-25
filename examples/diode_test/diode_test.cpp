#include <Arduino.h>
#include "FastDiode.h"

// 测试用的 LED 引脚定义
#define TEST_LED_PIN_PWM 13
#define TEST_LED_PIN_GPIO 12

FastDiode ledPWM(TEST_LED_PIN_PWM, 5000, CHANNEL_0, ACTIVE_LOW, "TestLED_PWM");
FastDiode ledGPIO(TEST_LED_PIN_GPIO, ACTIVE_LOW, "TestLED_GPIO");

void testBasicFunctions()
{
    Serial.println("测试基础功能...");

    // 测试开关功能
    Serial.println("测试LED开关...");
    ledPWM.open();
    delay(1000);
    ledPWM.close();
    delay(1000);

    // 测试亮度设置
    Serial.println("测试亮度调节...");
    for (int i = 0; i <= 255; i += 51)
    {
        Serial.printf("设置亮度: %d\n", i);
        ledPWM.setBrightness(i);
        delay(500);
    }
}

void testEffects()
{
    Serial.println("测试特效功能...");

    // 测试闪烁效果
    Serial.println("测试闪烁效果...");
    ledPWM.flickering(500, 3);
    delay(2000);

    // 测试渐变效果
    Serial.println("测试渐亮效果...");
    ledPWM.fodeOn(1000);
    delay(1500);

    Serial.println("测试渐暗效果...");
    ledPWM.fodeOff(1000);
    delay(1500);

    // 测试呼吸灯效果
    Serial.println("测试呼吸灯效果...");
    ledPWM.breathing(1000, 255);
    delay(3500);
}

void testGPIOMode()
{
    Serial.println("测试GPIO模式...");

    Serial.println("GPIO LED开关测试...");
    ledGPIO.open();
    delay(1000);
    ledGPIO.close();
    delay(1000);

    // 测试呼吸灯效果
    Serial.println("测试呼吸灯效果...");
    ledPWM.breathing(500, 255);
    delay(3500);

    Serial.println("GPIO LED闪烁测试...");
    ledGPIO.flickering(500, 3);
    delay(2000);
}

void setup()
{
    Serial.begin(115200);
    delay(2000); // 等待串口稳定

    Serial.println("\n开始FastDiode测试...");

    // 直接调用begin()，不检查返回值
    ledPWM.begin();
    // ledGPIO.begin();

    Serial.println("LED初始化完成");
}

void loop()
{
    Serial.println("\n=== 开始新一轮测试 ===");

    testBasicFunctions();
    delay(1000);

    testEffects();
    delay(1000);

    testGPIOMode();
    delay(1000);

    Serial.println("测试完成，5秒后重新开始...\n");
    delay(5000);
}