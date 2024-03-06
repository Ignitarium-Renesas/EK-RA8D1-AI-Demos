#include <math.h>
#include "graphics.h"
#include "hal_data.h"
#include "dave_driver.h"
//#include "cachel1_armv7.h"


#include "common_utils.h"
#include "common_init.h"
#include "jlink_console.h"

#include "r_ioport.h"
#include "r_mipi_dsi_api.h"

#include "hal_data.h"
#include "dsi_layer.h"

#include "r_glcdc.h"
#include "r_glcdc_cfg.h"


/******************************************************************************
 * Static global variables
 *****************************************************************************/
uint8_t drw_buf = 0;
static volatile uint32_t g_vsync_flag = 0;
//static d2_renderbuffer * renderbuffer;

static char_t g_print_buffer[BUFFER_LINE_LENGTH] = {};

/******************************************************************************
 * Extern global variables
 *****************************************************************************/
extern d2_device * d2_handle;

/******************************************************************************
 * Function definitions
 *****************************************************************************/

display_runtime_cfg_t glcd_layer_change;

bool update_frame = false;
static float precision_x = 0;
static float precision_y = 0;


void glcdc_vsync_isr(display_callback_args_t *p_args);
void glcdc_init();

//static unsigned char decompressed[480 * 858 * 2 + 1] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(".sdram");

void glcdc_init()
{
    /* Seed rand - rand is used by this demo for image location calculations */
    uint32_t data;

    display_input_cfg_t const *p_input   = &g_display0.p_cfg->input[0];
    display_output_cfg_t const *p_output = &g_display0.p_cfg->output;

//    display_input_cfg_t const *p_input = &g_mipi_dsi0_cfg.p_display_instance->p_cfg->input[0];
//    display_output_cfg_t const *p_output = &g_mipi_dsi0_cfg.p_display_instance->p_cfg->output;

    memset(&fb_background[0][0], 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);
    memset(&fb_background[1][0], 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);

#if 0
    if(0) // bespoke test pattern
    {
        // xxRRGGBB
        #define RGB_BLUE     0x000000FF
        #define RGB_GREEN    0x0000FF00
        #define RGB_RED      0x00FF0000
        #define RGB_BLACK    0x00000000
        #define RGB_WHITE    0x00FFFFFF

        uint32_t * buf_ptr = (uint32_t *)graphics_get_draw_buffer();

        display_input_cfg_t const *p_input = &g_mipi_dsi0_cfg.p_display_instance->p_cfg->input[0];

        uint32_t bit_width= p_input->hsize/12;
        for(uint32_t y = 0; y < p_input->vsize; y++)
        {
            for(uint32_t x = 0; x < p_input->hsize; x++)
            {
                uint32_t colors[4]={RGB_BLUE, RGB_GREEN, RGB_RED, RGB_RED};
                uint32_t bit = x/bit_width;
                /* encode row number in the colors...left color[2], right color[3], 0=colors[0], 1=colors[1] */
                if ((bit) == 0)
                    buf_ptr[x]=colors[2];
                else if (bit >= 11)
                {
                    //buf_ptr[x]=colors[3];
                    buf_ptr[x] = colors[((y*4)/p_input->vsize) % 4];
                }
                else // 1..10
                {
                    buf_ptr[x] = ((y & (1 << (10 - bit))) == 0) ? colors[0] : colors[1];
                }
            }
            buf_ptr += p_input->hstride;
        }

        memcpy(&image_src[0],&fb_background[0][0], DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);

        memset(&fb_background[0][0], 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);
        memset(&fb_background[1][0], 0, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0);
    }
    else
    {
//#include "gi_startup565.c"
//#include "gi_main_cmp.c"
#include "gi_font_text.c"

        uint8_t * buf_ptr = (uint8_t *)fb_background;

        /* Convert format */
        memset(buf_ptr, 0, p_input->hstride * p_input->vsize * 4);
        memset(&image_src, 0, 480 * 858 * 2);

#if 1
        size_t img_size = (DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0)/2;
        uint32_t bpp = gimp_image.bytes_per_pixel;

        //    GIMP_IMAGE_RUN_LENGTH_DECODE(image_buf, rle_data, size, bpp)
//        GIMP_IMAGE_RUN_LENGTH_DECODE(&image_src, &gimp_image.rle_pixel_data, img_size, bpp);


        for(uint32_t v = 0; v < gimp_image.height; v++)
        {
            uint32_t v_offset = v*gimp_image.width*bpp;

#if 1 // RGB565
            // RED    0xF800 - LE 00F8
            // GREEN  0x07E0 - LE E070
            // BLUE   0x00F8 - LE F800

            for(uint32_t h = 0; h < gimp_image.width*bpp; h+=bpp)
                {
#if 1 // UNCOMPRESSED
                    buf_ptr[0] = gimp_image.pixel_data[v_offset+h+0]; //
                    buf_ptr[1] = gimp_image.pixel_data[v_offset+h+1]; //
#else
                    buf_ptr[0] = decompressed[v_offset+h+0]; //
                    buf_ptr[1] = decompressed[v_offset+h+1]; //
#endif
                    buf_ptr+=2;
                }
            buf_ptr += ((p_input->hstride - p_input->hsize) * 2);

#else  // RGB888

            for(uint32_t h = 0; h < cat.width*bpp; h+=bpp)
                {
                buf_ptr[0] = cat.pixel_data[v_offset+h+2]; // B
                buf_ptr[1] = cat.pixel_data[v_offset+h+1]; // G
                buf_ptr[2] = cat.pixel_data[v_offset+h];   // R
                buf_ptr[3] = 0;                                     // A (Unused)
                buf_ptr+=4;
                }
            buf_ptr += ((p_input->hstride - p_input->hsize) * 4);
#endif
        }
#endif
    }
#endif

    /* copy the data to runtime - for GLCDC layer change */

    glcd_layer_change.input = g_display0.p_cfg->input[0];

    glcd_layer_change.layer = g_display0.p_cfg->layer[0];

      /* Center image */
     precision_x = (int16_t)(p_output->htiming.display_cyc - p_input->hsize) / 2;
     precision_y = (int16_t)(p_output->vtiming.display_cyc - p_input->vsize) / 2;
     glcd_layer_change.layer.coordinate.x = (int16_t)precision_x;
     glcd_layer_change.layer.coordinate.y = (int16_t)precision_y;

     (void)R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_1);
}

void glcdc_update_layer_position()
{
}

/* Draw 15 frames before enabling backlight to prevent flash of white light on screen */
static uint8_t g_draw_count = 0;
#define DIM_FRAME_COUNT_LIMIT (15)

void reenable_backlight_control(void)
{
    g_draw_count = 0;
}

void glcdc_vsync_isr(display_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    switch (p_args->event)
    {
        case DISPLAY_EVENT_GR1_UNDERFLOW:
            TURN_RED_ON;
            break;

        case DISPLAY_EVENT_GR2_UNDERFLOW:
// LAYER 2 NOT in use so ignore this
//            TURN_BLUE_ON;
            break;

        default:
//            TURN_BLUE_OFF;
            TURN_RED_OFF;
    }

    if(g_draw_count < DIM_FRAME_COUNT_LIMIT)
    {
        g_draw_count++;
        if(g_draw_count == 15)
        {
            dsi_layer_enable_backlight();
        }
    }

    update_frame = true;
    g_vsync_flag = 1;
    return;
}

/*******************************************************************************************************************//**
 * Simple function to wait for vertical sync semaphore
 **********************************************************************************************************************/
void graphics_wait_vsync()
{
    g_vsync_flag = 0;
   // while(!g_vsync_flag);
    g_vsync_flag = 0;

}

/*******************************************************************************************************************//**
 * Initialize D/AVE 2D driver and graphics LCD controller
 **********************************************************************************************************************/
void graphics_init (void)
{

}

/*******************************************************************************************************************//**
 * Get address of the buffer to use when setting the D2 framebuffer
 * 
 * NOTE: The returned address is technically a pointer to the currently displaying buffer.  By the time the DRW engine
 * starts drawing to it it will no longer be the active frame.
 * 
 * @retval     void *     Draw buffer pointer to use with d2_framebuffer
 **********************************************************************************************************************/
void * graphics_get_draw_buffer()
{
    return &(fb_background[drw_buf][0]);

}

/*******************************************************************************************************************//**
 * Swap the active buffer in the graphics LCD controller
 **********************************************************************************************************************/
void graphics_swap_buffer()
{
#if LCD_BUF_NUM > 1
    drw_buf = drw_buf ? 0 : 1;
#endif

    /* Update the layer to display in the GLCDC (will be set on next Vsync) */
//    R_GLCDC_BufferChange(g_display0.p_ctrl, fb_background[0], 0); // fixed
    R_GLCDC_BufferChange(g_display0.p_ctrl, fb_background[drw_buf], 0); // double buffered
    R_GLCDC_LayerChange(&g_display0.p_ctrl, &glcd_layer_change, DISPLAY_FRAME_LAYER_1);
}

/*******************************************************************************************************************//**
 * Start a new display list, set the framebuffer and add a clear operation
 * 
 * This function will automatically prepare an empty framebuffer.
 **********************************************************************************************************************/
void graphics_start_frame()
{
    /* Start a new display list */
    d2_startframe(d2_handle);

    /* Set the new buffer to the current draw buffer */
    d2_framebuffer(d2_handle, graphics_get_draw_buffer(), LCD_HPIX, LCD_STRIDE, LCD_VPIX, EP_SCREEN_MODE);
}

/*******************************************************************************************************************//**
 * End the current display list and flip the active framebuffer
 * 
 * WARNING: As part of d2_endframe the D2 driver will wait for the current frame to finish displaying.
 **********************************************************************************************************************/
void graphics_end_frame()
{
    /* End the current display list */
    d2_endframe(d2_handle);

    /* Flip the framebuffer */
    graphics_swap_buffer();

    /* Clean data cache */
    SCB_CleanDCache();
}

/*******************************************************************************************************************//**
 * Converts float HSV values (0.0F-1.0F) to RGB888
 *
 * @param[in]  h   Hue
 * @param[in]  s   Saturation
 * @param[in]  v   Value
 *
 * @retval     uint32_t    RGB888 color
 **********************************************************************************************************************/
uint32_t graphics_hsv2rgb888(float h, float s, float v) {

    uint32_t r, g, b;

    if(s < 0.003F) { //monochrome
        r = b = g = (uint16_t)(v*0xFF);
    }
    else {
        if(h>=1.0F) h = h - floorf(h);
        if(s>1.0F) s = s - floorf(s);
        if(v>1.0F) v = v - floorf(v);
        h *= 6.0F;
        switch((int)h) {
            case 0:
                r = (uint32_t)(0xFF*v);
                g = (uint32_t)(0xFF*v*(1.0F - s*(1.0F - h)));
                b = (uint32_t)(0xFF*v*(1.0F - s));
                break;
            case 1:
                r = (uint32_t)(0xFF*v*(1.0F - s*(h-1.0F)));
                g = (uint32_t)(0xFF*v);
                b = (uint32_t)(0xFF*v*(1.0F - s));
                break;
            case 2:
                r = (uint32_t)(0xFF*v*(1.0F - s));
                g = (uint32_t)(0xFF*(v));
                b = (uint32_t)(0xFF*v*(1.0F - s*(1.0F - (h-2.0F))));
                break;
            case 3:
                r = (uint32_t)(0xFF*v*(1.0F - s));
                g = (uint32_t)(0xFF*v*(1.0F - s*(h-3.0F)));
                b = (uint32_t)(0xFF*(v));
                break;
            case 4:
                r =(uint32_t)( 0xFF*v*(1.0F - s*(1.0F - (h-4.0F))));
                g =(uint32_t)( 0xFF*v*(1.0F - s));
                b =(uint32_t)( 0xFF*(v));
                break;
            case 5:
                r = (uint32_t)(0xFF*(v));
                g = (uint32_t)(0xFF*v*(1.0F - s));
                b = (uint32_t)(0xFF*v*(1.0F - s*(h-5.0F)));
                break;
            default:
                r = 0xFF;
                b = 0xFF;
                g = 0xFF;

        }
    }

    return (r<<16) + (g<<8) + b;
}
