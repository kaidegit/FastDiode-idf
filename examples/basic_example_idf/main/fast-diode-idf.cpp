#include <stdio.h>
#include "FastDiode.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define LED_D4 GPIO_NUM_12
#define LED_D5 GPIO_NUM_13

static const char *const TAG = "FastDiode";

FastDiode led(LED_D4, EPinPolarity::ACTIVE_LOW, "led");

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting FastDiode");
    led.init(LEDC_CHANNEL_0);
    while (1) {
        // 1. 开关测试
        ESP_LOGI(TAG, "1. 开关测试");
        led.open(); // 打开
        vTaskDelay(pdMS_TO_TICKS(1000));
        led.close(); // 关闭
        vTaskDelay(pdMS_TO_TICKS(1000));

        // 2. 亮度渐变
        ESP_LOGI(TAG, "2. 亮度测试");
        led.setBrightness(50); // 20%亮度
        vTaskDelay(pdMS_TO_TICKS(1000));
        led.setBrightness(125); // 50%亮度
        vTaskDelay(pdMS_TO_TICKS(1000));
        led.setBrightness(255); // 100%亮度
        vTaskDelay(pdMS_TO_TICKS(1000));

        // 3. 渐亮效果
        ESP_LOGI(TAG, "3. 渐亮效果");
        led.fodeOn(2000); // 2秒内渐亮
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 4. 渐暗效果
        ESP_LOGI(TAG, "4. 渐暗效果");
        led.fodeOff(2000); // 2秒内渐暗
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 6. 呼吸灯效果
        ESP_LOGI(TAG, "6. 呼吸灯效果");
        led.breathing(500); // 呼吸周期1秒
        vTaskDelay(pdMS_TO_TICKS(5000));

        // 5. 闪烁效果
        ESP_LOGI(TAG, "5. 闪烁效果 闪烁3次后重新回到呼吸灯效果");
        led.flickering(200, 3); // 闪烁3次后重新回到呼吸灯效果
        vTaskDelay(pdMS_TO_TICKS(3000));

        // 暂停一下，准备开始下一轮演示
        led.close();
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "------- 重新开始演示 -------\n");
    }
}
