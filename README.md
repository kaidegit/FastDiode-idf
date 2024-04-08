# FastDiode

## 介绍
FastDiode是一个为单色LED设计的库，它能够轻松实现多个LED的点亮、关闭、呼吸、闪烁等效果。该库适用于ESP32、ESP32-S2、ESP32-S3、ESP32-C3等系列的ESP芯片。

## 软件架构
FastDiode基于Arduino平台构建，利用FreeRTOS和`analogWrite()`函数来实现LED的控制。

## 安装教程

1. **引用头文件**
   ```cpp
   #include "FastDiode.h"
   ```

2. **实例化对象**
   ```cpp
   FastDiode redLED(19); // 创建一个FastDiode对象，连接到GPIO 19
   ```

3. **调用功能**
   - `redLED.open();` // 开启LED
   - `redLED.close();` // 关闭LED
   - `redLED.setBrightness(value);` // 设置LED亮度，`value`为0到255的整数
   - `redLED.flickering(period);` // LED持续闪烁，`period`为闪烁间隔时间（毫秒）
   - `redLED.flickering(period, times);` // LED闪烁指定次数后停止，`times`为闪烁次数
   - `redLED.fadeOn(duration);` // LED在`duration`毫秒内逐渐变亮
   - `redLED.fadeOff(duration);` // LED在`duration`毫秒内逐渐变暗
   - `redLED.breathing(period);` // 实现LED呼吸灯效果，`period`为呼吸周期时间（毫秒）

## 使用说明
- FastDiode使用FreeRTOS和`analogWrite()`函数来控制LED，因此不支持ESP8266，但支持ESP32及其衍生版本。
- 可以控制的LED数量取决于所使用的ESP芯片的PWM通道数量。例如，ESP32支持多达16路PWM，而ESP32-C3支持6路PWM。具体的最大数量可能需要根据硬件进行测试。
- 在使用FastDiode库时，请确保正确配置GPIO引脚，并根据需要调整PWM通道的设置。