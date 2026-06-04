/*
 * PWM.c — 四路 PWM 底层驱动（RA8P1 GPT 定时器）
 *
 * ========== 硬件引脚映射（全部在 CPU0 的 configuration.xml 中配置）==========
 *
 *  PWM 输出（GPT 定时器，频率 9kHz）：
 *    GPT 通道    |  信号线   |  引脚  |  对应电机
 *    Timer1 Ch.1 | GTIOCA   | P104  |  Motor 2（左前轮）
 *    Timer1 Ch.1 | GTIOCB   | P105  |  Motor 1（左后轮）
 *    Timer3 Ch.3 | GTIOCA   | P403  |  Motor 3（右前轮）
 *    Timer3 Ch.3 | GTIOCB   | P404  |  Motor 4（右后轮）
 *
 *  方向控制（GPIO 输出，驱动 TB6612/L298N 等电机驱动芯片的 IN1/IN2）：
 *    电机       | IN1 引脚 | IN2 引脚 |  前进(IN1,IN2) | 后退(IN1,IN2)
 *    Motor 1 左后 | P308    | P310    |  (LOW, HIGH)  | (HIGH, LOW)
 *    Motor 2 左前 | P311    | P312    |  (LOW, HIGH)  | (HIGH, LOW)
 *    Motor 3 右前 | P511    | P512    |  (LOW, HIGH)  | (HIGH, LOW)
 *    Motor 4 右后 | P805    | P806    |  (LOW, HIGH)  | (HIGH, LOW)
 *
 *  全部四轮统一极性（实测验证）。
 *
 * ========== 占空比范围 ==========
 *  占空比限制在 40%~80%（对应 0.4f ~ 0.8f），
 *  低于 40% 电机无法启动，高于 80% 留有余量保护驱动芯片。
 */

#include "hal_data.h"
#include "Motor.h"

/*
 * PWM12_Init — 初始化 Timer1（驱动 Motor 1 左后 + Motor 2 左前）
 * 引脚：P104(GTIOCA), P105(GTIOCB)
 */
void PWM12_Init(void) {
    R_GPT_Open(&g_timer1_ctrl, &g_timer1_cfg);      /* 打开 GPT1 通道，配置 PWM 模式 */
    R_GPT_Start(&g_timer1_ctrl);                      /* 启动定时器计数 */
    R_GPT_Enable(&g_timer1_ctrl);                     /* 使能 PWM 输出 */
    R_GPT_DutyCycleSet(&g_timer1_ctrl, 0, GPT_IO_PIN_GTIOCA);  /* 初始占空比 0 */
    R_GPT_DutyCycleSet(&g_timer1_ctrl, 0, GPT_IO_PIN_GTIOCB);
}

/*
 * PWM34_Init — 初始化 Timer3（驱动 Motor 3 右前 + Motor 4 右后）
 * 引脚：P403(GTIOCA), P404(GTIOCB)
 */
void PWM34_Init(void) {
    R_GPT_Open(&g_timer3_ctrl, &g_timer3_cfg);
    R_GPT_Start(&g_timer3_ctrl);
    R_GPT_Enable(&g_timer3_ctrl);
    R_GPT_DutyCycleSet(&g_timer3_ctrl, 0, GPT_IO_PIN_GTIOCA);
    R_GPT_DutyCycleSet(&g_timer3_ctrl, 0, GPT_IO_PIN_GTIOCB);
}

/* PWM_Init — 初始化全部 4 路 PWM */
void PWM_Init(void) {
    PWM34_Init();
    PWM12_Init();
}

/*
 * PWM_DutyCycleSet — 设置指定电机的 PWM 占空比
 *   num    : 电机编号 1~4（1=左后, 2=左前, 3=右前, 4=右后）
 *   percent: 占空比 0.0~1.0（调用方已限制在 0.4~0.8）
 *
 * 占空比计算方式：占空比 × period_counts（周期计数值）
 * period_counts 由 e2studio 根据 9kHz 频率自动计算，存储在 hal_data.c 中。
 */
void PWM_DutyCycleSet(uint8_t num, float percent) {
    uint32_t duty_count;
    timer_info_t TIMER;

    switch (num) {
    case 1: /* Motor 1: 左后轮 → Timer1 GTIOCB (P105) */
        R_GPT_InfoGet(&g_timer1_ctrl, &TIMER);
        duty_count = TIMER.period_counts;
        R_GPT_DutyCycleSet(&g_timer1_ctrl, percent * duty_count, GPT_IO_PIN_GTIOCB);
        break;
    case 2: /* Motor 2: 左前轮 → Timer1 GTIOCA (P104) */
        R_GPT_InfoGet(&g_timer1_ctrl, &TIMER);
        duty_count = TIMER.period_counts;
        R_GPT_DutyCycleSet(&g_timer1_ctrl, percent * duty_count, GPT_IO_PIN_GTIOCA);
        break;
    case 3: /* Motor 3: 右前轮 → Timer3 GTIOCA (P403) */
        R_GPT_InfoGet(&g_timer3_ctrl, &TIMER);
        duty_count = TIMER.period_counts;
        R_GPT_DutyCycleSet(&g_timer3_ctrl, percent * duty_count, GPT_IO_PIN_GTIOCA);
        break;
    case 4: /* Motor 4: 右后轮 → Timer3 GTIOCB (P404) */
        R_GPT_InfoGet(&g_timer3_ctrl, &TIMER);
        duty_count = TIMER.period_counts;
        R_GPT_DutyCycleSet(&g_timer3_ctrl, percent * duty_count, GPT_IO_PIN_GTIOCB);
        break;
    }
}

/*
 * PWM_DirectionSet — 设置指定电机的转向
 *   num: 电机编号 1~4
 *   di : 1=前进方向, 0=后退方向
 *
 * 通过控制电机驱动芯片的 IN1/IN2 两个 GPIO 实现转向：
 *   前进 → IN1=HIGH, IN2=LOW
 *   后退 → IN1=LOW, IN2=HIGH
 *
 * 注意：Motor 2（左前轮）因安装方向，前进/后退的 IN1/IN2 极性与其余电机相反。
 * 这是硬件安装决定的，如果重新安装电机改变了方向，修改此函数中各 case 的
 * IN1/IN2 高低电平即可。
 */
void PWM_DirectionSet(uint8_t num, uint8_t di) {
    /*
     * 统一方向逻辑（实测验证）：
     *   前进(di=1) → IN1=LOW, IN2=HIGH
     *   后退(di=0) → IN1=HIGH, IN2=LOW
     */
    uint8_t in1, in2;
    if (di == 1) {
        in1 = 0; in2 = 1;   /* 前进 */
    } else {
        in1 = 1; in2 = 0;   /* 后退 */
    }

    switch (num) {
    case 1: /* 左后轮 — P308(IN1), P310(IN2) */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_03_PIN_08, in1);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_03_PIN_10, in2);
        break;
    case 2: /* 左前轮 — P311(IN1), P312(IN2) */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_03_PIN_11, in1);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_03_PIN_12, in2);
        break;
    case 3: /* 右前轮 — P511(IN1), P512(IN2) */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_05_PIN_11, in1);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_05_PIN_12, in2);
        break;
    case 4: /* 右后轮 — P805(IN1), P806(IN2) */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_05, in1);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_06, in2);
        break;
    }
}
