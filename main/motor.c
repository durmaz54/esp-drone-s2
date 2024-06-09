#include "motor.h"

void bmotor_init()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_14_BIT, // 14 bit = 16384
        .timer_num = LEDC_CHANNEL_0,
        .freq_hz = (50),
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    const int pwm_out_gpio[4] = {MOTOR1_PIN, MOTOR2_PIN, MOTOR3_PIN, MOTOR4_PIN};
    ledc_channel_t ledc_channels_id[4] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3};
    for (int8_t i = 0; i < 4; i++)
    {
        ledc_channel_config_t ledc_channel = {
            .speed_mode = LEDC_TIMER_0,
            .channel = ledc_channels_id[i],
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = pwm_out_gpio[i],
            .duty = 820, // %5 duty = 820 and %10 duty = 1638
            .hpoint = 0};
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }
}

static uint16_t xto_820_1638(uint16_t value){
    if(value < BMOTOR_MIN_VALUE){
        value = BMOTOR_MIN_VALUE;
    }else if (value > BMOTOR_MAX_VALUE)
    {
        value = BMOTOR_MAX_VALUE;
    }
    return (value * 820) / 1638;
}

void bmotor_write(const struct BMotor *bmotor)
{
    uint16_t motor1 = xto_820_1638(bmotor->motor1);
    uint16_t motor2 = xto_820_1638(bmotor->motor2);
    uint16_t motor3 = xto_820_1638(bmotor->motor3);
    uint16_t motor4 = xto_820_1638(bmotor->motor4);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_TIMER_0, LEDC_CHANNEL_0, motor1));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_TIMER_0, LEDC_CHANNEL_1, motor2));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_TIMER_0, LEDC_CHANNEL_2, motor3));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_TIMER_0, LEDC_CHANNEL_3, motor4));
    ESP_LOGI("BMOTOR", "Motor1: %d, Motor2: %d, Motor3: %d, Motor4: %d", motor1, motor2, motor3, motor4);

    ESP_ERROR_CHECK(ledc_update_duty(LEDC_TIMER_0, LEDC_CHANNEL_0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_TIMER_0, LEDC_CHANNEL_1));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_TIMER_0, LEDC_CHANNEL_2));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_TIMER_0, LEDC_CHANNEL_3));
}

