/*
 * dz_pid.h
 *
 *  Created on: May 10, 2022
 *      Author: abdul
 */

#ifndef INC_DZ_PID_H_
#define INC_DZ_PID_H_

#include "stdint.h"

#define DELTAT (double)0.01
#define PIDMAX	100
#define PIDMIN	-100


void pidRollChange_KP(double* data);
void pidRollChange_KI(double* data);
void pidRollChange_KD(double* data);

int16_t pidRollCalculate(int16_t ref, int16_t imu);
int16_t pidPitchCalculate(int16_t ref, int16_t imu);
int16_t pidYawCalculate(int16_t ref, int16_t imu);
void pid_yawChange(int16_t* imu);
void pidRollReset();
void pidYawReset();
void pidPitchReset();



#endif /* INC_DZ_PID_H_ */
