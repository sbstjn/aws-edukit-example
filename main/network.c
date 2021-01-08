#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "core2forAWS.h"
#include "display.h"
#include "cryptoauthlib.h"
#include "atecc608.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event */
const int CONNECTED_BIT = BIT0;
const int DISCONNECTED_BIT = BIT1;

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");
/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupClearBits(wifi_event_group, DISCONNECTED_BIT);
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        display_wifi_label_update(true);

        char buffer[36];
        sprintf(buffer, "Network: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));

        display_textarea_add(buffer, NULL, NULL);

        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        display_textarea_add("Network connection failed.\nRetrying...\n", NULL, NULL);

        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        xEventGroupSetBits(wifi_event_group, DISCONNECTED_BIT);
        display_wifi_label_update(false);

        break;
    default:
        break;
    }

    return ESP_OK;
}

void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };

    printf("Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData)
{
    printf("\n");
    printf("New message from topic: %.*s\n", topicNameLen, topicName);
    printf("\n");
    printf("%.*s", (int)params->payloadLen, (char *)params->payload);
    printf("\n");
}

void disconnect_callback_handler(AWS_IoT_Client *pClient, void *data)
{
    printf("MQTT Disconnect");

    if (NULL == pClient)
    {
        return;
    }

    if (aws_iot_is_autoreconnect_enabled(pClient))
    {
        printf("Auto Reconnect is enabled, Reconnecting attempt will start now");
    }
    else
    {
        printf("Auto Reconnect not enabled. Starting manual reconnect...");
        IoT_Error_t rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if (NETWORK_RECONNECTED == rc)
        {
            printf("Manual Reconnect Successful");
        }
        else
        {
            printf("Manual Reconnect Failed - %d", rc);
        }
    }
}

void aws_iot_task(void *param)
{
    char cPayload[100];

    int32_t i = 0;

    IoT_Error_t rc = FAILURE;

    AWS_IoT_Client client;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    IoT_Publish_Message_Params paramsQOS0;
    IoT_Publish_Message_Params paramsQOS1;

    mqttInitParams.enableAutoReconnect = false;
    mqttInitParams.pHostURL = HostAddress;
    mqttInitParams.port = port;
    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = "#";
    mqttInitParams.pDevicePrivateKeyLocation = "#0";

    char *clientId = malloc(ATCA_SERIAL_NUM_SIZE * 2 + 1);
    ATCA_STATUS ret = Atecc608_GetSerialString(clientId);
    if (ret != ATCA_SUCCESS)
    {
        printf("Failed to get device serial from secure element. Error: %i", ret);
        abort();
    }

    const int clientIdLength = strlen(clientId);
    char subTopic[clientIdLength + 2];
    char pubTopic[clientIdLength + 1];
    sprintf(subTopic, "%s/#", clientId);
    sprintf(pubTopic, "%s/", clientId);

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnect_callback_handler;
    mqttInitParams.disconnectHandlerData = NULL;

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if (SUCCESS != rc)
    {
        printf("aws_iot_mqtt_init returned error : %d ", rc);
        abort();
    }

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;

    connectParams.pClientID = clientId;
    connectParams.clientIDLen = clientIdLength;
    connectParams.isWillMsgPresent = false;

    printf("Connecting to AWS IoT Core...\n");
    do
    {
        rc = aws_iot_mqtt_connect(&client, &connectParams);
        if (SUCCESS != rc)
        {
            printf("Error(%d) connecting to %s:%d\n", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            printf("Retrying in 2 seconds.");
            vTaskDelay(2000 / portTICK_RATE_MS);
        }
    } while (SUCCESS != rc);

    display_textarea_add("Connected to AWS IoT Core!\n", NULL, NULL);
    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if (SUCCESS != rc)
    {
        printf("Unable to set Auto Reconnect to true - %d", rc);
        abort();
    }

    const int subTopicLength = strlen(subTopic);
    rc = aws_iot_mqtt_subscribe(&client, subTopic, subTopicLength, QOS0, iot_subscribe_callback_handler, NULL);
    if (SUCCESS != rc)
    {
        printf("Error subscribing : %d ", rc);
        abort();
    }

    paramsQOS0.qos = QOS0;
    paramsQOS0.payload = (void *)cPayload;
    paramsQOS0.isRetained = 0;

    paramsQOS1.qos = QOS1;
    paramsQOS1.payload = (void *)cPayload;
    paramsQOS1.isRetained = 0;

    const int pubTopicLength = strlen(pubTopic);
    while ((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc))
    {
        rc = aws_iot_mqtt_yield(&client, 100);

        if (NETWORK_ATTEMPTING_RECONNECT == rc)
        {
            continue;
        }

        vTaskDelay(1000 / portTICK_RATE_MS);

        // QOS0
        //
        // Publish and ignore if "ack" was received or from AWS IoT Core
        sprintf(cPayload, "{\"message\": \"Moin!\", \"type\": \"QOS0\", \"counter\": %d}", i++);
        paramsQOS0.payloadLen = strlen(cPayload);
        rc = aws_iot_mqtt_publish(&client, pubTopic, pubTopicLength, &paramsQOS0);
        if (rc != SUCCESS)
        {
            printf("Publish QOS0 error %i", rc);
            rc = SUCCESS;
        }

        // QOS1
        //
        // Publish and check if "ack" was sent from AWS IoT Core
        sprintf(cPayload, "{\"message\": \"Moin!\", \"type\": \"QOS1\", \"counter\": %d}", i++);
        paramsQOS1.payloadLen = strlen(cPayload);
        rc = aws_iot_mqtt_publish(&client, pubTopic, pubTopicLength, &paramsQOS1);
        if (rc == MQTT_REQUEST_TIMEOUT_ERROR)
        {
            printf("QOS1 publish ack not received.");
            rc = SUCCESS;
        }
    }

    printf("An error occurred in the main loop.");
    abort();
}