#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/adc.h"
#include "i2c.h"
#include "esp_timer.h"

// defines for I2C
#define sda_gpio GPIO_NUM_19
#define scl_gpio GPIO_NUM_18

#define I2C_FREQ_HZ (100000) // 100kHz
#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_BUFSIZE 0
#define I2C_TIMEOUT_MS 1000
#define I2C_SLV_BUFSIZE 0

#define STEMMA_ADDR 0x36
#define STEMMA_BASE_ADDR 0x0F
#define STEMMA_F_REG 0x10
#define STEMMA_RBUF_SIZE 2

#define AM2320_I2C_ADDR 0x5C
#define AM2320_READ_CMD 0x03
#define AM2320_HUM_H 0x00
#define AM2320_HUM_L 0x01
#define AM2320_TEMP_H 0x02
#define AM2320_TEMP_L 0x03

#define AM2320_DELAY_T1 (2) // ms
#define AM2320_DELAY_T2 (3) // ms

// defines for adc read
#define PHOTOCELL_PIN GPIO_NUM_2 // Replace with the actual GPIO pin where your photocell is connected

// led gpio pins
#define green_led_gpio GPIO_NUM_6
#define red_led_gpio GPIO_NUM_7
#define blue_led_gpio GPIO_NUM_9
#define BUTTON_PIN GPIO_NUM_0

// led section

int led_on(int soil_status)
{
    switch (soil_status)
    {
    case 0:
        gpio_set_level(blue_led_gpio, false);
        gpio_set_level(red_led_gpio, false);
        gpio_set_level(green_led_gpio, false);
        break;
    case 1:
        gpio_set_level(blue_led_gpio, true);
        gpio_set_level(red_led_gpio, false);
        gpio_set_level(green_led_gpio, true);
        break;
    case 2:
        gpio_set_level(blue_led_gpio, true);
        gpio_set_level(red_led_gpio, false);
        gpio_set_level(green_led_gpio, false);
        break;
    case 3:
        gpio_set_level(blue_led_gpio, true);
        gpio_set_level(red_led_gpio, true);
        gpio_set_level(green_led_gpio, false);
        break;
    case 4:
        gpio_set_level(blue_led_gpio, true);
        gpio_set_level(red_led_gpio, false);
        gpio_set_level(green_led_gpio, true);
        break;

    default:
        break;
    }
    return 0;
}

void led_off(void)
{
    gpio_set_level(blue_led_gpio, true);
    gpio_set_level(red_led_gpio, true);
    gpio_set_level(green_led_gpio, true);
}

/***************************************************************
 *                     I2C COMMUNICATION SECTION
 ***************************************************************/

i2c_port_t setup_i2c(int sda_pin, int scl_pin)
{
    esp_err_t err;

    int i2c_master_port = I2C_MASTER_PORT;
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ};

    err = i2c_param_config(i2c_master_port, &i2c_conf);
    if (err != ESP_OK)
    {
        printf("ERROR: Not able to set up i2c soil sensor: %x\n", err);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        return -1;
    }

    err = i2c_driver_install(i2c_master_port, I2C_MODE_MASTER, I2C_SLV_BUFSIZE, I2C_SLV_BUFSIZE, 0);
    if (err != ESP_OK)
    {
        printf("ERROR: i2c driver install not ok: %x\n", err);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        return -1;
    }
    printf("I2C port was set up\n");
    return i2c_master_port;
}

unsigned short read_soil_sensor(i2c_port_t port)
{
    esp_err_t err;
    uint8_t wbuf[2] = {STEMMA_BASE_ADDR, STEMMA_F_REG};
    uint8_t rbuf[STEMMA_RBUF_SIZE];

    err = i2c_master_write_read_device(port, STEMMA_I2C_ADDR, wbuf, 2, rbuf, STEMMA_RBUF_SIZE, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        printf("ERROR: Unable to read soil sensor: %x\n", err);
    }
    unsigned short r = ((uint16_t)rbuf[0]) << 8 | ((uint16_t)rbuf[1]);
    return r;
}

esp_err_t wake_am2320(i2c_port_t port, uint8_t am2320_addr)
{
    esp_err_t res;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();       // Create a command buffer for sending commands
    i2c_master_start(cmd);                              // Enqueue a start transition
    i2c_master_write_byte(cmd, am2320_addr << 1, true); // Write byte, include ack
    i2c_master_stop(cmd);                               // Enqueue stop command

    // Perform command
    res = i2c_master_cmd_begin(port, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return res;
}

esp_err_t am2320_read_temp(i2c_port_t port, uint8_t am2320_addr, uint16_t *temp_reading, uint16_t *hum_reading)
{
    esp_err_t err;
    uint8_t read_cmd[] = {AM2320_READ_CMD, AM2320_HUM_H, 4}; // 2 because we're reading two items
    uint8_t rbuf[6];                                         // function code, data len, data high, data low, crc1, crc2

    // Try waking up the device.
    //  May be necessary to request wakeup multiple times, but more than 5x indicates failure
    for (int i = 0; i < 5; i++)
    {
        err = wake_am2320(port, am2320_addr);
        if (err == ESP_OK)
        {
            break;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    if (err != ESP_OK)
    {
        ESP_LOGI("I2C", "am2320 did not respond to wake command: %s", esp_err_to_name(err));
        return err;
    }
    // Delay a little to allow the device to come up
    vTaskDelay(AM2320_DELAY_T1 / portTICK_PERIOD_MS);

    // Send command to the device to read out data
    // port, address, command buffer, size of command buffer, timeout
    err = i2c_master_write_to_device(port, am2320_addr, read_cmd, 3, I2C_TIMEOUT_MS);
    if (err != ESP_OK)
    {
        ESP_LOGI("I2C", "am2320 did not respond to read command: %s", esp_err_to_name(err));
        return err;
    }
    // Delay before getting back datasetup_i2c
    vTaskDelay(AM2320_DELAY_T2 / portTICK_PERIOD_MS);

    // port, address, read buffer, size of read buffer, timeout
    err = i2c_master_read_from_device(port, am2320_addr, rbuf, 6, I2C_TIMEOUT_MS);
    if (err != ESP_OK)
    {
        ESP_LOGI("I2C", "am2320 did not return read data: %s", esp_err_to_name(err));
        return err;
    }

    // Check validity of returned data
    if (rbuf[0] != AM2320_READ_CMD)
    {
        ESP_LOGI("I2C", "am2320 did not return expected data function (got %d)", rbuf[0]);
        return ESP_ERR_INVALID_RESPONSE;
    }
    if (rbuf[1] != 4)
    {
        ESP_LOGI("I2C", "am2320 did not return expected number of bytes (got %d)", rbuf[1]);
    }
    *temp_reading = (((uint16_t)rbuf[4]) << 8) | ((uint16_t)rbuf[5]);
    *hum_reading = (((uint16_t)rbuf[2]) << 8) | ((uint16_t)rbuf[3]);
    return ESP_OK;
}

/***************************************************************
 *                       MQTT SECTION
 ***************************************************************/
static const char *TAG = "mqtt_example";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/oscarillerup", "ESP SOIL SENSOR ONLINE ", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/oscarillerup", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

volatile bool button_pressed = false;

void IRAM_ATTR button_isr_handler(void *arg)
{
    button_pressed = true;
}

volatile bool timer_expired = false;

void IRAM_ATTR timer_isr_handler(void *arg)
{
    timer_expired = true;
}

void app_main(void)
{

    /***************************************************************
     *                       MQTT APP MAIN SECTION
     ***************************************************************/
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    /***************************************************************
     *                    GPIO PIN SETUP SECTION
     ***************************************************************/
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11); // ADC1_CHANNEL_2 corresponds to GPIO 2

    // Configure of rgb_led
    gpio_reset_pin(blue_led_gpio);
    gpio_reset_pin(red_led_gpio);
    gpio_reset_pin(green_led_gpio);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(blue_led_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(red_led_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(green_led_gpio, GPIO_MODE_OUTPUT);

    gpio_set_level(blue_led_gpio, true);
    gpio_set_level(red_led_gpio, true);
    gpio_set_level(green_led_gpio, true);

    // setup soil_sensor
    int8_t soil_status = 0;
    // temp and hum sensor setup
    esp_err_t err;
    uint16_t temp_reading;
    uint16_t hum_reading;
    float raw_real_temp = 0;
    float raw_real_hum = 0;

    /***************************************************************
     *                    ATTACH INTERRUPT
     ***************************************************************/

    gpio_config_t io_conf;                       // Declare GPIO configuration struct
    io_conf.pin_bit_mask = (1ULL << BUTTON_PIN); // The config relates to BUTTON_GPIO_PIN, defined above
    io_conf.mode = GPIO_MODE_INPUT;              // The config sets the pin as input
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;     // Enable internal pull-up resistor (makes default value 1)
    io_conf.intr_type = GPIO_INTR_NEGEDGE;       // Interrupt on falling edge (when value becomes 0, we connect it to ground)
    gpio_config(&io_conf);                       // Configure GPIO with options abovet

    // Attach ISR to button
    gpio_install_isr_service(0);
    esp_err_t err2 = gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    /***************************************************************
     *                    TIMER CREATE
     ***************************************************************/

    const esp_timer_create_args_t oneshot_timer_args = {
        .callback = &timer_isr_handler,
        /* argument specified here will be passed to timer callback function */
        .arg = true,
        .name = "one-shot"};
    esp_timer_handle_t oneshot_timer;

    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));

    /***************************************************************
     *                    I2C APP MANIN SETUP
     ***************************************************************/

    // Configure of i2c port
    i2c_port_t port = setup_i2c(GPIO_NUM_19, GPIO_NUM_18);
    printf("i2c_port_t value: %d\n", port);

    while (1)
    {
        // Read ADC value
        int raw_light = adc1_get_raw(ADC1_CHANNEL_2);
        char buffer[10];
        sprintf(buffer, "%d", raw_light);
        esp_mqtt_client_publish(client, "/topic/oscarillerup/rawlightdata", buffer, 0, 1, 0);

        // read soil_sensor
        int raw_soil = read_soil_sensor(port);
        sprintf(buffer, "%d", raw_soil);
        esp_mqtt_client_publish(client, "/topic/oscarillerup/rawsoildata", buffer, 0, 1, 0);

        // read temp hum

        err = am2320_read_temp(port, AM2320_I2C_ADDR, &temp_reading, &hum_reading);

        if (err == ESP_OK)
        {
            raw_real_temp = (float)temp_reading / 10.0;
            raw_real_hum = (float)hum_reading / 10;
            printf("temp=%.2f hum=%.2f\n", raw_real_temp, raw_real_hum);
        }
        else
        {
            printf("ERR: Unable to read temperature sensor: %x (%s)\n", err, esp_err_to_name(err));
        }
        sprintf(buffer, "%.2f", raw_real_temp);
        esp_mqtt_client_publish(client, "/topic/oscarillerup/rawtempdata", buffer, 0, 1, 0);

        sprintf(buffer, "%.2f", raw_real_hum);
        esp_mqtt_client_publish(client, "/topic/oscarillerup/rawhumdata", buffer, 0, 1, 0);

        // processed data

        // processing soil
        switch (raw_soil)
        {
        case 0 ... 400:
            soil_status = 0;
            printf("Out of soil\n");
            esp_mqtt_client_publish(client, "/topic/oscarillerup/soilprocesseddata", "Out of soil", 0, 1, 0);
            break;
        case 401 ... 700:
            soil_status = 1;
            printf("Dry soil\n");
            esp_mqtt_client_publish(client, "/topic/oscarillerup/soilprocesseddata", "Dry soil", 0, 1, 0);
            break;
        case 701 ... 775:
            soil_status = 2;
            printf("light moist soil\n");
            esp_mqtt_client_publish(client, "/topic/oscarillerup/soilprocesseddata", "light moist soil", 0, 1, 0);
            break;
        case 776 ... 900:
            soil_status = 3;
            printf("perfect soil\n");
            esp_mqtt_client_publish(client, "/topic/oscarillerup/soilprocesseddata", "perfect soil", 0, 1, 0);
            break;
        case 901 ... 2000:
            soil_status = 4;
            printf("Too wet soil\n");
            esp_mqtt_client_publish(client, "/topic/oscarillerup/soilprocesseddata", "Too wet soil", 0, 1, 0);
            break;

        default:
            break;
        }

        // processing light
        if (raw_light < 3000)
        {
            printf("light on\n");
            esp_mqtt_client_publish(client, "/topic/oscarillerup/lightprocesseddata", "light on", 0, 1, 0);
        }
        else
        {
            printf("light off\n");
            esp_mqtt_client_publish(client, "/topic/oscarillerup/lightprocesseddata", "light off", 0, 1, 0);
        }

        // ISR's should set flags, while-loop in app_main should respond to flags and revert their state
        if (button_pressed)
        {
            printf("Button pressed\n");
            // Your code here
            ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, 5000000));
            led_on(soil_status);
            button_pressed = false;
        }
        if (timer_expired)
        {
            printf("Timer expired\n");
            // Your code here
            led_off();
            timer_expired = false;
        }

        // Print the ADC value to the console
        printf("ADC Value: %d\n", raw_light);

        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
