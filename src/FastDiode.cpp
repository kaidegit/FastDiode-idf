#include "FastDiode.h"

// 等待任务通知函数，用于接收新的 LED 控制命令,
// 参数：led - 用于存储接收到的 LED 控制参数
// 返回：true - 接收到通知，false - 超时未接收到通知
bool FastDiode::waitForNotify(LEDState &led)
{
    uint32_t value;
    // 等待通知，等待时间由 led.stepInterval 决定
    if (pdTRUE == xTaskGenericNotifyWait(0,                 // 进入时要清除的位（此处不清除）
                                         0,                 // 退出时要清除的位（此处不清除）
                                         0,                 // 用于接收通知值的指针
                                         &value,            // 存储接收到的通知值
                                         led.stepInterval)) // 最大等待时间
    {
        // 将接收到的数据转换为 LEDState 结构体并存储
        led = *(LEDState *)value;
        return 1;
    }
    return 0;
}

// 发送任务通知函数，用于更新 LED 的控制参数
// 参数：
//   _status: LED 状态（开关、渐变等）
//   _currentBrightness: 目标亮度值
//   _stepInterval: 延时时间
//   _totalDuration: 动作持续时间
//   repeatCount: 重复次数
bool FastDiode::sendNotify(EEffectType _status, uint8_t _targetBrightness, uint32_t _stepInterval, uint32_t _totalDuration, uint32_t _repeatCount)
{
    vTaskResume(taskHandle); // 恢复任务运行

    // 开始针对每个参数进行处理
    /************************************************
                    设置 LED 状态
     *************************************************/
    notifyLED.status = _status;

    /************************************************
                    设置 LED 最大亮度
    *************************************************/
    notifyLED.targetBrightness = _targetBrightness;

    /************************************************
                    设置 LED 步进时间
    *************************************************/
    // 1.对于渐变效果（渐亮、渐暗、呼吸灯）的情况，步进时间要根据总时间和亮度范围计算
    if (notifyLED.status == EEffectType::FADE_IN       // 渐亮
        || notifyLED.status == EEffectType::FADE_OUT   // 渐暗
        || notifyLED.status == EEffectType::BREATHING) // 呼吸灯
    {
        // 如果总时间小于255ms，则设置为255ms
        // 确保最小动作时间不小于 255ms，如果时间太短，则每次的变化值会很小，导致灯效不明显
        _totalDuration = _totalDuration < 255 ? 255 : _totalDuration;

        notifyLED.stepInterval = _totalDuration / _targetBrightness; // 根据总时间和亮度范围计算来计算每次延时多久
        notifyLED.stepping = notifyLED.stepInterval;                 // 每步的步进值,即每次变化的值
    }
    // 2.对于闪烁效果，直接使用步进时间，其他灯效此参数一般为0
    else
        notifyLED.stepInterval = _stepInterval;

    /************************************************
                    设置 LED 总时间
    *************************************************/
    // 设定亮度的totalDuration 为 0 表示一直保持
    // 闪动灯的 totalDuration 为0，因为持续时间由闪动次数来决定
    notifyLED.totalDuration = _totalDuration;

    /************************************************
                    设置 LED 当前亮度
    *************************************************/
    // 渐暗效果，当前亮度为最大亮度
    if (notifyLED.status == EEffectType::FADE_OUT)
        notifyLED.currentBrightness = _targetBrightness;

    // 渐亮效果，当前亮度为0
    if (notifyLED.status == EEffectType::FADE_IN)
        notifyLED.currentBrightness = 0;

    /************************************************
                    设置 LED 重复次数
    *************************************************/
    // 闪烁次数处理，乘 2 是因为开和关各算一次
    notifyLED.repeatCount = _repeatCount * 2;
    // if (_repeatCount != 1)
    //     notifyLED.repeatCount = _repeatCount * 2;
    // else
    //     notifyLED.repeatCount = _repeatCount;

    /************************************************
                    发送通知给 LED 控制任务
    *************************************************/
    if (xTaskGenericNotify(taskHandle, 0, (uint32_t)&notifyLED, eSetValueWithOverwrite, NULL) == pdPASS)
        return 1;
    else
        return 0;
}

// LED 控制任务的主函数
void FastDiode::task()
{
    LEDState LED;
    bool toggle = false; // 用于闪烁效果的开关切换

    while (1)
    {
        vTaskDelay(1);
        // 等待新的控制命令
        this->waitForNotify(LED);

    START: // 如果在等待期间收到新通知，跳转到这里重新开始处理
        switch (LED.status)
        {
        case EEffectType::STATIC: // 设置固定亮度
        {
            setBrightnessImpl(LED.targetBrightness);
            push(LED); // 保存当前状态
            goto END;
        }
        break;

        case EEffectType::BLINK: // 闪烁效果
        {
            // 闪烁次数用完后退出
            if (LED.repeatCount == 0)
                pull(LED); // 恢复到上一个状态

            if (LED.repeatCount > 0)
            {
                // 如果不是最大计数值，则递减计数
                if (LED.repeatCount < MAX_COUNT)
                    LED.repeatCount--;
                toggle = !toggle;
                if (toggle)
                    setBrightnessImpl(0); // 关闭
                if (!toggle)
                    setBrightnessImpl(LED.targetBrightness); // 最大亮度
            }
        }
        break;

        case EEffectType::FADE_IN: // 渐亮效果
        {
            // 达到目标亮度后结束
            if (LED.currentBrightness >= LED.targetBrightness)
            {
                setBrightnessImpl(LED.targetBrightness);
                LED.currentBrightness = 0;
                LED.status = EEffectType::NONE;
                goto END;
            }
            setBrightnessImpl(LED.currentBrightness);
            LED.currentBrightness++;
        }
        break;

        case EEffectType::FADE_OUT: // 渐暗效果
        {
            setBrightnessImpl(LED.currentBrightness);

            // 完全熄灭后结束
            if (LED.currentBrightness < 1)
            {
                setBrightnessImpl(0);
                LED.status = EEffectType::NONE;
                goto END;
            }

            if (LED.currentBrightness > 0)
                LED.currentBrightness--;
        }
        break;

        case EEffectType::BREATHING: // 呼吸灯效果
        {
            push(LED);                                      // 保存状态
            if (LED.direction == EBreathDirection::FADE_IN) // 亮度上升阶段
            {
                LED.currentBrightness += 1;
                if (LED.currentBrightness >= LED.targetBrightness)
                {
                    LED.direction = EBreathDirection::FADE_OUT; // 切换到下降阶段
                }
            }
            else if (LED.direction == EBreathDirection::FADE_OUT) // 亮度下降阶段
            {
                static uint8_t down = 0;
                if (LED.currentBrightness > 0)
                    LED.currentBrightness--;

                if (LED.currentBrightness < 1)
                {
                    LED.direction = EBreathDirection::FADE_IN; // 切换到上升阶段
                }
            }
            setBrightnessImpl(LED.currentBrightness);
        }
        break;

        default:
        END:
            // 暂停任务，等待新的控制命令
            vTaskSuspend(taskHandle);
            break;
        }
    }
}
