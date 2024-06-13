/*
 * dz_pid.c
 *
 *  Created on: May 10, 2022
 *      Author: abdul
 */

#include "dz_pid.h"

double ROLL_KP = 1; 	//
double ROLL_KI = 0.8;	//
double ROLL_KD = 0.8;	//

double PITCH_KP = 1;
double PITCH_KI = 0.8;
double PITCH_KD = 0.8;

double YAW_KP = 1; // 3 - 1
double YAW_KI = 1.5;
double YAW_KD = 0.0;

double roll_p, roll_i, roll_d;
int16_t previous_error_roll = 0;
int16_t previous_error_pitch = 0;
int16_t previous_error_yaw = 0;
double yaw_p, yaw_i, yaw_d;
double pitch_p, pitch_i, pitch_d;
int16_t roll_pid, yaw_pid, pitch_pid;
int16_t imu_previous_yaw = 0;

void pidRollChange_KP(double *data) {

	//ROLL_KP = *data;
	//PITCH_KP = *data;
	YAW_KP = *data;
}
void pidRollChange_KI(double *data) {
	//ROLL_KI = *data;
	//PITCH_KI = *data;
	YAW_KI = *data;
}
void pidRollChange_KD(double *data) {
	//ROLL_KD = *data;
	//PITCH_KD = *data;
	//YAW_KD = *data;
}

int16_t pidRollCalculate(int16_t ref, int16_t imu) {

	int16_t hata = ref - imu;

	roll_p = ROLL_KP * (double)hata;

	roll_d = ROLL_KD * (double)(hata - previous_error_roll) / DELTAT;

	roll_i += (double)hata * DELTAT;

	roll_pid = (int16_t)(roll_p + (roll_i * ROLL_KI) + roll_d);

	previous_error_roll = hata;

	if(roll_pid > PIDMAX){
		roll_pid = PIDMAX;
	}
	else if(roll_pid < PIDMIN){
		roll_pid = PIDMIN;
	}

	return roll_pid;

}

int16_t pidPitchCalculate(int16_t ref, int16_t imu) {

	int16_t hata = ref - imu;

	pitch_p = PITCH_KP * (double)hata;

	pitch_d = PITCH_KD * (double)(hata - previous_error_pitch) / DELTAT;

	pitch_i += (double)hata * DELTAT;

	pitch_pid = (int16_t)(pitch_p + (pitch_i * PITCH_KI) + pitch_d);

	previous_error_pitch = hata;

	if(pitch_pid > PIDMAX){
		pitch_pid = PIDMAX;
	}
	else if(pitch_pid < PIDMIN){
		pitch_pid = PIDMIN;
	}

	return pitch_pid;

}

int16_t pidYawCalculate(int16_t ref, int16_t imu) {

	pid_yawChange(&imu);

	int16_t hata = ref - imu;

	yaw_p = YAW_KP * (double)hata;

	yaw_d = YAW_KD * (double)(hata - previous_error_yaw) / DELTAT;

	yaw_i += (double)hata * DELTAT;

	yaw_pid = (int16_t)(yaw_p + (yaw_i * YAW_KI) + yaw_d);

	previous_error_yaw = hata;

	if(yaw_pid > PIDMAX){
		yaw_pid = PIDMAX;
	}
	else if(yaw_pid < PIDMIN){
		yaw_pid = PIDMIN;
	}

	return yaw_pid;


}

void pidRollReset() {
	roll_i = 0;
}
void pidYawReset() {
	yaw_i = 0;
}
void pidPitchReset() {
	pitch_i = 0;
}



void pid_yawChange(int16_t* imu){
	int16_t yawanglechangedelta = *imu - imu_previous_yaw;

	//   if (yawanglechangedelta > 180) {
	//     yawanglechangedelta - 360;
	//   } else if (yawanglechangedelta < -180) {
	//     yawanglechangedelta + 360;
	//   }

	yawanglechangedelta *= 100;

	imu_previous_yaw = *imu;


	*imu = yawanglechangedelta;
}








