#include "FastDiode.h"

// 等待接收LED控制通知
// 参数 led: 用于存储接收到的LED参数
// 返回值: 成功接收返回1，超时返回0
bool FastDiode::waitForNotify(Diode &led)
{
    uint32_t value;
    // 等待任务通知，使用LED自己的延时时间
    if (pdTRUE == xTaskGenericNotifyWait(0, 0, ULONG_MAX, &value, pdMS_TO_TICKS(led.delayTime)))
    {
        // 将接收到的数据安全地复制到LED结构体中，避免指针访问可能的问题
        memcpy(&led, (void *)value, sizeof(Diode));
        return 1;
    }
    return 0;
}

// 发送LED控制通知，用于设置LED的各种效果
// 参数说明:
// _status: LED状态(如闪烁、渐变等)
// _brighness: 目标亮度值(0-255)
// _delayTime: 延时时间(ms)，用于控制效果的速度
// _activeTime: 动作持续时间(ms)，用于渐变效果
// _count: 动作重复次数，用于闪烁效果
bool FastDiode::sendNotify(EStatus _status, uint8_t _brighness, uint32_t _delayTime, uint32_t _activeTime, uint32_t _count)
{
    // 在切换到闪烁模式时，保存当前LED状态，以便闪烁完成后恢复
    if (_status == STATE_FLICKER)
    {
        // 只在非闪烁状态时保存状态
        if (notifyLED.status != STATE_FLICKER)
        {
#ifdef FAST_DIODE_DEBUG
            Serial.printf("[%s] Saving state before flicker: %s\n", _name.c_str(), getStatusString(notifyLED.status));
#endif
            memcpy(&saveLED, &notifyLED, sizeof(Diode));
        }
    }

    // 恢复任务运行（如果任务被挂起）
    // 任务可能在等待新命令时被挂起，需要先恢复运行
    vTaskResume(taskHandle);
    notifyLED.status = _status;
    notifyLED.MaxBrighness = _brighness;

    // 对不同类型的效果进行特殊处理
    if (notifyLED.status == STATE_FADEON || notifyLED.status == STATE_FADEOFF || notifyLED.status == STATE_BREATH)
    {
        // 渐变效果的处理
        // 由于亮度范围是0-255，确保总时间不小于255ms，保证每级亮度至少1ms
        if (_activeTime < 255)
            _activeTime = 255;
        // 计算每级亮度变化的延时时间，实现平滑渐变
        notifyLED.delayTime = _activeTime / _brighness;
        notifyLED.stepping = notifyLED.delayTime;
        notifyLED.breathBrightness = 0; // 初始化呼吸灯亮度
        notifyLED.isRising = true;      // 从暗到亮开始
    }
    else if (notifyLED.status == STATE_FLICKER)
    {
        // 闪烁效果参数设置
        notifyLED.delayTime = _delayTime / 2; // 每次闪烁使用一半时间
        notifyLED.activeTime = _delayTime;    // 保存原始延时用于恢复
    }
    else
    {
        // 其他效果的处理（如常亮状态）
        notifyLED.delayTime = _delayTime;
        notifyLED.activeTime = _activeTime;
    }

    notifyLED.brighness = _brighness;

    // 闪烁次数处理：一次完整的闪烁包含亮和灭两个状态
    // 例如：闪烁2次实际需要4个状态切换（亮-灭-亮-灭）
    if (_count != 1)
        notifyLED.count = _count * 2; // 所以实际次数要乘2
    else
        notifyLED.count = _count;

    // 发送通知到LED控制任务
    // eSetValueWithoutOverwrite: 如果已有通知未处理，则不覆盖
    // 这样可以避免快速连续发送命令时丢失状态
    return xTaskGenericNotify(taskHandle, 0, (uint32_t)&notifyLED, eSetValueWithoutOverwrite, NULL) == pdPASS;
}

// LED控制任务的主函数
// 该函数实现了LED的各种控制效果，包括常亮、闪烁、渐变等
// 作为一个独立的FreeRTOS任务运行
void FastDiode::task()
{
    Diode LED;
    EStatus lastState = STATE_NULL;
    // 将 toggle 移到类成员变量中
    while (1)
    {
        this->waitForNotify(LED);

        switch (LED.status)
        {
        case STATE_FLICKER:
        {
            if (LED.count == 0)
            {
#ifdef FAST_DIODE_DEBUG
                Serial.printf("[%s] Flicker complete\n", _name.c_str());
#endif
                pull(LED);
                // 确保恢复后不会立即再次进入闪烁状态
                LED.status = saveLED.status;
                continue;
            }

            if (LED.count > 0)
            {
                // 如果不是无限闪烁（MAX_COUNT），则递减计数
                if (LED.count != MAX_COUNT)
                {
                    LED.count--;
                }

                // 切换LED状态（亮/灭）
                _toggle = !_toggle; // 使用类成员变量
                writeValue(_toggle ? 0 : LED.MaxBrighness);
                // 使用非阻塞延时
                LED.delayTime = LED.activeTime;
            }
        }
        break;

        case STATE_BREATH:
        {
            // 呼吸灯效果：亮度逐渐增加然后逐渐减少
            if (LED.isRising)
            {
                // 亮度递增阶段
                LED.breathBrightness += 1;
                LED.brighness = LED.breathBrightness;
                if (LED.brighness >= LED.MaxBrighness)
                {
                    LED.isRising = false; // 达到最大亮度后开始递减
                }
            }
            else
            {
                // 亮度递减阶段
                if (LED.breathBrightness > 0)
                {
                    LED.breathBrightness--;
                    LED.brighness = LED.breathBrightness;
                }

                if (LED.breathBrightness < 1)
                {
                    LED.isRising = true; // 达到最小亮度后开始递增
                }
            }
            writeValue(LED.brighness);
        }
        break;

        case STATE_BRIGHT: // LED常亮模式
        {
            writeValue(LED.brighness); // 设置LED亮度
            push(LED);                 // 保存当前LED状态
            goto END;                  // 跳转到结束处理
        }
        break;

        case STATE_FADEON: // LED渐亮模式
        {
            uint8_t t_up_brightness = LED.breathBrightness; // 使用实例变量存储渐变亮度
            t_up_brightness++;
            if (t_up_brightness == LED.brighness) // 达到目标亮度时
            {
                LED.breathBrightness = 0;
                LED.status = STATE_NULL;
                goto END;
            }
            writeValue(t_up_brightness);            // 更新LED亮度
            LED.breathBrightness = t_up_brightness; // 保存当前亮度
        }
        break;

        case STATE_FADEOFF: // LED渐灭模式
        {
            writeValue(LED.brighness);

            if (LED.brighness < 1) // 亮度降到最低时
            {
                writeValue(0); // 完全熄灭
                LED.status = STATE_NULL;
                goto END;
            }

            if (LED.brighness > 0)
                LED.brighness--; // 递减亮度值
        }
        break;

        default:
        END:                          // 结束处理
            vTaskSuspend(taskHandle); // 挂起任务，等待新的控制命令
            break;
        }

        // 状态变化检测
        // 如果当前状态为空，尝试恢复到之前保存的状态
        if (LED.status == STATE_NULL)
        {
            pull(LED); // 恢复之前的状态
        }
    }
}

// 根据不同ESP32芯片型号定义LEDC通道配置
#if defined(CONFIG_IDF_TARGET_ESP32)
uint8_t FastDiode::nextAnalogChannel = 0;   // 从通道0开始
const uint8_t FastDiode::MAX_CHANNELS = 16; // ESP32支持16个通道
const uint8_t FastDiode::START_CHANNEL = 0;
#elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
uint8_t FastDiode::nextAnalogChannel = 0;
const uint8_t FastDiode::MAX_CHANNELS = 8; // S2/S3支持8个通道
const uint8_t FastDiode::START_CHANNEL = 0;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
uint8_t FastDiode::nextAnalogChannel = 0;
const uint8_t FastDiode::MAX_CHANNELS = 6; // C3支持6个通道
const uint8_t FastDiode::START_CHANNEL = 0;
#else
#error "Unsupported ESP32 variant"
#endif

// 定义静态成员
uint32_t FastDiode::total_instances = 0;
FastDiode::Config FastDiode::defaultConfig;

// 辅助函数：将状态值转换为可读字符串
const char *FastDiode::getStatusString(EStatus status)
{
    switch (status)
    {
    case STATE_NULL:
        return "NULL";
    case STATE_BRIGHT:
        return "BRIGHT";
    case STATE_FLICKER:
        return "FLICKER";
    case STATE_FADEON:
        return "FADE_ON";
    case STATE_FADEOFF:
        return "FADE_OFF";
    case STATE_BREATH:
        return "BREATH";
    default:
        return "UNKNOWN";
    }
}

// 基础构造函数
// 参数说明:
// pin: LED连接的引脚号
// reverse: LED是否反向控制(true表示低电平点亮)
// name: LED名称标识
FastDiode::FastDiode(uint8_t pin, bool reverse, String name)
{
    _pin = pin;
    _polarity = reverse ? ACTIVE_LOW : ACTIVE_HIGH;
    _name = name;
    _useLEDC = false;
    _initialized = false;
    _channel = nextAnalogChannel++; // 分配一个新通道
    if (nextAnalogChannel >= MAX_CHANNELS)
    {
        nextAnalogChannel = START_CHANNEL;
        Serial.println("Warning: LEDC channel overflow, wrapping around to first channel!");
    }
    _freq = 5000;
    _resolution = 8;
    pinMode(_pin, OUTPUT);
    taskHandle = NULL;
    if (taskHandle == NULL)
    {
        xTaskCreate(this->startTaskImpl, "waitTask", 1024 * 2, this, 1, &taskHandle);
        sendNotify(STATE_BRIGHT, 0, 0, 0, 0);
    }
}

// PWM控制构造函数
// 参数说明:
// pin: LED连接的引脚号
// freq: PWM频率
// channel: PWM通道号
// reverse: LED是否反向控制
// name: LED名称标识
FastDiode::FastDiode(uint8_t pin, uint32_t freq, ELEDChannel channel, bool reverse, String name)
{
    _pin = pin;
    _freq = freq;
    // 检查指定的通道是否有效
    if (channel == CHANNEL_AUTO || channel >= MAX_CHANNELS)
    {
        _channel = nextAnalogChannel++;
        if (nextAnalogChannel >= MAX_CHANNELS)
        {
            nextAnalogChannel = START_CHANNEL;
            Serial.println("Warning: LEDC channel overflow, wrapping around to first channel!");
        }
    }
    else
    {
        _channel = static_cast<uint8_t>(channel);
    }
    _polarity = reverse ? ACTIVE_LOW : ACTIVE_HIGH;
    _name = name;
    _useLEDC = true;
    _initialized = false;
    _resolution = 8;
    taskHandle = NULL;
}

// 初始化LED控制
// 根据是否使用LEDC来进行相应的初始化设置
void FastDiode::begin()
{
    if (_useLEDC && !_initialized)
    {
        ledcSetup(_channel, _freq, _resolution);
        ledcAttachPin(_pin, _channel);
        _initialized = true;

        // 只有 LEDC 模式才在 begin 中创建任务
        if (taskHandle == NULL)
        {
            xTaskCreate(this->startTaskImpl, "waitTask", 1024 * 2, this, 1, &taskHandle);
            // 初始化时设置一个默认的恢复状态
            saveLED.status = STATE_BRIGHT;
            saveLED.brighness = 255;
            saveLED.MaxBrighness = 255;
            sendNotify(STATE_BRIGHT, 255, 0, 0, 0);
        }
    }
}

// 写入LED亮度值
// 参数 value: 目标亮度值(0-255)
void FastDiode::writeValue(uint8_t value)
{
    if (_polarity == ACTIVE_LOW)
    {
        value = 255 - value;
    }

    if (_useLEDC)
    {
        if (!_initialized)
        {
            begin();
        }
        ledcWrite(_channel, value);
    }
    else
    {
        // 对于非 LEDC 模式，直接使用 analogWrite
        analogWrite(_pin, value);
    }
}

void FastDiode::open()
{
    if (_polarity == ACTIVE_HIGH)
        sendNotify(STATE_BRIGHT, 255, 0, 0, 0);
    else
        sendNotify(STATE_BRIGHT, 0, 0, 0, 0);
}

void FastDiode::close()
{
    if (_polarity == ACTIVE_HIGH)
        sendNotify(STATE_BRIGHT, 0, 0, 0, 0);
    else
        sendNotify(STATE_BRIGHT, 255, 0, 0, 0);
}

void FastDiode::setBrightness(uint8_t brightness)
{
    sendNotify(STATE_BRIGHT, brightness, 0, 0, 0);
}

void FastDiode::flickering(uint32_t time, uint32_t count, uint8_t brightness)
{
    // 如果是有限次闪烁，保存当前状态用于恢复
    if (count != MAX_COUNT)
    {
        // 总是保存当前状态，无论是什么状态
        memcpy(&saveLED, &notifyLED, sizeof(Diode));

#ifdef FAST_DIODE_DEBUG
        Serial.printf("[%s] Saving current state before flicker:\n", _name.c_str());
        Serial.printf("  Status: %s, Brightness: %d, DelayTime: %d\n",
                      getStatusString(notifyLED.status), notifyLED.brighness, notifyLED.delayTime);
#endif

        // 根据不同状态进行特殊处理
        switch (saveLED.status)
        {
        case STATE_BREATH:
            // 保存呼吸灯的完整状态
            saveLED.breathBrightness = notifyLED.breathBrightness;
            saveLED.isRising = notifyLED.isRising;
            saveLED.stepping = notifyLED.stepping;
#ifdef FAST_DIODE_DEBUG
            Serial.printf("  Breath params - Brightness: %d, Rising: %d, Stepping: %d\n",
                          saveLED.breathBrightness, saveLED.isRising, saveLED.stepping);
#endif
            break;

        case STATE_FLICKER:
            // 如果当前是无限闪烁，保存完整的闪烁参数
            if (notifyLED.count == MAX_COUNT)
            {
                // 保存原始的闪烁参数
                saveLED.delayTime = notifyLED.delayTime * 2;  // 恢复原始延时
                saveLED.activeTime = notifyLED.delayTime * 2; // 保存原始时间
                saveLED.count = MAX_COUNT;                    // 保持无限闪烁
#ifdef FAST_DIODE_DEBUG
                Serial.printf("  Flicker params - DelayTime: %d, ActiveTime: %d, Count: %d\n",
                              saveLED.delayTime, saveLED.activeTime, saveLED.count);
#endif
            }
            break;
        }
    }
    sendNotify(STATE_FLICKER, brightness, time, 0, count);
}

void FastDiode::fodeOn(uint32_t time, uint8_t brightness)
{
    sendNotify(STATE_FADEON, brightness, 0, time, 0);
}

void FastDiode::fodeOff(uint32_t time, uint8_t brightness)
{
    sendNotify(STATE_FADEOFF, brightness, 0, time, 0);
}

void FastDiode::breathing(uint32_t time, uint8_t brightness)
{
    sendNotify(STATE_BREATH, brightness, 0, time, 0);
}

void FastDiode::push(const Diode &led)
{
    // 保存完整的LED状态
    saveLED = led;
    // 特别保存呼吸灯相关状态
    if (led.status == STATE_BREATH)
    {
        saveLED.breathBrightness = led.breathBrightness;
        saveLED.isRising = led.isRising;
        saveLED.delayTime = led.delayTime;
        saveLED.stepping = led.stepping;
    }
#ifdef FAST_DIODE_DEBUG
    Serial.printf("[%s] Saving state: %s\n", _name.c_str(), getStatusString(led.status));
#endif
}

void FastDiode::pull(Diode &led)
{
    // 深度复制保存的状态
    memcpy(&led, &saveLED, sizeof(Diode));

#ifdef FAST_DIODE_DEBUG
    Serial.printf("[%s] Restoring state details:\n", _name.c_str());
    Serial.printf("  Status: %s, Brightness: %d, DelayTime: %d\n",
                  getStatusString(saveLED.status), saveLED.brighness, saveLED.delayTime);
#endif

    // 直接恢复保存的状态，不重新发送通知
    if (led.status == STATE_BREATH)
    {
        // 确保呼吸灯参数正确恢复
        led.breathBrightness = saveLED.breathBrightness;
        led.isRising = saveLED.isRising;
        led.delayTime = saveLED.delayTime;
        led.stepping = saveLED.stepping;
        led.activeTime = saveLED.activeTime;
        led.MaxBrighness = saveLED.MaxBrighness;
        // 直接写入当前亮度值，保持呼吸灯节奏
        writeValue(led.breathBrightness);
#ifdef FAST_DIODE_DEBUG
        Serial.printf("  Restored breath params - Brightness: %d, Rising: %d, Stepping: %d\n",
                      led.breathBrightness, led.isRising, led.stepping);
#endif
    }
    else if (led.status == STATE_BRIGHT)
    {
        // 对于常亮状态，直接写入亮度值
        writeValue(led.brighness);
#ifdef FAST_DIODE_DEBUG
        Serial.printf("  Restored bright state - Brightness: %d\n", led.brighness);
#endif
    }
    else if (led.status == STATE_FLICKER && led.count == MAX_COUNT)
    {
#ifdef FAST_DIODE_DEBUG
        Serial.printf("  Restoring infinite flicker - DelayTime: %d, ActiveTime: %d\n",
                      led.delayTime, led.activeTime);
#endif
        _toggle = false;
        // 使用保存的原始闪烁参数
        sendNotify(STATE_FLICKER, led.MaxBrighness, led.delayTime, 0, MAX_COUNT);
    }

#ifdef FAST_DIODE_DEBUG
    Serial.printf("[%s] Restoring state from: %s\n", _name.c_str(), getStatusString(saveLED.status));
    Serial.printf("[%s] State restored: %s\n", _name.c_str(), getStatusString(led.status));
#endif
}
