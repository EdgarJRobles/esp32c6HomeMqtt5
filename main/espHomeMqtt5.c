#include "esp_err.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <protocol_examples_common.h>
#include <mqtt_client.h>
/*
 * Home assistant support through MQTT 5.0 protocol to connect esp32 c6.
*/

#define GPIO_OUTPUT_IO_0    CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_IO_1    CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
/*
 * Let's say, GPIO_OUTPUT_IO_0=18, GPIO_OUTPUT_IO_1=19
 * In binary representation,
 * 1ULL<<GPIO_OUTPUT_IO_0 is equal to 0000000000000000000001000000000000000000 and
 * 1ULL<<GPIO_OUTPUT_IO_1 is equal to 0000000000000000000010000000000000000000
 * GPIO_OUTPUT_PIN_SEL                0000000000000000000011000000000000000000
*/
#define GPIO_INPUT_IO_0     CONFIG_GPIO_INPUT_0
#define GPIO_INPUT_IO_1     CONFIG_GPIO_INPUT_1
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
/*
 * Let's say, GPIO_INPUT_IO_0=4, GPIO_INPUT_IO_1=5
 * In binary representation,
 * 1ULL<<GPIO_INPUT_IO_0 is equal to 0000000000000000000000000000000000010000 and
 * 1ULL<<GPIO_INPUT_IO_1 is equal to 0000000000000000000000000000000000100000
 * GPIO_INPUT_PIN_SEL                0000000000000000000000000000000000110000
 */
#define ESP_INTR_FLAG_DEFAULT 0
#define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)
static const char *TAG = "MQTT5_EXAMPLE";
bool isconnected=false;
bool ispressed=false;
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	uint32_t gpio_num = (uint32_t) arg;
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
	uint32_t io_num;
	for(;;) 
	{
		if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)&&isconnected) 
		{
			printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
			ispressed=true;
		}
	}
}

static esp_mqtt5_publish_property_config_t publish_property = {
	.payload_format_indicator = 1,
	.message_expiry_interval = 1000,
	.topic_alias = 0,
	.response_topic = "/topic/test/response",
	.correlation_data = "123456",
	.correlation_data_len = 6,
};

static esp_mqtt5_subscribe_property_config_t subscribe_property = {
	.subscribe_id = 25555,
	.no_local_flag = false,
	.retain_as_published_flag = false,
	.retain_handle = 0,
	.is_share_subscribe = true,
	.share_name = "group1",
};


static esp_mqtt5_user_property_item_t user_property_arr[] = {
        {"board", "esp32"},
        {"u", "mqtt_user"},
        {"p", "mqttpass"}
};
static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		isconnected=true;
		//esp_mqtt5_client_set_user_property(&publish_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
		//esp_mqtt5_client_set_publish_property(client, &publish_property);
		//msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 1);
		//esp_mqtt5_client_delete_user_property(publish_property.user_property);
		//publish_property.user_property = NULL;
		//ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        //print_user_property(event->property->user_property);

        esp_mqtt5_client_set_user_property(&subscribe_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        esp_mqtt5_client_set_subscribe_property(client, &subscribe_property);
        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        esp_mqtt5_client_delete_user_property(subscribe_property.user_property);
        subscribe_property.user_property = NULL;
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        //esp_mqtt5_client_set_user_property(&subscribe1_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        //esp_mqtt5_client_set_subscribe_property(client, &subscribe1_property);
        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 2);
        //esp_mqtt5_client_delete_user_property(subscribe1_property.user_property);
        //subscribe1_property.user_property = NULL;
        //ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        //esp_mqtt5_client_set_user_property(&unsubscribe_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        //esp_mqtt5_client_set_unsubscribe_property(client, &unsubscribe_property);
        //msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos0");
        //ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        //esp_mqtt5_client_delete_user_property(unsubscribe_property.user_property);
        //unsubscribe_property.user_property = NULL;
        break;
    case MQTT_EVENT_DISCONNECTED:
        //ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        //print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //print_user_property(event->property->user_property);
        //esp_mqtt5_client_set_publish_property(client, &publish_property);
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        //print_user_property(event->property->user_property);
        //esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        //esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
        //esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
        //disconnect_property.user_property = NULL;
        esp_mqtt_client_disconnect(client);
        break;
    case MQTT_EVENT_PUBLISHED:
        //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        //print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        //print_user_property(event->property->user_property);
       // ESP_LOGI(TAG, "payload_format_indicator is %d", event->property->payload_format_indicator);
       // ESP_LOGI(TAG, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
       // ESP_LOGI(TAG, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
       // ESP_LOGI(TAG, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
        //ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
		//ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
		int topicres11 = 100;
		topicres11 = strcmp("testonn",event->data);
		int topicres1 = 100;
		int topicres3 = 100;
		topicres1 = strcmp("testoff",event->data);
		topicres3 = strcmp("teston",event->data);
        ESP_LOGI(TAG, "RES=%.*d", event->data_len, topicres11);
        ESP_LOGI(TAG, "RES=%.*d", event->data_len, topicres1);
        ESP_LOGI(TAG, "RES=%.*d", event->data_len, topicres3);
		if (topicres11==0)
		{
			ESP_LOGI(TAG, "IFDATA=%.*s", event->data_len, event->data);
			gpio_set_level(GPIO_OUTPUT_IO_0, 1);
		}
		else if (topicres1==0)
		{
			ESP_LOGI(TAG, "IFDATA=%.*s", event->data_len, event->data);
			gpio_set_level(GPIO_OUTPUT_IO_0, 0);		
		}
		else if (topicres3==0)
		{
			ESP_LOGI(TAG, "IFDATA3=%.*s", event->data_len, event->data);
			gpio_set_level(GPIO_OUTPUT_IO_0, 1);
		}
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        //print_user_property(event->property->user_property);
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        //    log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        //    log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        //    log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
        //    ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt5_app_start(void)
{
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = "/test/response",
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .broker.address.hostname = "192.168.1.71",
        .broker.address.port = 1883,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = true,
        .credentials.username = "mqtt_user",
        .credentials.authentication.password = "mqttpass",
        .session.last_will.topic = "/topic/will",
        .session.last_will.msg = "i will leave",
        .session.last_will.msg_len = 12,
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
    };

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt5_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt5_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);

    /* Set connection properties and user properties */
    esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_connect_property(client, &connect_property);

    /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
     * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
     */
    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
	//zero-initialize the config structure.
	gpio_config_t io_conf = {};
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
	//change gpio interrupt type for one pin
	gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);
	//create a queue to handle gpio event from isr
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	//start gpio task
	xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
	//install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);
	//remove isr handler for gpio number.
	//gpio_isr_handler_remove(GPIO_INPUT_IO_0);
	//hook isr handler for specific gpio pin again
	//gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(example_connect());
    mqtt5_app_start();
    //int cnt = 0;
//    while(1) 
//	{
		//cnt++;
        //printf("cnt: %d\n", cnt);
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        //gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
//    }
}
