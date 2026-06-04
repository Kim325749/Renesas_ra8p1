/*
 * UART_CMD.c — 串口0 收发功能
 *
 * 硬件配置：
 *   外设：SCI0 (SCI_B_UART)
 *   引脚：P602(RXD), P603(TXD) — 在 CPU0 中配置
 *   波特率：115200 bps
 *
 * 接收方式：中断接收 + 环形缓冲区，非阻塞读取
 */

#include "UART_CMD.h"
#include "hal_data.h"
#include "r_sci_b_uart.h"

/* ---- 接收环形缓冲区 ---- */
#define RX_BUF_SIZE   256
static uint8_t g_rx_buf[RX_BUF_SIZE];
static volatile uint16_t g_rx_head = 0;  /* 中断回调写入位置 */
static volatile uint16_t g_rx_tail = 0;  /* 用户读取位置 */
static uint8_t g_rx_byte;                /* 单字节接收缓冲 */

/* 环形缓冲区内可读字节数 */
static inline uint16_t rx_buf_avail(void)
{
    return (g_rx_head - g_rx_tail) & (RX_BUF_SIZE - 1);
}

/* ========== 以下为蓝牙指令处理代码（已注释） ========== */
#if 0

#include "Motor.h"

/* 当前速度，默认 0.6（60% 占空比） */
static float g_speed = 0.6f;

/* UART 接收完成中断回调 */
static void uart_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_COMPLETE) {
        char cmd = (char)g_rx_byte;

        /* 回显：将收到的字符原样发回，用于调试确认通信正常 */
        R_SCI_B_UART_Write(&g_uart0_ctrl, &g_rx_byte, 1);

        switch (cmd) {
        case 'F':  Motor_Forward(g_speed);     break;  /* 前进 */
        case 'B':  Motor_Backward(g_speed);    break;  /* 后退 */
        case 'L':  Motor_TurnLeft(g_speed);    break;  /* 左转 */
        case 'R':  Motor_TurnRight(g_speed);   break;  /* 右转 */
        case 'Q':  Motor_LeftShift(g_speed);   break;  /* 左平移 */
        case 'E':  Motor_RightShift(g_speed);  break;  /* 右平移 */
        case 'S':  Motor_Stop();               break;  /* 停止 */

        /* 速度档位：'0'=40% 到 '9'=80%，每档增加约 4.44% */
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            g_speed = 0.4f + (cmd - '0') * 0.044444f;
            break;

        default:
            break;
        }

        /* 重新启动单字节接收，保持持续监听 */
        R_SCI_B_UART_Read(&g_uart0_ctrl, &g_rx_byte, 1);
    }
}

#endif
/* ========== 蓝牙指令处理代码结束 ========== */


/* UART 接收中断回调 — 将收到的字节存入环形缓冲区 */
static void uart_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_COMPLETE) {
        uint16_t next = (g_rx_head + 1) & (RX_BUF_SIZE - 1);

        /* 缓冲区未满则存入，满了则丢弃（不覆盖旧数据） */
        if (next != g_rx_tail) {
            g_rx_buf[g_rx_head] = g_rx_byte;
            g_rx_head = next;
        }

        /* 继续接收下一字节 */
        R_SCI_B_UART_Read(&g_uart0_ctrl, &g_rx_byte, 1);
    }
}


/* UART_CMD_Init — 初始化串口0，启动中断接收 */
void UART_CMD_Init(void)
{
    fsp_err_t err;

    /* 打开 SCI0 UART 通道 */
    err = R_SCI_B_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
    if (FSP_SUCCESS != err) {
        while (1) { }
    }
 
    /* TX 引脚上拉稳定延时：避免 TX 线在空闲时浮空，导致下次发送的起始位被误判而造成数据损坏 */
    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);

    /* 注册接收中断回调 */
    R_SCI_B_UART_CallbackSet(&g_uart0_ctrl, uart_callback, NULL, NULL);

    /* 启动单字节接收，后续由中断持续驱动 */
    R_SCI_B_UART_Read(&g_uart0_ctrl, &g_rx_byte, 1);

    /* 以下为蓝牙指令相关代码（已注释）：
     *   发送欢迎消息...
     */
}


/* UART_CMD_Send — 通过串口0发送数据 */
uint32_t UART_CMD_Send(uint8_t *p_data, uint32_t length)
{
    fsp_err_t err;

    err = R_SCI_B_UART_Write(&g_uart0_ctrl, p_data, length);
    if (FSP_SUCCESS != err) {
        return 0;
    }

    return length;
}


/* UART_CMD_Recv — 从串口0接收缓冲区读取数据（非阻塞） */
uint32_t UART_CMD_Recv(uint8_t *p_data, uint32_t length)
{
    uint32_t count = 0;

    while (count < length && g_rx_head != g_rx_tail) {
        p_data[count++] = g_rx_buf[g_rx_tail];
        g_rx_tail = (g_rx_tail + 1) & (RX_BUF_SIZE - 1);
    }

    return count;
}
