# FastDiode

#### 介绍
用于单色LED，可以简单实现多个灯点亮，关闭，呼吸，闪烁等灯效。适用ESP32，ESP32S2，ESP32S3、ESP32C3等系列

#### 软件架构
Arduino

#### 安装教程

1.  引用头文件 
    #include "FastDiode.h"

2.  实例化对象 
    FastDiode redLED(19);

3.  调用功能
    redLED.open(); // 开灯
    redLED.close(); // 关灯
    redLED.setBrightness(122); // 设置亮度
    redLED.flickering(500); // 一直闪烁，隔500ms闪烁
    redLED.flickering(500, 2); // 隔500ms闪烁,闪动2次后回到闪动前的状态
    redLED.fodeOn(2000); // 2000ms内逐渐变亮
    redLED.fodeOff(2000); // 2000ms内逐渐变暗
    redLED.breathing(500); // 呼吸灯



#### 使用说明

1.  使用 freertos 与 analogWrite() 实现，
2.  不支持esp8266，支持esp32，S2，C3，S3等版本
3.  可以点亮的灯数取决于不同ESP芯片的PWM通道，比如ESP32支持16路，C3支持6路，但是并未进行最大数量测试。