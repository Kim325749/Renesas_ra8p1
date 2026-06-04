/*
 * hal_entry.c — CPU1（从核）入口
 *
 * RA8P1 双核架构说明：
 *   CPU0 (Cortex-M85)：负责引脚配置、启动 CPU1
 *   CPU1 (Cortex-M33)：负责外设驱动和运动控制逻辑
 *
 * 本文件是 CPU1 的入口点。在非 TrustZone 双核项目中，
 * CPU0 启动后会唤醒 CPU1，CPU1 从此处开始执行。
 * 引脚在 CPU0 的 pin_data.c 中配置，CPU1 仅通过外设 HAL 驱动访问。
 */

#include "hal_data.h"


#if (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
bsp_ipc_semaphore_handle_t g_core_start_semaphore =
{
    .semaphore_num = 0
};
#endif

void hal_entry(void) {


	/* ---- 用户初始化 ---- */
    //Motor_Init();       /* 初始化 4 路 PWM 和方向控制 GPIO */
    /*UART_CMD_Init();    /* 初始化蓝牙串口 (SCI0, 115200 baud) */

/*
    PWM_Init();
    PWM_DutyCycleSet(1,0.6f);
    PWM_DutyCycleSet(2,0.6f);
    PWM_DutyCycleSet(3,0.6f);
    PWM_DutyCycleSet(4,0.6f);


	/*Motor_Forward(0.6f);


		while(1);


    /* 主循环：所有运动控制通过 UART 中断回调触发，此处仅保持程序运行 */
   /*while (1) {
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }

    /* 以下为 TrustZone 多核模板代码，非 TZ 构建时被条件编译排除 */



#if (0 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && !BSP_TZ_NONSECURE_BUILD
#if BSP_TZ_SECURE_BUILD
    R_BSP_IpcSemaphoreTake(&g_core_start_semaphore);
#endif
    R_BSP_SecondaryCoreStart();
#if BSP_TZ_SECURE_BUILD
    while(FSP_ERR_IN_USE == R_BSP_IpcSemaphoreTake(&g_core_start_semaphore))
    {
        ;
    }
#endif
#endif

#if (1 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
    R_BSP_IpcSemaphoreGive(&g_core_start_semaphore);
#endif

#if BSP_TZ_SECURE_BUILD
    R_BSP_NonSecureEnter();
#endif
}

#if BSP_TZ_SECURE_BUILD
FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{
}
FSP_CPP_FOOTER
#endif
