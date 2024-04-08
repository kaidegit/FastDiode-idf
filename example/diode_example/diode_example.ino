#include <Arduino.h>
#include "FastDiode.h"

FastDiode redLED(19);
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("Hello, ESP32!");

    pinMode(12, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
    pinMode(27, INPUT_PULLUP);
    pinMode(26, INPUT_PULLUP);
    pinMode(25, INPUT_PULLUP);
    pinMode(33, INPUT_PULLUP);
    pinMode(32, INPUT_PULLUP);
}

void loop()
{
    if (digitalRead(12) == LOW)
    {
        static bool single = true;
        if (single)
        {
            Serial.println("开灯");
            redLED.open();
            single = !single;
        }
        else
        {
            Serial.println("关灯");
            redLED.close();
            single = !single;
        }
        delay(200);
    }
    if (digitalRead(14) == LOW)
    {
        Serial.println("设置亮度");
        redLED.setBrightness(122);
        delay(100);
    }
    if (digitalRead(27) == LOW)
    {
        Serial.println("一直闪动");
        redLED.flickering(500);
        delay(100);
    }
    if (digitalRead(26) == LOW)
    {
        Serial.println("逐渐变亮");
        redLED.fodeOn(2000);
        delay(100);
    }
    if (digitalRead(25) == LOW)
    {
        Serial.println("逐渐变暗");
        redLED.fodeOff(2000);
        delay(100);
    }
    if (digitalRead(33) == LOW)
    {
        Serial.println("呼吸灯");
        redLED.breathing(500);
        delay(100);
    }
    if (digitalRead(32) == LOW)
    {
        Serial.println("闪动几次后回到闪动前的状态");
        redLED.flickering(500, 2);
        delay(100);
    }
}
