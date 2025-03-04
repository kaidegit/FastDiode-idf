# FastDiode

## 简介

FastDiode 是一个为 ESP32 系列芯片设计的 LED 控制库，提供简单易用的 API 来实现 LED 的各种效果控制。支持 ESP32、ESP32-S2、ESP32-S3、ESP32-C3 等系列芯片。

本仓库为https://gitee.com/chiyoooo/fast-diode的fork版本，支持了esp-idf

## 特性

- 支持两种控制模式:
  - LEDC PWM 控制模式 (5KHz)
  - 普通 GPIO (analogWrite) 控制模式 (1KHz)
- 支持多个 LED 同时控制
- 自动管理 PWM 通道资源
- 丰富的灯效：
  - 开关控制
  - 亮度调节（0-255）
  - 闪烁效果（可设置次数和恢复）
  - 渐亮/渐暗效果
  - 呼吸灯效果
- 基于 FreeRTOS 任务的非阻塞控制
- 支持高低电平触发（ACTIVE_HIGH/ACTIVE_LOW）
- 支持状态保存和恢复

## 安装

1. 使用 PlatformIO

   ```ini
   lib_deps =
       https://github.com/chiyoooo/FastDiode.git
   ```

2. 手动安装
   - 下载本库到 Arduino 的库文件夹
   - 或复制到项目的 lib 目录

## 快速开始

### 基础示例

```cpp
#include <Arduino.h>
#include "FastDiode.h"

// 使用 LEDC PWM 方式
FastDiode led1(13, EPinPolarity::ACTIVE_LOW, "LED1");

void setup() {
    Serial.begin(115200);

    // 初始化 LEDC 模式 (可选)
    led1.init(CHANNEL_0);  // 5KHz PWM

    // 设置呼吸灯效果
    led1.breathing(500);  // 500ms 周期
}

void loop() {
    // 基础控制
    led1.open();                     // 打开 LED
    delay(1000);

    led1.setBrightness(122);         // 设置亮度 (0-255)
    delay(1000);

    led1.flickering(500, 3);         // 闪烁3次后恢复
    delay(2000);

    led1.fodeOn(2000);              // 2秒内渐亮
    delay(2000);

    led1.fodeOff(2000);             // 2秒内渐暗
    delay(2000);

    led1.breathing(500);            // 呼吸灯效果
    delay(5000);
}
```

### 实际应用示例

1. 触摸台灯

```cpp
// 单击开关灯
// 长按无极调光
// 支持断电记忆
FastDiode led(12, EPinPolarity::ACTIVE_LOW);
```

2. 双 LED 控制

```cpp
// LED1: 开关/调光/渐变
// LED2: 呼吸灯效果
FastDiode led1(12, EPinPolarity::ACTIVE_LOW);
FastDiode led2(13, EPinPolarity::ACTIVE_LOW);
```

## API 说明

### 构造函数

```cpp
FastDiode(uint8_t pin, EPinPolarity edge = EPinPolarity::ACTIVE_HIGH, String name = "")
```

参数说明:

- `pin`: LED 连接的 GPIO 引脚
- `edge`: 触发电平 (ACTIVE_LOW/ACTIVE_HIGH)
- `name`: LED 标识名称

### 初始化函数

```cpp
void init(ELEDChannel channel, uint32_t freq = 5000, uint8_t resolution = 8)
```

参数说明:

- `channel`: LEDC 通道
- `freq`: PWM 频率，默认 5KHz
- `resolution`: PWM 分辨率，默认 8 位

### 控制函数

- `open()` - 打开 LED
- `close()` - 关闭 LED
- `setBrightness(uint8_t brightness)` - 设置亮度 (0-255)
- `flickering(uint32_t time, uint32_t count = MAX_COUNT, uint8_t brightness = 255)` - 闪烁效果
- `fodeOn(uint32_t time, uint8_t brightness = 255)` - 渐亮效果
- `fodeOff(uint32_t time, uint8_t brightness = 255)` - 渐暗效果
- `breathing(uint32_t time, uint8_t brightness = 255)` - 呼吸灯效果

## 注意事项

1. LEDC 模式需调用 init() 初始化
2. 渐变效果最小时间为 255ms
3. 闪烁次数不指定时持续闪烁
4. 指定闪烁次数后会恢复之前状态
5. LEDC 模式 PWM 频率为 5KHz，普通 GPIO 模式为 1KHz
6. 每个 LED 实例会创建一个 FreeRTOS 任务

## 硬件兼容性

- ESP32 系列
  - ESP32-WROOM ✓
  - ESP32-WROVER ✓
  - ESP32-S2 ✓
  - ESP32-C3 ✓
  - ESP32-S3 ✓

## 依赖项

- Arduino ESP32 Core >= 2.0.0
- FreeRTOS
- ESP32 LEDC 硬件支持

## 许可证

MIT License

## 作者

[CHIYoooo](https://gitee.com/chiyoooo)
