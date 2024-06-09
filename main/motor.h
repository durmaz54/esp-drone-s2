// Creator:  Samet Durmaz
// Date: 26.05.2024
#ifndef MOTOR_H
#define MOTOR_H

#include "driver/ledc.h"
#include "esp_log.h"

#define MOTOR1_PIN 5
#define MOTOR2_PIN 6
#define MOTOR3_PIN 7
#define MOTOR4_PIN 8

#define BMOTOR_MAX_VALUE 2000
#define BMOTOR_MIN_VALUE 1000

struct BMotor{
    uint16_t motor1;
    uint16_t motor2;
    uint16_t motor3;
    uint16_t motor4;
};

void bmotor_init();
void bmotor_write(const struct BMotor *bmotor);

#endif // MOTOR_H
