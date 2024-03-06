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
 * Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
/**********************************************************************************************************************
 * File Name    : led_screen_demo.c
 * Version      : .
 * Description  : The led demo screen display.
 *********************************************************************************************************************/

#include <math.h>

#include "FreeRTOS.h"
#include "FreeRTOSconfig.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

#include "common_utils.h"
#include "common_init.h"
#include "menu_camview.h"
#include "jlink_console.h"

#include "r_ioport.h"
#include "r_mipi_dsi_api.h"

#include "hal_data.h"
#include "dsi_layer.h"

#include "camera_layer.h"
#include "graphics\graphics.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#include "touch_ft5x06.h"
#include "gimp.h"

#define MAX_SPEED_REFRESH   1
#define REFRESH_RATE        8
#define RADIANS_CONSTANT    0.12
//#define RADIANS_CONSTANT    0.25


extern void  reset_transition(void);
extern bool_t in_transition(void);
extern void show_menu_icon();
extern void process_main_menu();
extern uint32_t get_image_data(st_image_data_t ref);
extern uint32_t get_sub_image_data(st_image_data_t ref, uint32_t sub_image);

extern char_t g_print_buffer[];

#define USE_BACKGROUND_IMAGE

uint16_t SELECTED= 0;


static double xf    = 420.0;
static double yf    = 420.0;

static double xef   = 0.0;
static double yef   = 0.0;

static double xcos  = 15.0;
static double ysin  = 15.0;

static double angle_radians = 15.0;

static double inner_length =  40.0;
static double outer_length = 100.0;

static d2_width green_ap      = 0;

typedef enum e_led_screen_indictor
{
    LED_SCREEN_RED,
    LED_SCREEN_GREEN,
    LED_SCREEN_BLUE,
} led_screen_indictor_t;

typedef struct
{
    led_screen_indictor_t  id;
    st_gimp_led_image_t   *simg;                  // sub_image_location
    double                 blink_center_x;        // blink indicator needle ceneter point x
    double                 blink_center_y;        // blink indicator needle ceneter point y
    d2_width               blink_dest_pos_x;      // image destination position x
    d2_width               blink_dest_pos_y;      // image destination position y
    d2_width               blink_position;        // blink indicator position in radians
    d2_width               brightness_dest_pos_x; // image destination position x
    d2_width               brightness_dest_pos_y; // image destination position y
    d2_width               brightness_position;   // brightness indicator position in screen pixels
} st_led_screen_indicator_def_t;

st_led_screen_indicator_def_t led_control[] =
{
 { LED_SCREEN_RED, NULL, 158.0, 288.0,  65,  38, 0,  74, 132, 0},  // RED
 { LED_SCREEN_RED, NULL, 428.0, 288.0,  65, 308, 0, 344, 132, 0} , // GREEN
 { LED_SCREEN_RED, NULL, 698.0, 288.0,  65, 578, 0, 614, 132, 0} , // BLUE
};

void  do_led_screen(void);

static d2_point brightness_offset = 0;

static st_gimp_led_image_t sdram_images[3] BSP_PLACE_IN_SECTION(".sdram") = {};

static void draw_brightness_indicaor(led_screen_indictor_t  active)
{
    // Coloured arc led brightness position
    d2_setcolor(d2_handle, 0, 0xf7f7f7);

    brightness_offset = led_control[active].brightness_position;

    d2_point start_x = led_control[active].brightness_dest_pos_x + brightness_offset;
    d2_point start_y = led_control[active].brightness_dest_pos_y + 0;

    d2_point pos_x = led_control[active].brightness_dest_pos_x + brightness_offset;
    d2_point pos_y = led_control[active].brightness_dest_pos_y + 40;

    d2_renderline(d2_handle, (((d2_point) start_y) << 4), (((d2_point) start_x) << 4), (((d2_point) pos_y) << 4), (((d2_point) pos_x) << 4), 7 << 4, 0);
}

static void draw_rate_indicaor(led_screen_indictor_t  active)
{
    // Coloured arc led frequency position
    d2_setcolor(d2_handle, 0, 0xf7f7f7);

    xf   = led_control[active].blink_center_x;
    yf   = led_control[active].blink_center_y;

    angle_radians = (led_control[active].blink_position * RADIANS_CONSTANT);

    // Calculate line angle
    xcos = cos(angle_radians);
    ysin = sin(angle_radians);

    // Calculate Line co-ordinates
    xef  = xf + (xcos * inner_length);
    yef  = yf + (ysin * inner_length);

//#define LINE_DEBUG
#ifndef LINE_DEBUG
    // Line form outer edge  to inner edge mode
    xf   = xf + (xcos * outer_length);
    yf   = yf + (ysin * outer_length);
#else
    // Line form center to inner edge mode
    xf   = xc;
    yf   = yc;
#endif

    d2_renderline(d2_handle, (((d2_point) yf) << 4), (((d2_point) xf) << 4), (((d2_point) yef) << 4), (((d2_point) xef) << 4), 7 << 4, 0);
}

static void animate_rate_indicator(led_screen_indictor_t active)
{
    green_ap = led_control[active].blink_position;

    green_ap += 1;

    if(green_ap == 53)
    {
        green_ap = 0;
    }

    if(green_ap == 32)
    {
        green_ap = 47;
    }

    led_control[active].blink_position = green_ap;
}

static d2_point fkit99 = 175;
static void animate_brightness_indicator(led_screen_indictor_t active)
{
    brightness_offset = led_control[active].brightness_position;

    brightness_offset++;
    brightness_offset++;
    brightness_offset++;

    if(brightness_offset > fkit99)
    {
        brightness_offset = 0;

    }

    led_control[active].brightness_position =  brightness_offset;
}

/* not needed
sprintf(g_print_buffer, "index [%02ld] angle_radians [%3.2f] Line from [%3.2f,%3.2f] to [%3.2f,%3.2f]\r\n",
        green_ap, angle_radians, xf,yf,xef,yef);
print_to_console(g_print_buffer);
*/


/*
 * RENDER ORDER
 * BACKGROUND
 *     !
 *     !
 *     V
 * FOREGROUND
 */

led_screen_indictor_t  active = LED_SCREEN_RED;
uint32_t               active_buffer = 0;
void  do_led_screen(void)
{
    st_gimp_image_t          *img = NULL;
    st_gimp_led_image_t     *simg = NULL;

    /* Wait for vertical blanking period */
    graphics_wait_vsync();
    graphics_start_frame();

    if(in_transition())
    {
        img = (st_gimp_image_t *)get_image_data(MODE_SLIDE_7);
        d2_setblitsrc(d2_handle, img->pixel_data, 480, 480, LCD_VPIX, EP_SCREEN_MODE);

        d2_blitcopy(d2_handle,
                480,LCD_VPIX,  // Source width/height
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((480) << 4), (d2_width) ((LCD_VPIX) << 4), // Destination width/height
                0, 0,  // Destination position
                d2_tm_filter);

        uint32_t active_sub_image = 0;
        st_gimp_led_image_t *image_tmp = NULL;

#if 1
        /* load from ospi and store in SDRAM */
        image_tmp =                 (st_gimp_led_image_t*) get_sub_image_data(GI_LED_GRAPHICS_SCREEN, active_sub_image);
        memcpy(&sdram_images[active_sub_image], image_tmp, sizeof (st_gimp_led_image_t));
        led_control[active_sub_image].simg =  &sdram_images[active_sub_image];
        active_sub_image++;

        image_tmp =                 (st_gimp_led_image_t*) get_sub_image_data(GI_LED_GRAPHICS_SCREEN, active_sub_image);
        memcpy(&sdram_images[active_sub_image], image_tmp, sizeof (st_gimp_led_image_t));
        led_control[active_sub_image].simg =  &sdram_images[active_sub_image];
        active_sub_image++;

        image_tmp =                 (st_gimp_led_image_t*) get_sub_image_data(GI_LED_GRAPHICS_SCREEN, active_sub_image);
        memcpy(&sdram_images[active_sub_image], image_tmp, sizeof (st_gimp_led_image_t));
        led_control[active_sub_image].simg =  &sdram_images[active_sub_image];
        active_sub_image++;
#else

/* Load from ospi read from ospi
        led_control[active_sub_image].simg =  (st_gimp_led_image_t*) get_sub_image_data(GI_LED_GRAPHICS_SCREEN, active_sub_image);
        active_sub_image++;
        active_sub_image++;
        led_control[active_sub_image].simg =  (st_gimp_led_image_t*) get_sub_image_data(GI_LED_GRAPHICS_SCREEN, active_sub_image);
        active_sub_image++;
        led_control[active_sub_image].simg =  (st_gimp_led_image_t*) get_sub_image_data(GI_LED_GRAPHICS_SCREEN, active_sub_image);
        active_sub_image++;
*/
#endif

        active = LED_SCREEN_RED;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);
        draw_rate_indicaor(active);
        draw_brightness_indicaor(active);

        active = LED_SCREEN_GREEN;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);
        draw_rate_indicaor(active);
        draw_brightness_indicaor(active);

        active = LED_SCREEN_BLUE;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);
        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);
        draw_rate_indicaor(active);
        draw_brightness_indicaor(active);

        show_menu_icon();

    }
    else
    {
        active = LED_SCREEN_RED;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);

        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);

        active = LED_SCREEN_GREEN;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);

        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);

        active = LED_SCREEN_BLUE;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);

        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);

        if (active_buffer < 40)
        {
            active = LED_SCREEN_RED;
        }
        else if (active_buffer < 80)
        {
            active = LED_SCREEN_GREEN;
        }
        else
        {
            active = LED_SCREEN_BLUE;
        }

        active_buffer = (active_buffer+1) % 120;

//        draw_rate_indicaor(active);
        animate_rate_indicator(active);
        draw_rate_indicaor(LED_SCREEN_RED);
        draw_rate_indicaor(LED_SCREEN_GREEN);
        draw_rate_indicaor(LED_SCREEN_BLUE);

//        draw_brightness_indicaor(active);
        draw_brightness_indicaor(LED_SCREEN_RED);
        draw_brightness_indicaor(LED_SCREEN_GREEN);
        draw_brightness_indicaor(LED_SCREEN_BLUE);
        animate_brightness_indicator(active);

        show_menu_icon();

#if 0
        // RED
        active = LED_SCREEN_RED;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);

        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);

        draw_rate_indicaor(active);
        draw_brightness_indicaor(active);
        animate_rate_indicator(active);
        animate_brightness_indicator(active);

        // GREEN
        active = LED_SCREEN_GREEN;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);

        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);

        draw_rate_indicaor(active);
        draw_brightness_indicaor(active);

        // BLUE
        active = LED_SCREEN_BLUE;
        simg = led_control[active].simg;

        d2_setblitsrc(d2_handle, simg->pixel_data, (d2_s32)simg->width, (d2_s32)simg->width, (d2_s32)simg->height, EP_SCREEN_MODE);

        d2_blitcopy(d2_handle,
                (d2_s32)simg->width, (d2_s32)simg->height,
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((simg->width) << 4), (d2_width) ((simg->height) << 4), // Destination width/height
                (d2_width) ((led_control[active].blink_dest_pos_x) << 4), (d2_width) ((led_control[active].blink_dest_pos_y) << 4),  // Destination position
                d2_tm_filter);

        draw_rate_indicaor(active);
        draw_brightness_indicaor(active);
#endif


          show_menu_icon();
          process_main_menu();

        }

    /* Reset alpha in case it was changed above */
    d2_setalpha(d2_handle, 0xFF);

    /* Wait for previous frame rendering to finish, then finalize this frame and flip the buffers */
    d2_flushframe(d2_handle);
    graphics_end_frame();
}





