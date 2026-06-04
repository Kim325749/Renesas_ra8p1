/*
 * Motor.c — 麦克纳姆轮小车运动控制层
 *
 * 本层封装了麦克纳姆轮（Mecanum Wheel）的运动学逻辑，
 * 将前进、后退、平移、旋转等高层运动指令转换为四个电机的
 * PWM 占空比和方向控制信号。
 *
 * ========== 电机编号与车轮位置 ==========
 *   Motor 1 — 左后轮
 *   Motor 2 — 左前轮
 *   Motor 3 — 右前轮
 *   Motor 4 — 右后轮
 *
 * ========== 麦轮运动原理 ==========
 *   【前进/后退】四轮同向旋转，合力向前/向后
 *   【原地左转】左侧两轮后转 + 右侧两轮前转，车体绕中心逆时针旋转
 *   【原地右转】左侧两轮前转 + 右侧两轮后转，车体绕中心顺时针旋转
 *   【左平移】  对角轮同向：W1/W3前转 + W2/W4后转，合力向左
 *   【右平移】  对角轮同向：W1/W3后转 + W2/W4前转，合力向右
 *
 * ========== 占空比限制 ==========
 *   PWM 频率：9kHz（由 e2studio 中 GPT 配置决定）
 *   有效占空比范围：40% ~ 80%（低于 40% 电机无法转动）
 */

#include "Motor.h"
#include "hal_data.h"
#include <stdint.h>

/* ---- 内部辅助函数 ------------------------------------------------ */

/* 将占空比限制在 [0.4, 0.8] 范围内 */
static float limit_speed(float speed)
{
    if (speed < 0.4f) return 0.4f;
    if (speed > 0.8f) return 0.8f;
    return speed;
}

/* 分别设置四个轮子的占空比（用于需要四轮不同占空比的场景） */
static void set_part_duty(float duty1, float duty2, float duty3, float duty4)
{
    PWM_DutyCycleSet(1, duty1);
    PWM_DutyCycleSet(2, duty2);
    PWM_DutyCycleSet(3, duty3);
    PWM_DutyCycleSet(4, duty4);
}

/* 设置四个轮子为相同占空比 */
static void set_all_duty(float duty)
{
    PWM_DutyCycleSet(1, duty);
    PWM_DutyCycleSet(2, duty);
    PWM_DutyCycleSet(3, duty);
    PWM_DutyCycleSet(4, duty);
}

/* ---- 方向组合函数 ------------------------------------------------ */

/* 四轮同向：全部前进或全部后退 */
static void set_all_direction(uint8_t dir)
{
    PWM_DirectionSet(1, dir);
    PWM_DirectionSet(2, dir);
    PWM_DirectionSet(3, dir);
    PWM_DirectionSet(4, dir);
}

/* 原地左转方向：左侧两轮反转 + 右侧两轮正转 */
static void set_turnleft_direction(void)
{
    PWM_DirectionSet(1, 0);   /* 左后轮反转 */
    PWM_DirectionSet(2, 0);   /* 左前轮反转 */
    PWM_DirectionSet(3, 1);   /* 右前轮正转 */
    PWM_DirectionSet(4, 1);   /* 右后轮正转 */
}

/* 原地右转方向：左侧两轮正转 + 右侧两轮反转 */
static void set_turnright_direction(void)
{
    PWM_DirectionSet(1, 1);   /* 左后轮正转 */
    PWM_DirectionSet(2, 1);   /* 左前轮正转 */
    PWM_DirectionSet(3, 0);   /* 右前轮反转 */
    PWM_DirectionSet(4, 0);   /* 右后轮反转 */
}

/* 左平移方向：对角同向，W1/W3正转 + W2/W4反转 */
static void set_leftshift_direction(void)
{
    PWM_DirectionSet(1, 1);   /* 左后轮正转 */
    PWM_DirectionSet(2, 0);   /* 左前轮反转 */
    PWM_DirectionSet(3, 1);   /* 右前轮正转 */
    PWM_DirectionSet(4, 0);   /* 右后轮反转 */
}

/* 右平移方向：对角同向，W1/W3反转 + W2/W4正转 */
static void set_rightshift_direction(void)
{
    PWM_DirectionSet(1, 0);   /* 左后轮反转 */
    PWM_DirectionSet(2, 1);   /* 左前轮正转 */
    PWM_DirectionSet(3, 0);   /* 右前轮反转 */
    PWM_DirectionSet(4, 1);   /* 右后轮正转 */
}

/* ---- 公共运动接口 ------------------------------------------------ */

/* Motor_Init — 初始化全部电机，停车，默认设为前进方向 */
void Motor_Init(void)
{
    PWM12_Init();          /* 初始化 Timer1（Motor 1+2） */
    PWM34_Init();          /* 初始化 Timer3（Motor 3+4） */
    Motor_Stop();          /* 占空比归零 */
    set_all_direction(1);  /* 默认前进方向 */
}

/* Motor_Forward — 前进（四轮正转） */
void Motor_Forward(float speed)
{
    speed = limit_speed(speed);
    set_all_direction(1);
    set_all_duty(speed);
}

/* Motor_Backward — 后退（四轮反转） */
void Motor_Backward(float speed)
{
    speed = limit_speed(speed);
    set_all_direction(0);
    set_all_duty(speed);
}

/* Motor_TurnLeft — 原地左转（左侧反转 + 右侧正转） */
void Motor_TurnLeft(float speed)
{
    speed = limit_speed(speed);
    set_turnleft_direction();
    set_all_duty(speed);
}

/* Motor_TurnRight — 原地右转（左侧正转 + 右侧反转） */
void Motor_TurnRight(float speed)
{
    speed = limit_speed(speed);
    set_turnright_direction();
    set_all_duty(speed);
}

/* Motor_LeftShift — 左平移（W1/W3正转 + W2/W4反转） */
void Motor_LeftShift(float speed)
{
    speed = limit_speed(speed);
    set_leftshift_direction();
    set_all_duty(speed);
}

/* Motor_RightShift — 右平移（W1/W3反转 + W2/W4正转） */
void Motor_RightShift(float speed)
{
    speed = limit_speed(speed);
    set_rightshift_direction();
    set_all_duty(speed);
}

/* Motor_Stop — 停止：四轮占空比归零，电机停转 */
void Motor_Stop(void)
{
    set_all_duty(0.0f);
}
