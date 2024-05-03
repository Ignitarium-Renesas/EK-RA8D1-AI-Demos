
/**********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO
 * THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2021 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
/**********************************************************************************************************************
 * File Name    : blinky_thread_entry.c
 * Version      : .
 * Description  : .
 *********************************************************************************************************************/

#include <blinky_thread.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"


#include "menu_main.h"
#include "common_utils.h"
#include "common_init.h"
#include "menu_main.h"

#include "r_typedefs.h"
#include "portable.h"

static char_t s_buffer[128] = { };

/**********************************************************************************************************************
 * Function Name: blinky_thread_entry
 * Description  : .
 * Argument     : pvParameters (contains TaskHandle_t)
 * Return Value : .
 *********************************************************************************************************************/
void blinky_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    size_t uxCurrentSize = 0;
    size_t uxMinLastSize = 0;

    uxMinLastSize = xPortGetFreeHeapSize();

    // dumb way to let system startup
    vTaskDelay (3000);

    while (1)
    {
        vTaskDelay (100);

        uxCurrentSize = xPortGetFreeHeapSize();
        if(uxMinLastSize > uxCurrentSize)
        {
            uxMinLastSize = uxCurrentSize;
        }

        sprintf(s_buffer, "Heap: current %u lowest %u\r", uxCurrentSize, uxMinLastSize);
//        print_to_console((void*)s_buffer);

    }
}
/**********************************************************************************************************************
 End of function blinky_thread_entry
 *********************************************************************************************************************/
void debug_memory_free(uint8_t id, void * ptr);

void debug_memory_free(uint8_t id, void * ptr)
{
    char ref[32] = "UKN";

    switch(id)
    {
        case 0:
            sprintf(ref,"wi_alloc");
            break;
        case 1:
            sprintf(ref,"wi_txalloc");
            break;
        default:
            sprintf(ref,"UKN");
    }
    sprintf(s_buffer, "%s Heap: current %u after malloc %p \r\n", ref, xPortGetFreeHeapSize(), ptr);
    print_to_console((void*)s_buffer);
}
