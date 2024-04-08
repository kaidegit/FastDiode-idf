#include "FastDiode.h"

bool FastDiode::waitForNotify(Diode &led)
{
    uint32_t value;
    if (pdTRUE == xTaskGenericNotifyWait(0, 0, 0, &value, led.delayTime))
    {
        // 将接收到的数据放入传入的 led 中
        led = *(Diode *)value;
        return 1;
    }
    return 0;
}

bool FastDiode::sendNotify(EStatus _status, uint8_t _brighness, uint32_t _delayTime, uint32_t _activeTime, uint32_t _count)
{
    vTaskResume(taskHandle);
    notifyLED.status = _status;
    notifyLED.MaxBrighness = _brighness;

    if (notifyLED.status == STATE_FADEON || notifyLED.status == STATE_FADEOFF || notifyLED.status == STATE_BREATH)
    {
        if (_activeTime < 255)
            _activeTime = 255;
        // 亮度变化每次只能+1 , 因此要在 单位时间 _activeTime 内达到亮度 _brighness, 每次延时的时间就是
        notifyLED.delayTime = _activeTime / _brighness;
        notifyLED.stepping = notifyLED.delayTime;
    }
    else
        notifyLED.delayTime = _delayTime;

    notifyLED.activeTime = _activeTime;
    notifyLED.brighness = _brighness;

    if (_count != 1)
        notifyLED.count = _count * 2;
    else
        notifyLED.count = _count;

    if (xTaskGenericNotify(taskHandle, 0, (uint32_t)&notifyLED, eSetValueWithOverwrite, NULL) == pdPASS)
        return 1;
    else
        return 0;
}

void FastDiode::task()
{
    Diode LED;
    bool toggle = false;

    while (1)
    {
        vTaskDelay(1);
        // 获取通知
        this->waitForNotify(LED);

    START: // 在等待通知的时间段内，如果收到通知，回调到程序一开始的位置，从而跳出延时
        switch (LED.status)
        {

        case STATE_BRIGHT: // 设定亮度
        {
            // Serial.printf("%s  set Brighness : %d \n", name, LED.brighness);
            analogWrite(pin, LED.brighness);
            push(LED); // 保存状态
            goto END;
        }
        break;

        case STATE_FLICKER: // 闪动
        {
            // Serial.printf("%s  flciker, count:%d \n", name, LED.count);
            //  结束后的退出条件
            if (LED.count == 0)
                pull(LED); // 还原到上一个状态状态

            if (LED.count > 0)
            {
                // 如果传入的是默认的最大值, 就不进行递减, 将一直进行闪灯
                if (LED.count < MAX_COUNT)
                    LED.count--;
                toggle = !toggle;
                if (toggle)
                    analogWrite(pin, 0);
                if (!toggle)
                    analogWrite(pin, LED.MaxBrighness);
            }
        }
        break;

        case STATE_FADEON: // 逐渐亮起
        {
            //// Serial.printf("%s gradually brighten to %d within %dms \n", name, LED.brighness, LED.activeTime);
            static uint8_t t_up_brightness = 0;
            t_up_brightness++;
            if (t_up_brightness == LED.brighness)
            {
                t_up_brightness = 0;
                LED.status = STATE_NULL;
                goto END;
            }
            analogWrite(pin, t_up_brightness);
            // Serial.println(t_up_brightness);
        }
        break;

        case STATE_FADEOFF: // 逐渐熄灭
        {
            analogWrite(pin, LED.brighness);
            // Serial.println(LED.brighness);

            if (LED.brighness < 1)
            {
                analogWrite(pin, 0);
                LED.status = STATE_NULL;
                goto END;
            }

            if (LED.brighness > 0)
                LED.brighness--;
        }
        break;

        case STATE_BREATH: // 呼吸灯
        {
            push(LED);             // 保存状态
            if (LED.edge == false) // 上升沿
            {
                LED.brighness += 1;
                if (LED.brighness >= LED.MaxBrighness)
                {
                    LED.edge = true;
                }
            }
            else if (LED.edge == true) // 下降沿沿
            {
                static uint8_t down = 0;
                if (LED.brighness > 0)
                    LED.brighness--;

                if (LED.brighness < 1)
                {
                    //   down++;
                    //   if (down >= 100)
                    //   {
                    //     down = 0;
                    LED.edge = false;
                    //   }
                }
            }
            analogWrite(pin, LED.brighness);
            // Serial.println(LED.brighness);
        }
        break;
        default:
        END:
            // Serial.printf("%s  Suspend! \n", name);
            vTaskSuspend(taskHandle);
            break;
        }
    }
}
