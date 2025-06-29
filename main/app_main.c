/**
 * @file app_main.c
 * @author Denilson da Silva Pereira
 * @date 28 de Junho de 2025
 * @brief Cliente MQTT no ESP32 para controlar um LED via Wi-Fi.
 *
 * Este projeto implementa um cliente MQTT que se conecta a um broker,
 * e se inscreve no tópico específico na atividade /ifpe/ads/embarcados/esp32/led e controla o estado do LED
 * embutido na placa ESP no GPIO 2 com base nas mensagens recebidas do cliente inscrito no tópico ("1" para ligar, "0" para desligar).
 */


#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

// Aqui definimos o pino do esp para acender o led, na minha placa é o GPIO 2
#define LED_GPIO GPIO_NUM_2
// Aqui definimos o tópico MQTT para controlar o led, da forma que o professor solicitou
#define MQTT_TOPIC_LED "/ifpe/ads/embarcados/esp32/led"

static const char *TAG = "MQTT5_LED_CONTROL";

// Essa função auxilia imprimir as propriedades do usuário do MQTT 5
static void print_user_property(mqtt5_user_property_handle_t user_property)
{
    if (user_property) {
        uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
        if (count) {
            ESP_LOGI(TAG, "  User Properties:");
            esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
            if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK) {
                for (int i = 0; i < count; i++) {
                    esp_mqtt5_user_property_item_t *t = &item[i];
                    ESP_LOGI(TAG, "    key: %s, value: %s", t->key, t->value);
                    free((char *)t->key);
                    free((char *)t->value);
                }
            }
            free(item);
        }
    }
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registrado para receber eventos MQTT
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // Imprime propriedades recebidas do Broker no pacote CONNACK
        print_user_property(event->property->user_property);
        
        msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_LED, 1);
        ESP_LOGI(TAG, "Inscrito no tópico [%s], msg_id=%d", MQTT_TOPIC_LED, msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        ESP_LOGI(TAG, "Inscrição bem-sucedida! Aguardando mensagens...");
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPICO: %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DADO: %.*s", event->data_len, event->data);

        ESP_LOGI(TAG, "--- Propriedades da Mensagem Recebida ---");
        print_user_property(event->property->user_property);
        if (event->property->correlation_data_len > 0) {
             ESP_LOGI(TAG, "  Correlation Data: %.*s", event->property->correlation_data_len, event->property->correlation_data);
        }

        // Esse if verifica se a mensagem recebida é no tópico do LED, se sim ele entra para os if internos
        if (strncmp(event->topic, MQTT_TOPIC_LED, event->topic_len) == 0) {
            //Aqui verifica se a informação da mensagem recebida no tópico é 1, 0 ou outra coisa. Se a informação for 0 ou 1 que são válidas apaga ou acende o led. Caso não seja nenhuma dessas ele manda a mensagem pro monitor do ESP_IDF informando 'Comando desconhecido para o LED'
            if (strncmp(event->data, "1", event->data_len) == 0) {
                gpio_set_level(LED_GPIO, 1);
                ESP_LOGI(TAG, "LED LIGADO");
            } else if (strncmp(event->data, "0", event->data_len) == 0) {
                gpio_set_level(LED_GPIO, 0);
                ESP_LOGI(TAG, "LED DESLIGADO");
            } else {
                ESP_LOGW(TAG, "Comando desconhecido para o LED: '%.*s'", event->data_len, event->data);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    // Configurando User Properties para enviar na conexão
    esp_mqtt5_user_property_item_t user_props[] = {
        {"board", "esp32"},
        {"project", "ifpe-embarcados-led"}
    };
    #define USER_PROPS_SIZE sizeof(user_props)/sizeof(esp_mqtt5_user_property_item_t)

    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .user_property = NULL, 
    };
    
    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);

    esp_mqtt5_client_set_user_property(&connect_property.user_property, user_props, USER_PROPS_SIZE);
    // Define as propriedades de conexão no cliente
    esp_mqtt5_client_set_connect_property(client, &connect_property);
    // Libera a memória que foi alocada para as user properties (o cliente já tem sua cópia)
    esp_mqtt5_client_delete_user_property(connect_property.user_property);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    //Fazemos a configuração do GPIO do led aqui, no gpio_set_level, o 0 é para o led iniciar apagado
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    mqtt_app_start();
}