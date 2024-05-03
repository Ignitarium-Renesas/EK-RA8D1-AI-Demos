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
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
/**********************************************************************************************************************
 * File Name    : menu_main.c
 * Description  : The main menu controller.
 *********************************************************************************************************************/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"


#include "menu_main.h"
#include "common_utils.h"
#include "common_init.h"
#include "menu_main.h"


#define MODULE_NAME     "\r\n\x1b[2m\x1b[37mWelcome to QSEP Project for %s!\r\n"

#define SUB_OPTIONS     "\r\n> Select from the options in the menu below:\r\n" \
                        "\r\nMENU"


typedef struct menu_fn_tbl
{
    char_t * p_name;                      /*<! Name of Test */
    test_fn ( * p_func)(void);            /*<! Pointer to Test Function */
} st_menu_fn_tbl_t;


/* Table of menu functions */
static st_menu_fn_tbl_t s_menu_items[] =
{
    {"Kit Information"                                , kis_display_menu},
    {"Various Interactive Connectivity & HMI Demos "  , camview_display_menu},
//    {"AI Demo (text console)"                  , ai_display_menu},
//    {"IOT Demo (text console)"                 , iot_display_menu},
    {"Next Steps"                                     , ns_display_menu},
    {"", NULL}
};

int8_t g_selected_menu = 0;

static char_t s_print_buffer[BUFFER_LINE_LENGTH] = {};
static bool_t s_bool_screen_test = true;

/**********************************************************************************************************************
 * Function Name: main_display_menu
 * Description  : .
 * Return Value : The main menu controller.
 *********************************************************************************************************************/
int8_t main_display_menu(void)
{
    int8_t c = -1;
    int8_t menu_limit = 0;


#if 0
             /* Start AI Demo */
             xEventGroupSetBits(g_update_console_event, STATUS_START_ENABLE_AI_DEMO);
#endif



    if(s_bool_screen_test == true)
    {
        /*         automatically run graphics */
        sprintf (s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);

        g_selected_menu = 2;

        camview_display_menu(s_bool_screen_test);

        s_bool_screen_test = false;

        /* When this workflow has run drop to end of function to froce menu redraw */
    }

//    sprintf (s_print_buffer, "%s%s", gp_clear_screen, gp_cursor_home);
//
//    /* ignoring -Wpointer-sign is OK when treating signed char_t array as as unsigned */
//    print_to_console((void*)s_print_buffer);
//    sprintf (s_print_buffer, MODULE_NAME, FULL_NAME);
//
//    /* ignoring -Wpointer-sign is OK when treating signed char_t array as as unsigned */
//    print_to_console((void*)s_print_buffer);

#if 0
    sprintf (s_print_buffer, "%s\r\n", FSP_VERSION_BUILD_STRING);
    print_to_console((void*)s_print_buffer);

    sprintf (s_print_buffer, "%s\r\n", __TIMESTAMP__);
    print_to_console((void*)s_print_buffer);

    sprintf (s_print_buffer, "Version 0.61 - AI Demo, MIPI Display using double buffered background Frame \r\n");
    print_to_console((void*)s_print_buffer);
#endif

//    sprintf (s_print_buffer, SUB_OPTIONS);
//
//    /*  */
//    /* ignoring -Wpointer-sign is OK when treating signed char_t array as as unsigned */
//    print_to_console((void*)s_print_buffer);
//
//    for (int8_t test_active = 0; NULL != s_menu_items[test_active].p_func; test_active++ )
//    {
//        sprintf (s_print_buffer, "\r\n %d. %s", (test_active + 1), s_menu_items[menu_limit++ ].p_name);
//
//        /* ignoring -Wpointer-sign is OK when treating signed char_t array as as unsigned */
//        print_to_console((void*)s_print_buffer);
//    }
//
//    /* ignoring -Wpointer-sign is OK for a constant string */
//    print_to_console((uint8_t *)"\r\n");
//
//
//    while ((0 != c))
//    {
//        c = input_from_console ();
//        if (0 != c)
//        {
//            /* Cast, as compiler will assume calc is int */
//            c = (int8_t) (c - '0');
//            g_selected_menu = c;
//
//            if ((c > 0) && (c <= menu_limit))
//            {
//                if(c == 2)
//                {
//                    camview_display_menu(s_bool_screen_test);
//                }
//                else
//                {
//                    s_menu_items[c - 1].p_func ();
//                }
//                break;
//            }
//        }
//
//    }

    /* Cast, as compiler will assume calc is int */
    return ((int8_t) (c - '0'));
}
/**********************************************************************************************************************
 End of function main_display_menu
 *********************************************************************************************************************/

