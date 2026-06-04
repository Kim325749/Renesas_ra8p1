/*
 * UART_CMD.h — 串口0 发送接口
 */

#ifndef UART_CMD_H
#define UART_CMD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化串口0 (SCI0, 115200 baud) */
void UART_CMD_Init(void);

/* 通过串口0发送数据，返回实际发送的字节数 */
uint32_t UART_CMD_Send(uint8_t *p_data, uint32_t length);

/* 从串口0接收数据（非阻塞），返回实际读取的字节数，无数据时返回0 */
uint32_t UART_CMD_Recv(uint8_t *p_data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
