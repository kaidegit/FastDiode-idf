/**
 * @file touch_desk_lamp.cpp
 * @brief FastDiode库实现触摸台灯
 * @details 使用FastDiode和OneButton库实现触摸台灯功能：
 *          - 单击：开关灯
 *          - 长按：无极调光（0-100%渐变，约5秒完成一次循环）
 *          - 断电记忆（使用EEPROM保存亮度）
 *
 * @author Mushroom
 * @date 2024-01-15
 * @version 1.0
 *
 * @copyright Copyright (c) 2024
 *
 * @note 硬件连接：
 *       - LED: GPIO12 (低电平点亮)
 *       - 触摸按键: GPIO9 (低电平触发)
 */

#include <Arduino.h>
#include "FastDiode.h"
#include <OneButton.h>
#include <EEPROM.h>

// 引脚定义
#define LED_PIN 12
#define TOUCH_PIN 9
#define EEPROM_SIZE 4
#define BRIGHTNESS_ADDR 0

// 创建LED对象
FastDiode led(LED_PIN, EPinPolarity::ACTIVE_LOW, "desk_lamp");

// 创建按键对象（低电平触发）
OneButton button(TOUCH_PIN, true);

// 灯的状态
bool isLampOn = false;
bool isDimming = false;        // 是否正在调光
bool dimmingDirection = true;  // 调光方向 true=变亮 false=变暗
uint8_t currentPercent = 50;   // 当前亮度百分比（0-100）
unsigned long lastDimTime = 0; // 上次调光时间

// 调光参数
const unsigned long DIM_INTERVAL = 20; // 调光间隔(ms)
const uint8_t PERCENT_STEP = 1;        // 每次调光步进（1%）
const uint16_t FULL_CYCLE_TIME = 5000; // 完整调光周期时间（ms）
const uint8_t MIN_PERCENT = 2;         // 最小亮度百分比
const uint8_t MAX_PERCENT = 100;       // 最大亮度百分比

// 保存亮度到EEPROM
void saveBrightness()
{
    EEPROM.write(BRIGHTNESS_ADDR, currentPercent);
    EEPROM.commit();
    Serial.printf("亮度已保存: %d%%\n", currentPercent);
}

// 加载保存的亮度
void loadBrightness()
{
    currentPercent = EEPROM.read(BRIGHTNESS_ADDR);
    // 确保读取的值有效
    if (currentPercent > MAX_PERCENT || currentPercent < MIN_PERCENT)
    {
        currentPercent = 50; // 默认50%
    }
    Serial.printf("加载保存的亮度: %d%%\n", currentPercent);
}

// 设置LED亮度
void setLEDBrightness(uint8_t percent)
{
    uint8_t brightness = map(percent, 0, 100, 0, 255);
    led.setBrightness(brightness);
}

// 单击回调：开关灯
void toggleLamp()
{
    isLampOn = !isLampOn;
    if (isLampOn)
    {
        setLEDBrightness(currentPercent);
        Serial.printf("灯已开启，当前亮度: %d%%\n", currentPercent);
    }
    else
    {
        led.close();
        Serial.println("灯已关闭");
    }
}

// 开始长按
void startDimming()
{
    if (!isLampOn)
        return;
    isDimming = true;
    lastDimTime = millis();
    Serial.println("开始调光...");
}

// 结束长按
void stopDimming()
{
    if (!isLampOn)
        return;
    isDimming = false;
    saveBrightness(); // 保存当前亮度
    Serial.printf("调光结束，当前亮度: %d%%\n", currentPercent);
}

// 执行调光（非阻塞）
void doDimming()
{
    if (!isDimming)
        return;

    unsigned long currentTime = millis();
    if (currentTime - lastDimTime >= DIM_INTERVAL)
    {
        // 根据方向调整亮度
        if (dimmingDirection)
        {
            if (currentPercent < MAX_PERCENT)
            {
                currentPercent += PERCENT_STEP;
                if (currentPercent > MAX_PERCENT)
                    currentPercent = MAX_PERCENT;
                setLEDBrightness(currentPercent);
                Serial.printf("当前亮度: %d%%\n", currentPercent);
            }
            else
            {
                dimmingDirection = false;
            }
        }
        else
        {
            if (currentPercent > MIN_PERCENT)
            {
                currentPercent -= PERCENT_STEP;
                if (currentPercent < MIN_PERCENT)
                    currentPercent = MIN_PERCENT;
                setLEDBrightness(currentPercent);
                Serial.printf("当前亮度: %d%%\n", currentPercent);
            }
            else
            {
                dimmingDirection = true;
            }
        }

        lastDimTime = currentTime;
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("触摸台灯初始化...");

    // 初始化EEPROM
    EEPROM.begin(EEPROM_SIZE);
    loadBrightness();

    // 配置按键回调函数
    button.attachClick(toggleLamp);
    button.attachLongPressStart(startDimming);
    button.attachLongPressStop(stopDimming);

    // 设置按键参数（使用新的API）
    button.setDebounceMs(50); // 消抖时间50ms
    button.setClickMs(250);   // 单击判定时间250ms
    button.setPressMs(400);   // 长按判定时间400ms

    // 初始状态：关闭
    led.close();
    Serial.println("初始化完成，单击开关，长按调节亮度");
}

void loop()
{
    // 检测按键状态
    button.tick();

    // 执行调光（非阻塞）
    doDimming();

    // 这里可以执行其他任务
    // ...
}