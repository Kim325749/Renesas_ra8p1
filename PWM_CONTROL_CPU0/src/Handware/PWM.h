/*
 * PWM.h — PWM 底层驱动接口声明
 *
 * 提供 PWM 初始化和占空比/方向控制函数。
 * 具体实现和引脚映射见 PWM.c。
 */

#ifndef HANDWARE_PWM1_H_
#define HANDWARE_PWM1_H_
#include "hal_data.h"

/* 初始化 Timer1（Motor 1+2 的 PWM） */
void PWM12_Init(void);

/* 初始化 Timer3（Motor 3+4 的 PWM） */
void PWM34_Init(void);

/* 初始化全部 4 路 PWM */
void PWM_Init(void);

/* 设置指定电机的 PWM 占空比（num: 1~4, percent: 0.0~1.0） */
void PWM_DutyCycleSet(uint8_t num, float percent);

/* 设置指定电机的转向（num: 1~4, di: 1=前进, 0=后退） */
void PWM_DirectionSet(uint8_t num, uint8_t di);

#endif /* HANDWARE_PWM1_H_ */
