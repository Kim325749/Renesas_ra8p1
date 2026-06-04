/*
 * Motor.h — 麦克纳姆轮小车运动控制接口
 *
 * 速度参数 speed 范围：0.4f ~ 0.8f（对应 40%~80% PWM 占空比）
 * 超出范围的值会被 limit_speed() 自动钳位。
 */

#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 初始化 ---- */
void Motor_Init(void);

/* ---- 基础运动 ---- */
void Motor_Forward(float speed);     /* 前进 */
void Motor_Backward(float speed);    /* 后退 */
void Motor_TurnLeft(float speed);    /* 原地左转 */
void Motor_TurnRight(float speed);   /* 原地右转 */

/* ---- 麦轮平移 ---- */
void Motor_LeftShift(float speed);   /* 左平移 */
void Motor_RightShift(float speed);  /* 右平移 */

/* ---- 停止 ---- */
void Motor_Stop(void);

/* ---- 底层 PWM 驱动声明（在 PWM.c 中实现）---- */
void PWM12_Init(void);
void PWM34_Init(void);
void PWM_DutyCycleSet(uint8_t motor_num, float duty);
void PWM_DirectionSet(uint8_t motor_num, uint8_t dir);

#ifdef __cplusplus
}
#endif

#endif
