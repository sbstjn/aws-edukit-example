#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "core2forAWS.h"
#include "ft6336u.h"
#include "mpu6886.h"
#include "cryptoauthlib.h"
#include "atecc608.h"

#include "display.h"
#include "network.h"

// in disp_spi.c
extern SemaphoreHandle_t xGuiSemaphore;
extern SemaphoreHandle_t spi_mutex;

#define CLIENT_ID_LEN (ATCA_SERIAL_NUM_SIZE * 2 + 1)

void display_clientId(void)
{
       char *clientId = malloc(CLIENT_ID_LEN);
       ATCA_STATUS ret = Atecc608_GetSerialString(clientId);
       if (ret != ATCA_SUCCESS)
       {
              printf("Failed to get device serial from secure element. Error: %i", ret);
              abort();
       }

       display_textarea_add("DeviceId: %s\n", clientId, (size_t *)CLIENT_ID_LEN);
}

void initialize_crypto(void)
{
       ATCA_STATUS ret = Atecc608_Init();
       if (ret != ATCA_SUCCESS)
       {
              printf("ATECC608 secure element initialization error");
              abort();
       }
}

void initialize_AWS(void)
{
       spi_mutex = xSemaphoreCreateMutex();
       Core2ForAWS_Init();
       MPU6886_Init();
       FT6336U_Init();
       Core2ForAWS_LCD_Init();
       Core2ForAWS_Button_Init();
       Core2ForAWS_Sk6812_Init();
       Core2ForAWS_Sk6812_Clear();
       Core2ForAWS_Sk6812_Show();
       Core2ForAWS_LCD_SetBrightness(80);
       Core2ForAWS_LED_Enable(0x01);
}

void show_hardware_info(void)
{
       esp_chip_info_t chip_info;
       esp_chip_info(&chip_info);

       printf("\n");

       printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
              CONFIG_IDF_TARGET,
              chip_info.cores,
              (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
              (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

       printf("silicon revision %d, ", chip_info.revision);

       printf("%dMB %s flash\n",
              spi_flash_get_chip_size() / (1024 * 1024),
              (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

       printf("Minimum free heap size: %d bytes", esp_get_minimum_free_heap_size());
       printf("\n\n");
}

void initialize_NVS(void)
{
       esp_err_t err = nvs_flash_init();
       if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
       {
              ESP_ERROR_CHECK(nvs_flash_erase());
              err = nvs_flash_init();
       }
       ESP_ERROR_CHECK(err);
}

void connect_IOT(void)
{
       xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", 9216, NULL, 5, NULL, 1);
}

void app_main(void)
{
       show_hardware_info();

       initialize_NVS();
       initialize_AWS();

       display_init();

       initialise_wifi();
       initialize_crypto();

       display_clientId();

       connect_IOT();
}