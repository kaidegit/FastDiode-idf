#pragma once
#include <Arduino.h>

#define push(x) saveLED = x
#define pull(x) x = saveLED
#define MAX_COUNT 0xffffffff / 2

enum EStatus
{
  STATE_NULL = 0,
  STATE_BRIGHT,  // 设定亮度
  STATE_FLICKER, // 闪动
  STATE_FADEON,  // 逐渐亮起
  STATE_FADEOFF, // 逐渐熄灭
  STATE_BREATH   // 呼吸灯
};

struct Diode
{
  EStatus status = STATE_NULL; // 灯效
  uint32_t activeTime;         // 作用时间时间,比如在 多少时间逐步变亮 / 暗,或者是呼吸灯一次的时间
  uint32_t delayTime;          // 等待通知的时间，在此时间内， 如果接收到通知，则进入下一个状态
  uint32_t count;              // 作用次数 主要用于约束闪动次数
  uint8_t brighness;           // 亮度
  uint16_t stepping;           // 渐进量
  bool edge = false;           // false 呼吸灯上升沿.   ture下降沿
  uint8_t MaxBrighness;        // 呼吸灯最大亮度
};

class FastDiode
{
private:
  uint8_t pin;
  TaskHandle_t taskHandle = NULL;
  Diode notifyLED; // 用于发送状态的缓存
  Diode saveLED;   // 用于存储 前一个状态的缓存
  String name;
  bool edge; // flase 拉低点亮.   ture 拉高点亮

  // 处理任务
  void task();
  // 等待通知，用来取代vTaskDelay , 这样在延时的过程中, 只要接收到数据就能跳出延时
  bool waitForNotify(Diode &led);
  // 发送通知
  bool sendNotify(EStatus _status, uint8_t _brighness, uint32_t _delayTime, uint32_t _activeTime, uint32_t _count);

  static void startTaskImpl(void *_this) { static_cast<FastDiode *>(_this)->task(); }

public:
  FastDiode(uint8_t _pin, bool _edge = false, String _name = " ")
  {
    pin = _pin;
    edge = _edge;
    name = _name;
    pinMode(pin, OUTPUT);
    xTaskCreate(this->startTaskImpl, "waitTask", 1024 * 2, this, 1, &taskHandle);
    sendNotify(STATE_BRIGHT, 0, 0, 0, 0);
  }

  /// @brief 开灯
  void open()
  {
    if (!edge)
      sendNotify(STATE_BRIGHT, 255, 0, 0, 0);
    else
      sendNotify(STATE_BRIGHT, 0, 0, 0, 0);
  }
  /// @brief 关灯
  void close()
  {
    if (!edge)
      sendNotify(STATE_BRIGHT, 0, 0, 0, 0);
    else
      sendNotify(STATE_BRIGHT, 255, 0, 0, 0);
  }

  /// @brief 设定亮度 0~255
  void setBrightness(uint8_t brightness) { sendNotify(STATE_BRIGHT, brightness, 0, 0, 0); }

  /// @brief  闪灯
  /// @param time 时间间隔
  /// @param count 闪动次数
  /// @param brightness 闪动亮度
  void flickering(uint32_t time, uint32_t count = MAX_COUNT, uint8_t brightness = 255) { sendNotify(STATE_FLICKER, brightness, time, 0, count); }

  /// @brief 逐渐变亮
  /// @param time 时间
  /// @param brightness 最终亮度
  void fodeOn(uint32_t time, uint8_t brightness = 255) { sendNotify(STATE_FADEON, brightness, 0, time, 0); }

  /// @brief 逐渐变暗
  /// @param time 时间
  /// @param brightness 开始亮度
  void fodeOff(uint32_t time, uint8_t brightness = 255) { sendNotify(STATE_FADEOFF, brightness, 0, time, 0); }

  /// @brief 呼吸灯
  /// @param time 时间
  /// @param brightness 亮度
  void breathing(uint32_t time, uint8_t brightness = 255) { sendNotify(STATE_BREATH, brightness, 0, time, 0); }
};
