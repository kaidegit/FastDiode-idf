#pragma once
#include <Arduino.h>

// 定义最大计数值，用于无限闪烁模式
#define MAX_COUNT 0xffffffff / 2

// LED状态枚举
enum EStatus
{
  STATE_NULL = 0, // 空状态，用于状态重置
  STATE_BRIGHT,   // 常亮状态，可设定亮度
  STATE_FLICKER,  // 闪烁状态，可设定次数和间隔
  STATE_FADEON,   // 渐亮状态，从暗到亮
  STATE_FADEOFF,  // 渐暗状态，从亮到暗
  STATE_BREATH    // 呼吸灯状态，亮度循环变化
};

// LED极性模式枚举
enum EPolarityMode
{
  ACTIVE_LOW = 0, // 低电平点亮模式
  ACTIVE_HIGH = 1 // 高电平点亮模式
};

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
  CHANNEL_AUTO = 0xFF // 自动分配通道
};

// LED控制结构体，用于存储LED的状态和参数
struct Diode
{
  EStatus status = STATE_NULL;  // LED当前状态
  uint32_t activeTime;          // 动作持续时间
  uint32_t delayTime;           // 状态切换延时时间
  uint32_t count;               // 动作重复次数
  uint8_t brighness;            // 当前亮度值
  uint16_t stepping;            // 渐变步进值
  bool isRising = false;        // 呼吸灯上升标志
  uint8_t MaxBrighness;         // 最大亮度限制
  uint8_t breathBrightness = 0; // 呼吸灯当前亮度
};

// FastDiode类：提供LED控制功能
class FastDiode
{
private:
  uint8_t _pin;            // LED连接的GPIO引脚
  EPolarityMode _polarity; // LED极性模式
  String _name;            // LED标识名称
  uint8_t _channel;        // LEDC通道号
  uint32_t _freq;          // PWM频率
  bool _useLEDC;           // 是否使用LEDC控制
  bool _initialized;       // LEDC是否已初始化
  uint8_t _resolution;     // PWM分辨率
  bool _toggle = false;    // 闪烁状态切换标志

  TaskHandle_t taskHandle = NULL; // FreeRTOS任务句柄
  Diode notifyLED;                // LED控制通知缓存
  Diode saveLED;                  // LED状态保存缓存

  // 静态成员，用于通道管理
  static uint8_t nextAnalogChannel;   // 下一个可用通道
  static const uint8_t MAX_CHANNELS;  // 最大通道数
  static const uint8_t START_CHANNEL; // 起始通道号
  static uint32_t total_instances;    // 实例计数

  // 状态统计
  uint32_t state_changes = 0; // 状态改变次数
  uint32_t total_on_time = 0; // 总计开启时间
  bool _error_state = false;  // 错误状态标志
  String _last_error;         // 最后错误信息

  // 内部方法
  void task();                    // LED控制任务
  bool waitForNotify(Diode &led); // 等待控制通知
  bool sendNotify(EStatus _status, uint8_t _brighness, uint32_t _delayTime, uint32_t _activeTime, uint32_t _count);
  static void startTaskImpl(void *_this) { static_cast<FastDiode *>(_this)->task(); }

  // 辅助函数：将状态值转换为可读字符串
  const char *getStatusString(EStatus status); // 只声明函数

public:
  // 配置结构体，用于设置LED控制参数
  struct Config
  {
    uint32_t pwm_freq = 5000;            // PWM频率默认5KHz
    uint8_t resolution = 8;              // PWM分辨率默认8位
    uint32_t fade_step_time = 4;         // 渐变步进时间
    uint32_t min_fade_time = 255;        // 最小渐变时间
    uint32_t task_stack_size = 1024 * 2; // 任务栈大小
    uint8_t task_priority = 1;           // 任务优先级
  };

  static Config defaultConfig; // 默认配置

  // 构造函数
  FastDiode(uint8_t pin, bool reverse = false, String name = "");                                                    // 普通GPIO模式
  FastDiode(uint8_t pin, uint32_t freq, ELEDChannel channel = CHANNEL_AUTO, bool reverse = false, String name = ""); // LEDC模式

  // LED控制方法
  void begin();                                                                         // 初始化LED
  void open();                                                                          // 打开LED
  void close();                                                                         // 关闭LED
  void setBrightness(uint8_t brightness);                                               // 设置亮度
  void flickering(uint32_t time, uint32_t count = MAX_COUNT, uint8_t brightness = 255); // 闪烁控制
  void fodeOn(uint32_t time, uint8_t brightness = 255);                                 // 渐亮控制
  void fodeOff(uint32_t time, uint8_t brightness = 255);                                // 渐暗控制
  void breathing(uint32_t time, uint8_t brightness = 255);                              // 呼吸灯控制

  // 错误处理方法
  bool hasError() const { return _error_state; }
  String getLastError() const { return _last_error; }
  void clearError()
  {
    _error_state = false;
    _last_error = "";
  }

  // 统计信息方法
  static uint32_t getTotalInstances() { return total_instances; }
  uint32_t getStateChanges() const { return state_changes; }
  uint32_t getTotalOnTime() const { return total_on_time; }

protected:
  // 错误设置方法
  void setError(const String &error)
  {
    _error_state = true;
    _last_error = error;
#ifdef FAST_DIODE_DEBUG
    Serial.printf("FastDiode Error (%s): %s\n", _name.c_str(), error.c_str());
#endif
  }

private:
  void writeValue(uint8_t value); // 写入LED亮度值
  void push(const Diode &led);    // 保存LED状态
  void pull(Diode &led);          // 恢复LED状态
};
