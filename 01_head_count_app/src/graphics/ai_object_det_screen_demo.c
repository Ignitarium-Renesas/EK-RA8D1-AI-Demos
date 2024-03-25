/**********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER Escreen_offset_xpRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE Escreen_offset_xpRESSLY DISCLAIMED. TO THE MAXIMUM
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
#include "FreeRTOSConfig.h"
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
#include "graphics/graphics.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"

#include "touch_ft5x06.h"
#include "gimp.h"

#include "bg_font_18_full.h"

#include "renesas_logo.c"
#include "ign_logo.c"
#include "head_count_title.c"
#include "log_macros.h"

extern uint32_t get_image_data(st_image_data_t ref);
extern bool_t in_transition(void);

extern void show_menu_icon();
extern void process_main_menu();
extern uint8_t 			 bsp_camera_out_rot_buffer565 [ BSP_CAM_WIDTH  * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL ];
extern st_ai_detection_point_t g_ai_detection[20];

extern uint32_t uart_head_count;
extern uint32_t uart_inference_time;



uint64_t object_detection_inference_time;

void  do_object_detection_screen(void);

/*
 * SIMULATED AI uses MAB image and should no longer be used
 */

void  do_object_detection_screen(void)
{
//    st_gimp_image_t * img = NULL;
    static uint64_t t1 = 0;
    /* Wait for vertical blanking period */
    graphics_wait_vsync();
    graphics_start_frame();

    // Use background image
    if(in_transition())
    {
        /* Wait for vertical blanking period */
    	// Clear display
		d2_clear(d2_handle, 0x000000);

		// Display IGN logo
		d2_setblitsrc(d2_handle, ign_logo.pixel_data, ign_logo.width, ign_logo.width, ign_logo.height, EP_SCREEN_MODE);
		d2_blitcopy(d2_handle,
				ign_logo.width, ign_logo.height,  // Source width/height
				(d2_blitpos) 0, 0, // Source position
				(d2_width) (ign_logo.width << 4), (d2_width) (ign_logo.height << 4), // Destination width/height
				(10 << 4), ((LCD_VPIX - ign_logo.height - 10) << 4),  // Destination position
				d2_tm_filter);

		// Display Renesas logo
		d2_setblitsrc(d2_handle, renesas_logo.pixel_data, renesas_logo.width, renesas_logo.width, renesas_logo.height, EP_SCREEN_MODE);

		d2_blitcopy(d2_handle,
		renesas_logo.width, renesas_logo.height,  // Source width/height
		(d2_blitpos) 0, 0, // Source position
		(d2_width) (renesas_logo.width << 4), (d2_width) (renesas_logo.height << 4), // Destination width/height
		((LCD_HPIX - renesas_logo.width - 10) << 4), ((LCD_VPIX - ign_logo.height - 10) << 4),  // Destination position
		d2_tm_filter);

		// Display Application Title
		d2_setblitsrc(d2_handle, head_count_title.pixel_data, head_count_title.width, head_count_title.width, head_count_title.height, EP_SCREEN_MODE);

		d2_blitcopy(d2_handle,
				head_count_title.width, head_count_title.height,  // Source width/height
				(d2_blitpos) 0, 0, // Source position
				(d2_width) (renesas_logo.width*3 << 4), (d2_width) (renesas_logo.height*3 << 4), // Destination width/height
				(250 << 4), (200 << 4),  // Destination position
				d2_tm_filter);

        // show model information
//        print_bg_font_18(d2_handle, 180, 260,  (char*)"Neural network - FastestDet", 1);

    }
    else
    {
        print_bg_font_18(d2_handle, 400, 710,  (char*)"Head Count", 1);
//        print_bg_font_18(d2_handle, 210, 700,  (char*)"Inference time", 1);
    	volatile t2 = get_timestamp() - t1;
    	t1 = get_timestamp();
    	/* RESOLUTION FROM CAMERA */
    	#define CAM_IMG_SIZE_X   320
    	#define CAM_IMG_SIZE_Y   240  // Trim the Right Hand Edge hiding corruption
    	// normal screen
    	#define CAM_LAYER_SIZE_X 476 // 000 --> LCD_VPIX
    	#define CAM_LAYER_SIZE_Y 360 // 000 --> 480

        d2_setblitsrc(d2_handle, bsp_camera_out_rot_buffer565, CAM_IMG_SIZE_Y, CAM_IMG_SIZE_Y, CAM_IMG_SIZE_X, d2_mode_rgb565);

        d2_blitcopy(d2_handle,
                240, 320,  // Source width/height
                (d2_blitpos) 0, 0, // Source position
                (d2_width) ((480) << 4), (d2_width) ((640) << 4), // Destination size width/height
				(d2_width) 0, (d2_width) 0,
                d2_tm_filter);

        bsp_camera_capture_image();

		{
			uint8_t head_count = 0;
#define DET_MODEL_IMG_SIZE_X 192
#define DET_MODEL_IMG_SIZE_Y 192
			for(int i=0; i<20; i++)
			{
				if(g_ai_detection[i].m_w == 0 || g_ai_detection[i].m_h == 0)
					continue;

				d2_point x = (g_ai_detection[i].m_x*640)/192;
				d2_point y = (g_ai_detection[i].m_y*480)/192;
				d2_point w = (g_ai_detection[i].m_w*640)/192;
				d2_point h = (g_ai_detection[i].m_h*480)/192;

				volatile d2_point left_bottom_x = (d2_point)(480-y);
				volatile d2_point left_bottom_y = (d2_point)(640-x-2);
				volatile d2_point right_top_x = (d2_point)(480-(y+h));
				volatile d2_point right_top_y = (d2_point)(640-(x+w)-2);
				d2_setcolor(d2_handle, 0, 0xFF0000);
				d2_renderline(d2_handle, (d2_point) ((right_top_x) << 4), (d2_point) (right_top_y << 4), (d2_point) ((left_bottom_x) << 4), (d2_point) ((right_top_y) << 4), (d2_point) (2 << 4), 0);
				d2_renderline(d2_handle, (d2_point) ((left_bottom_x) << 4), (d2_point) ((right_top_y) << 4), (d2_point) ((left_bottom_x) << 4), (d2_point) ((left_bottom_y) << 4), (d2_point) (2 << 4), 0);
				d2_renderline(d2_handle, (d2_point) ((left_bottom_x) << 4), (d2_point) ((left_bottom_y) << 4), (d2_point) ((right_top_x) << 4), (d2_point) ((left_bottom_y) << 4), (d2_point) (2 << 4), 0);
				d2_renderline(d2_handle, (d2_point) ((right_top_x) << 4), (d2_point) ((left_bottom_y) << 4), (d2_point) ((right_top_x) << 4), (d2_point) ((right_top_y) << 4), (d2_point) (2 << 4), 0);

				memset(&g_ai_detection[i], 0, sizeof(g_ai_detection[i]));
				head_count++;
			}

			// update string on display
			char num_str[2] = {'\0'};
			num_str[0] = '0' + head_count;
			print_bg_font_18(d2_handle, 50, 680,  (char*)num_str, 15);

			uint32_t time = object_detection_inference_time / 1000; // ms

//			char time_str[7] = {'0', '0', '0', ' ', 'm', 's', '\0'};
//			time_str[0] += (time / 100);
//			time_str[1] += (time / 10) % 10;
//			time_str[2] += time % 10;
//    		print_bg_font_18(d2_handle, 170, 720,  (char*)time_str, 1);

    		uart_head_count = head_count + 1;
//    		uart_inference_time = time + 1;


		}
    }

    /* Wait for previous frame rendering to finish, then finalize this frame and flip the buffers */
    d2_flushframe(d2_handle);
    graphics_end_frame();

}


