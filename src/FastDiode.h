#pragma once
#include <Arduino.h>

#define push(x) saveLED = x
#define pull(x) x = saveLED
#define MAX_COUNT 0xffffffff / 2

// LED通道枚举，用于LEDC控制
enum ELEDChannel
{
  CHANNEL_0 = 0,
  CHANNEL_1 = 1,
  CHANNEL_2 = 2,
  CHANNEL_3 = 3,
  CHANNEL_4 = 4,
  CHANNEL_5 = 5,
  CHANNEL_6 = 6,
  CHANNEL_7 = 7,
#if defined(CONFIG_IDF_TARGET_ESP32)
  CHANNEL_8 = 8, // ESP32支持更多通道
  CHANNEL_9 = 9,
  CHANNEL_10 = 10,
  CHANNEL_11 = 11,
  CHANNEL_12 = 12,
  CHANNEL_13 = 13,
  CHANNEL_14 = 14,
  CHANNEL_15 = 15,
#endif
};

// LED极性枚举
enum class EPinPolarity
{
  ACTIVE_LOW = false, // 低电平有效（低电平点亮）
  ACTIVE_HIGH = true  // 高电平有效（高电平点亮）
};

// 灯效状态枚举
enum class EEffectType
{
  NONE = 0, // 无灯效
  STATIC,   // 固定亮度
  BLINK,    // 闪烁
  FADE_IN,  // 渐亮
  FADE_OUT, // 渐暗
  BREATHING // 呼吸灯
};

// 呼吸灯方向枚举
enum class EBreathDirection
{
  FADE_IN = false, // 渐亮
  FADE_OUT = true  // 渐暗
};

struct LEDState
{
  EEffectType status = EEffectType::NONE;                 // 灯效
  uint32_t totalDuration;                                 // 灯效总时长,比如在多少时间逐步变亮 / 暗,或者是呼吸灯一次的时间，在开关设定亮度，闪灯等瞬态的事件中，一般为0
  uint32_t stepInterval;                                  // 步进时间 每个步骤之间的时间间隔，也就是呼吸灯每一步亮度持续的事件，同时这个值也用于任务通知等待事件，如果接收到通知，则进入下一个状态，避免在呼吸灯等持续事件中，没办法及时切换灯效
  uint32_t repeatCount;                                   // 重复次数主要用于约束闪动次数，闪动次数为0时，则会结束当前灯效，回到上个灯效e，如果闪动次数不指定，则无限闪动
  uint8_t currentBrightness;                              // 当前亮度值
  uint16_t stepping;                                      // 渐进量，在渐变，呼吸灯等灯效中，表示每一步的亮度变化量
  EBreathDirection direction = EBreathDirection::FADE_IN; // false 呼吸灯上升沿.   ture下降沿
  uint8_t targetBrightness;                               // 目标亮度
};

class FastDiode
{
private:
  uint8_t pin;                                  // 引脚
  uint8_t channel;                              // 通道
  TaskHandle_t taskHandle = NULL;               // 任务句柄
  LEDState notifyLED;                           // 用于发送状态的缓存
  LEDState saveLED;                             // 用于存储 前一个状态的缓存
  String name;                                  // LED灯标记名
  EPinPolarity edge = EPinPolarity::ACTIVE_LOW; // 引脚极性
  bool initialized = false;                     // 是否已初始化，如果调用初始化，就使用LEDC

  // 处理任务
  void task();
  // 等待通知，用来取代vTaskDelay , 这样在延时的过程中, 只要接收到数据就能跳出延时
  bool waitForNotify(LEDState &led);
  // 发送通知
  bool sendNotify(EEffectType _status,       // 灯效
                  uint8_t _targetBrightness, // 目标亮度
                  uint32_t _stepInterval,    // 步进时间
                  uint32_t _totalDuration,   // 总时间
                  uint32_t _repeatCount);    // 重复次数
  // 启动任务
  static void startTaskImpl(void *_this) { static_cast<FastDiode *>(_this)->task(); }

  // 设置亮度 ,在这边判断initialized，如果initialized为true，则使用LEDC，否则使用analogWrite
  void setBrightnessImpl(uint8_t brightness)
  {
    if (initialized)
      ledcWrite(channel, brightness);
    else
      analogWrite(pin, brightness);
  }

public:
  FastDiode(uint8_t _pin, EPinPolarity _edge = EPinPolarity::ACTIVE_HIGH, String _name = " ")
  {
    pin = _pin;
    edge = _edge;
    name = _name;
    pinMode(pin, OUTPUT);
    xTaskCreate(this->startTaskImpl, _name.c_str(), 1024 * 2, this, 1, &taskHandle);
    sendNotify(EEffectType::STATIC, 0, 0, 0, 0);
  }

  /// @brief 初始化
  /// @param channel 通道
  /// @param freq 频率
  /// @param resolution 分辨率
  void init(ELEDChannel _channel, uint32_t freq = 5000, uint8_t resolution = 8)
  {
    initialized = true;
    channel = _channel;
    ledcSetup(channel, freq, resolution);
    ledcAttachPin(pin, channel);
  }

  /// @brief 开灯
  void open()
  {
    if (edge == EPinPolarity::ACTIVE_LOW)
      sendNotify(EEffectType::STATIC, // 固定亮度
                 255,                 // 亮度
                 0,                   // 步进时间:无，直接设定
                 0,                   // 总时间:无，一直保持
                 0);                  // 重复次数:无，一直保持
    else
      sendNotify(EEffectType::STATIC, // 固定亮度
                 0,                   // 亮度
                 0,                   // 步进时间:无，直接设定
                 0,                   // 总时间:无，一直保持
                 0);                  // 重复次数:无，一直保持
  }
  /// @brief 关灯
  void close()
  {
    if (edge == EPinPolarity::ACTIVE_LOW)
      sendNotify(EEffectType::STATIC, // 固定亮度
                 0,                   // 亮度
                 0,                   // 步进时间:无，直接设定
                 0,                   // 总时间:无，一直保持
                 0);                  // 重复次数:无，一直保持
    else
      sendNotify(EEffectType::STATIC, // 固定亮度
                 255,                 // 亮度
                 0,                   // 步进时间:无，直接设定
                 0,                   // 总时间:无，一直保持
                 0);                  // 重复次数:无，一直保持
  }

  /// @brief 设定亮度 0~255
  void setBrightness(uint8_t brightness)
  {
    sendNotify(EEffectType::STATIC, // 固定亮度
               brightness,          // 亮度
               0,                   // 步进时间:无，直接设定
               0,                   // 总时间:无，一直保持
               0);                  // 重复次数:无，一直保持
  }

  /// @brief  闪灯
  /// @param time 时间间隔
  /// @param repeatCount 闪动次数
  /// @param brightness 闪动亮度
  void flickering(uint32_t time, uint32_t repeatCount = MAX_COUNT, uint8_t brightness = 255)
  {
    sendNotify(EEffectType::BLINK, // 闪烁
               brightness,         // 闪动亮度
               time,               // 步进时间:闪烁时间间隔
               0,                  // 总时间:无，一直保持
               repeatCount);       // 重复次数，默认MAX_COUNT无限闪动
  }

  /// @brief 逐渐变亮
  /// @param time 时间
  /// @param brightness 最终亮度
  void fodeOn(uint32_t time, uint8_t brightness = 255)
  {
    sendNotify(EEffectType::FADE_IN, // 逐渐变亮
               brightness,           // 最终亮度，默认255 最亮
               0,                    // 步进时间:无，直接设定
               time,                 // 总时间：经过多久到达最终亮度
               0);                   // 重复次数:无，一直保持
  }

  /// @brief 逐渐变暗
  /// @param time 时间
  /// @param brightness 开始亮度
  void fodeOff(uint32_t time, uint8_t brightness = 255)
  {
    sendNotify(EEffectType::FADE_OUT, // 逐渐变暗
               brightness,            // 开始亮度，默认255 最亮
               0,                     // 步进时间:无，直接设定
               time,                  // 总时间：经过多久到达最终亮度
               0);                    // 重复次数:无，一直保持
  }

  /// @brief 呼吸灯
  /// @param time 时间
  /// @param brightness 亮度
  void breathing(uint32_t time, uint8_t brightness = 255)
  {
    sendNotify(EEffectType::BREATHING, // 呼吸灯
               brightness,             // 亮度
               0,                      // 步进时间:无，直接设定，由总时间和总亮度计算得到。
               time,                   // 总时间：呼吸灯一次的时间
               0);                     // 重复次数:无，一直保持
  }
};
