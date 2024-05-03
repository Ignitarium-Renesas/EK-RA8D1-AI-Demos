/*
 * camera_layer.c
 */


#include "camera_layer.h"
#include "hal_data.h"
#include "board_cfg.h"
#include "r_ceu.h"

#include "r_capture_api.h"

#define REG_CLKRC   0x3011
#define REG_PIDH    0x300a
#define REG_PIDL    0x300b
#define REG_SYS     0x3012
#define SYS_RESET   0x80

#define CAM_PWDN_PIN            BSP_IO_PORT_07_PIN_04
#define BSP_I2C_SLAVE_ADDR_CAMERA        (0x3C)   //Slave address for OV3640 Camera Module

#define OV7740_REG_TABLE         (ov_reg_t *)OV7740_VGA_YUV422

//#define OV7740_REG_TABLE                 (ov_reg_t *)OV7740_VGA_YUV422
//#define JPEG_BUFFER_SIZE                 (8192)

// OV7740 registers
#define OV7740_PIDH                      (0x0A)
#define OV7740_PIDH_DEFAULT              (0x77)


#define OV7740_PIDL                      (0x0B)
#define OV7740_PIDL_DEFAULT              (0x42)

#define OV7740_REG0C_MAX_EXPOSURE_Pos    (1)
#define OV7740_REG0C_MAX_EXPOSURE_Msk    (0x3u << OV7740_REG0C_MAX_EXPOSURE_Pos) //(OV7740_REG0C) Max exposure = frame length - limit x 2
#define OV7740_REG0C_MAX_EXPOSURE(value) ((OV7740_REG0C_MAX_EXPOSURE_Msk & ((value) << OV7740_REG0C_MAX_EXPOSURE_Pos)))

#define OV7740_REG0C_MIRROR_ENABLE       (0x1u << 6) /**< \brief (OV7740_REG0C) Mirror enable */
#define OV7740_REG0C_FLIP_ENABLE         (0x1u << 7) /**< \brief (OV7740_REG0C) Flip enable */
#define OV7740_REG0C_YUV_SWAP_ENABLE     (0x0u << 4) /**< \brief (OV7740_REG0C) output UYVYUYVY */
#define OV7740_REG0C_YUV_BIT_SWAP        (0x1u << 3) /**< \brief (OV7740_REG0C) output [Y2,Y3…Y8,Y9,Y1,Y0] */

#define RANGE_LIMIT(x)        (x > 255 ? 255 : (x < 0 ? 0 : x))

static void bsp_camera_write_array(st_ov_reg_t *array);

static st_ov_reg_t ov3640_fmt_yuv422_vga[] = {
    {0x3002, 0x06 }, {0x3003, 0x1F }, {0x3001, 0x12 }, {0x304d, 0x45 },
    {0x30aa, 0x45 }, {0x30B0, 0xff }, {0x30B1, 0xff }, {0x30B2, 0x10 },
    {0x30d7, 0x10 }, {0x3047, 0x00 }, {0x3018, 0x60 }, {0x3019, 0x58 },
    {0x301A, 0xa1 }, {0x3087, 0x02 }, {0x3082, 0x20 }, {0x303C, 0x08 },
    {0x303d, 0x18 }, {0x303e, 0x06 },
    {0x303f, 0x0c }, {0x3030, 0x62 }, {0x3031, 0x26 }, {0x3032, 0xe6 },
    {0x3033, 0x6e }, {0x3034, 0xea }, {0x3035, 0xae }, {0x3036, 0xa6 },
    {0x3037, 0x6a }, {0x3015, 0x12 }, {0x3013, 0xfd }, {0x3104, 0x02 },
    {0x3105, 0xfd }, {0x3106, 0x00 }, {0x3107, 0xff }, {0x3308, 0xa5 },
    {0x3316, 0xff }, {0x3317, 0x00 }, {0x3087, 0x02 }, {0x3082, 0x20 },
    {0x3300, 0x13 }, {0x3301, 0xd6 }, {0x3302, 0xef }, {0x30B8, 0x20 },
    {0x30B9, 0x17 }, {0x30BA, 0x04 }, {0x30BB, 0x08 },

    {0x3507, 0x06 },
    {0x350a, 0x4f }, {0x3600, 0xc4 }, {0x332B, 0x00 }, {0x332D, 0x45 },
    {0x332D, 0x60 }, {0x332F, 0x03 },
    {0x3100, 0x02 }, {0x3304, 0xfc }, {0x3400, 0x00 }, {0x3404, 0x02 }, /* YUV422 */
    {0x3601, 0x01 }, {0x302a, 0x06 }, {0x302b, 0x20 },
    {0x300E, 0x32 }, {0x300F, 0x21 }, {0x3010, 0x21 }, {0x3011, 0x01 }, /* QXGA PLL setting*/
    {0x304c, 0x81 },
    {0x3602, 0x22 }, {0x361E, 0x00 }, {0x3622, 0x18 }, {0x3623, 0x69 }, /* CSI setting */
    {0x3626, 0x00 }, {0x3627, 0xf0 }, {0x3628, 0x00 }, {0x3629, 0x26 },
    {0x362A, 0x00 }, {0x362B, 0x5f }, {0x362C, 0xd0 }, {0x362D, 0x3c },
    {0x3632, 0x10 }, {0x3633, 0x28 }, {0x3603, 0x4d }, {0x364C, 0x04 },
    {0x309e, 0x00 },
    {0x3020, 0x01 }, {0x3021, 0x1d }, {0x3022, 0x00 }, {0x3023, 0x0a }, /* crop window setting*/
    {0x3024, 0x08 }, {0x3025, 0x18 }, {0x3026, 0x06 }, {0x3027, 0x0c },
    {0x335f, 0x68 }, {0x3360, 0x18 }, {0x3361, 0x0c },
    // {0x307c, 0x13 }, {0x3023, 0x09 }, {0x3090, 0xc0 }, // 11 Flip, 12 mirror, 13 mirror and flip
    {0x307c, 0x10 }, {0x3023, 0x0a }, {0x3090, 0xc0 }, // 11 Flip, 12 mirror, 13 mirror and flip
    {0x3362, 0x12 }, {0x3363, 0x88 }, {0x3364, 0xe4 }, {0x3403, 0x42 },  /* VGA */
    {0x3088, 0x02 }, {0x3089, 0x80 }, {0x308a, 0x01 }, {0x308b, 0xe0 },
    // {0x3355, 0x04 }, {0x3354, 0x01 }, {0x335e, 0x28 },      /* brightness */
    // {0x3355, 0x04 }, {0x335c, 0x20 }, {0x335d, 0x20 },      /* contrast */
    {0x3355, 0x04 }, {0x3354, 0x01 }, {0x335e, 0x05 },      /* brightness */
    {0x3355, 0x04 }, {0x335c, 0x10 }, {0x335d, 0x20 },      /* contrast */

    /* Disable Test Pattern Mode */
    {0x306c, 0x10}, // Disable color bar mode normal image
    {0x307b, 0x40}, // Disable colour bar pattern
    {0x307d, 0x20}, // Colour bar Disable

    /* End of file marker (0xFFFF) */
    {0xffff, 0x00ff}
};


static st_ov_reg_t ov3640_fmt_yuv422_vga_test_mode[] = {
    {0x3002, 0x06 }, {0x3003, 0x1F }, {0x3001, 0x12 }, {0x304d, 0x45 },
    {0x30aa, 0x45 }, {0x30B0, 0xff }, {0x30B1, 0xff }, {0x30B2, 0x10 },
    {0x30d7, 0x10 }, {0x3047, 0x00 }, {0x3018, 0x60 }, {0x3019, 0x58 },
    {0x301A, 0xa1 }, {0x3087, 0x02 }, {0x3082, 0x20 }, {0x303C, 0x08 },
    {0x303d, 0x18 }, {0x303e, 0x06 },
    {0x303f, 0x0c }, {0x3030, 0x62 }, {0x3031, 0x26 }, {0x3032, 0xe6 },
    {0x3033, 0x6e }, {0x3034, 0xea }, {0x3035, 0xae }, {0x3036, 0xa6 },
    {0x3037, 0x6a }, {0x3015, 0x12 }, {0x3013, 0xfd }, {0x3104, 0x02 },
    {0x3105, 0xfd }, {0x3106, 0x00 }, {0x3107, 0xff }, {0x3308, 0xa5 },
    {0x3316, 0xff }, {0x3317, 0x00 }, {0x3087, 0x02 }, {0x3082, 0x20 },
    {0x3300, 0x13 }, {0x3301, 0xd6 }, {0x3302, 0xef }, {0x30B8, 0x20 },
    {0x30B9, 0x17 }, {0x30BA, 0x04 }, {0x30BB, 0x08 },
    {0x307c, 0x12 }, {0x3023, 0x09 }, {0x3090, 0xc0 }, // 11 Flip, 12 mirror, 13 mirroe and flip
    {0x3507, 0x06 },
    {0x350a, 0x4f }, {0x3600, 0xc4 }, {0x332B, 0x00 }, {0x332D, 0x45 },
    {0x332D, 0x60 }, {0x332F, 0x03 },
    {0x3100, 0x02 }, {0x3304, 0xfc }, {0x3400, 0x00 }, {0x3404, 0x02 }, /* YUV422 */
    {0x3601, 0x01 }, {0x302a, 0x06 }, {0x302b, 0x20 },
    {0x300E, 0x32 }, {0x300F, 0x21 }, {0x3010, 0x21 }, {0x3011, 0x01 }, /* QXGA PLL setting*/
    {0x304c, 0x81 },
    {0x3602, 0x22 }, {0x361E, 0x00 }, {0x3622, 0x18 }, {0x3623, 0x69 }, /* CSI setting */
    {0x3626, 0x00 }, {0x3627, 0xf0 }, {0x3628, 0x00 }, {0x3629, 0x26 },
    {0x362A, 0x00 }, {0x362B, 0x5f }, {0x362C, 0xd0 }, {0x362D, 0x3c },
    {0x3632, 0x10 }, {0x3633, 0x28 }, {0x3603, 0x4d }, {0x364C, 0x04 },
    {0x309e, 0x00 },
    {0x3020, 0x01 }, {0x3021, 0x1d }, {0x3022, 0x00 }, {0x3023, 0x0a }, /* crop window setting*/
    {0x3024, 0x08 }, {0x3025, 0x18 }, {0x3026, 0x06 }, {0x3027, 0x0c },
    {0x335f, 0x68 }, {0x3360, 0x18 }, {0x3361, 0x0c },
    {0x3362, 0x12 }, {0x3363, 0x88 }, {0x3364, 0xe4 }, {0x3403, 0x42 },  /* VGA */
    {0x3088, 0x02 }, {0x3089, 0x80 }, {0x308a, 0x01 }, {0x308b, 0xe0 },
    {0x3355, 0x04 }, {0x3354, 0x01 }, {0x335e, 0x28 },      /* brightness */
    {0x3355, 0x04 }, {0x335c, 0x20 }, {0x335d, 0x20 },      /* contrast */

    /* Enable Test Pattern Mode */
    {0x306c, 0x00}, // Enable color bar mode
    {0x307b, 0x42}, // Enable colour bar pattern
    {0x307d, 0x80}, // Colour bar Enable

    /* End of file marker (0xFFFF) */
    {0xffff, 0x00ff}
};

//#define USE_DEBUG_BREAKPOINTS 1

static capture_event_t g_last_cam_event = CEU_EVENT_NONE;              ///< Event causing the callback
volatile bool g_capture_ready = false;
volatile bool image_processed = true;
#define USE_SDRAM (1)
#ifdef USE_SDRAM
uint8_t           bsp_camera_out_buffer [ BSP_CAM_WIDTH  * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL ] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t           bsp_camera_out_buffer565 [ BSP_CAM_WIDTH  * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL ] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t           bsp_camera_out_buffer888 [ BSP_CAM_WIDTH  * BSP_CAM_HEIGHT * 3 ] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t           bsp_det_model_ip_buffer888[ 224 * 224 * 3 ] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
//uint8_t           bsp_det_crop_model_ip_buffer888[ 240 * 240 * 3 ] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
uint8_t           bsp_camera_out_rot_buffer565 [ 240  * 320 * BSP_CAM_BYTE_PER_PIXEL ] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);


//uint8_t           bsp_cls_model_ip_buffer888[ 224 * 224 * 3 ] BSP_PLACE_IN_SECTION(".sdram") BSP_ALIGN_VARIABLE(8);
//uint8_t           bsp_cls_model_ip_buffer888[ 224 * 224 * 3 ] BSP_ALIGN_VARIABLE(8);


#else
uint8_t           bsp_camera_out_buffer [ BSP_CAM_WIDTH  * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL ]  BSP_ALIGN_VARIABLE(8);
uint8_t           bsp_camera_out_buffer565 [ BSP_CAM_WIDTH  * BSP_CAM_HEIGHT * BSP_CAM_BYTE_PER_PIXEL ] BSP_ALIGN_VARIABLE(8);
uint8_t           bsp_camera_out_buffer888 [ BSP_CAM_WIDTH  * BSP_CAM_HEIGHT * 3 ] BSP_ALIGN_VARIABLE(8);
uint8_t           bsp_det_model_ip_buffer888   [ 224 * 224 * 3 ] BSP_ALIGN_VARIABLE(8);
uint8_t           bsp_rec_model_ip_buffer888   [ 224 * 224 * 3 ] BSP_ALIGN_VARIABLE(8);
#endif

static void bsp_camera_write_array(st_ov_reg_t *array)
{
#if USE_DEBUG_BREAKPOINTS
    uint8_t value;
    R_BSP_PinAccessEnable();
#endif


    while (0xFFFF != array->reg_num)
    {
        wrSensorReg16_8(array->reg_num, array->value);

#if USE_DEBUG_BREAKPOINTS
        rdSensorReg16_8(array->reg_num, &value);

        if(value == array->value)
        {
            TURN_GREEN_ON;
//            printf_colour ("Write ADDR:[0x%04x] Data:[[GREEN]0x%02x[WHITE]]\r\n",array->reg_num,array->value);
        }
        else
        {
            TURN_RED_ON;
            __BKPT(0);

//            printf_colour ("Write ADDR:[0x%04x] Data:[[RED]0x%02x[WHITE]]\r\n",array->reg_num,array->value);
        }
#endif
        array++;
     }
#if USE_DEBUG_BREAKPOINTS
    TURN_RED_OFF;
    TURN_GREEN_OFF;

    R_BSP_PinAccessDisable();
#endif
}



void new_frame()
{

#if 0
    display_input_cfg_t const *p_input = &g_mipi_dsi0_cfg.p_display_instance->p_cfg->input[0];
    uint8_t * buf_ptr = (uint8_t *)fb_background[1];
    uint8_t *p_ref = &bsp_camera_out_buffer888;
    uint8_t *p_start = &bsp_camera_out_buffer888 + 0;

    /* Convert format */
    p_ref = p_start;
    uint32_t bpp = 4;
//    uint32_t img_height = 320;
//    uint32_t img_width = 240;
    uint32_t img_height = 854;
    uint32_t img_width = 480;
    volatile size_t href = 0;

    for(uint32_t v = 0; v < img_height; v++)
    {
        size_t o    = 1;
        href = 0;
        for(uint32_t h = 0; h < img_width*bpp; h+=bpp)
        {
            uint32_t v_offset = (((img_width - o) * img_height) + v - 1) * 3;
            o++;

#ifdef USE_CAM_IMAGE
            buf_ptr[0] = *(p_ref + v_offset + 2); // B
            buf_ptr[1] = *(p_ref + v_offset + 1); // G
            buf_ptr[2] = *(p_ref + v_offset + 0); // R
#else
            if ((href > 150) && (href < 250))
            {
                buf_ptr[0] = 100; // B
                buf_ptr[1] = 150; // G
                buf_ptr[2] = 250; // R
            }
            else
            {
                buf_ptr[0] = 0; // B
                buf_ptr[1] = 0; // G
                buf_ptr[2] = (50 + href); // R
            }

#endif
            buf_ptr[3] = 0;                                     // A (Unused)
            buf_ptr+=4;
            href++;
        }
        buf_ptr += ((p_input->hstride - p_input->hsize) * 4);
    }

    uint8_t * live_ptr  = (uint8_t *)fb_background[1];
    uint8_t * upd_ptr   = (uint8_t *)fb_background[0];

    memcpy(upd_ptr, live_ptr, (DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0));
#endif
}


void bsp_camera_yuv422_to_rgb565(const void* inbuf, void* outbuf, uint16_t width, uint16_t height)
{
    uint32_t rows, columns;
    int32_t  y, u, v, r, g, b;
    uint8_t  *yuv_buf;
    uint16_t *rgb_buf = (uint16_t *) outbuf;
    uint32_t y_pos,u_pos,v_pos;

    yuv_buf = (uint8_t *)inbuf;
    uint32_t x_start, y_start;
    int32_t temp;
    uint16_t pixel_data;

  //  SCB_EnableDCache();

    x_start = 0;
    y_start = 0;

    y_pos = 1; // 0 1
    u_pos = 0; // 1 0
    v_pos = 2; // 3 2

    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            // Extract pixel Y U V byte from buffer
            y = yuv_buf[y_pos];
            u = yuv_buf[u_pos] - 128;
            v = yuv_buf[v_pos] - 128;

            //   Formula to Convert YUV422 to RGB888
            //   R = Y + 1.403V'
            //   G = Y - 0.344U' - 0.714V'
            //   R = Y + 1.770U'

            // R conversion
            temp = (int32_t) ( y + v + ( (v * 103) >> 8 ) ) ;
            r = (int32_t) RANGE_LIMIT( temp );

            // G Conversion
            temp = (int32_t) ( y - ( (u * 88) >> 8 ) - ( (v * 183) >> 8 ) );
            g = (int32_t) RANGE_LIMIT( temp );

            // B Conversion
            temp = (int32_t)  ( y + u + ( (u * 198) >> 8 ) );
            b = (int32_t) RANGE_LIMIT( temp );

            // RGB rearrange & merge back into RGB565 pixel
            pixel_data = (uint16_t) ( ( (r & 0xF8) << 8 ) | ( (g & 0xFC) << 3 ) | ( (b & 0xF8) >> 3 ) );

            // Display pixel directly into the screen working buffer
            rgb_buf[ ( ( rows + y_start ) * BSP_CAM_WIDTH ) - ( columns + x_start) ] = pixel_data;

            // Move to next pixel
            y_pos += 2;

            // Move to next set of UV
            if (columns & 0x01)
            {
                u_pos += 4;
                v_pos += 4;
            }
        }
    }

  //  SCB_DisableDCache();
}



void bsp_camera_yuv422_to_rgb888(const void* inbuf, void* outbuf, uint16_t width, uint16_t height)
{
    uint32_t rows, columns;
    int32_t  y, u, v, r, g, b;
    uint8_t  *yuv_buf;
    uint8_t *rgb_buf = (uint8_t *) outbuf;
    uint32_t y_pos,u_pos,v_pos;
    uint32_t d_pos = 0;
    int32_t temp;

    yuv_buf = (uint8_t *)inbuf;

  //  SCB_EnableDCache();

    y_pos = 1; // 0 1
    u_pos = 0; // 1 0
    v_pos = 2; // 3 2

    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            // Extract pixel Y U V byte from buffer
            y = yuv_buf[y_pos];
            u = yuv_buf[u_pos] - 128;
            v = yuv_buf[v_pos] - 128;

            //   Formula to Convert YUV422 to RGB888
            //   R = Y + 1.403V'
            //   G = Y - 0.344U' - 0.714V'
            //   R = Y + 1.770U'

            // R conversion
            temp = (int32_t) ( y + v + ( (v * 103) >> 8 ) ) ;
            r = (int32_t) RANGE_LIMIT( temp );

            // G Conversion
            temp = (int32_t) ( y - ( (u * 88) >> 8 ) - ( (v * 183) >> 8 ) );
            g = (int32_t) RANGE_LIMIT( temp );

            // B Conversion
            temp = (int32_t)  ( y + u + ( (u * 198) >> 8 ) );
            b = (int32_t) RANGE_LIMIT( temp );

            rgb_buf[d_pos + 0] = (uint8_t)r;
            rgb_buf[d_pos + 1] = (uint8_t)g;
            rgb_buf[d_pos + 2] = (uint8_t)b;

            // Move to next pixel
            y_pos += 2;
            d_pos += 3;

            // Move to next set of UV
            if (columns & 0x01)
            {
                u_pos += 4;
                v_pos += 4;
            }
        }
    }

  //  SCB_DisableDCache();
}


bool_t camera_init(bool_t use_test_mode)
{
    fsp_err_t status = FSP_ERR_NOT_OPEN;
    byte reg_val1 = 0;
    byte reg_val2 = 0;
    bool_t initialised_state = false;

    R_GPT_Open(&g_cam_clk_ctrl, &g_cam_clk_cfg);
    R_GPT_Start(&g_cam_clk_ctrl);

    // Hardware reset module
    R_BSP_PinAccessEnable();
    R_IOPORT_PinWrite(&g_ioport_ctrl, CAM_PWDN_PIN, (bsp_io_level_t)POWER_DOWN);
    R_BSP_SoftwareDelay(40, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, CAM_PWDN_PIN, (bsp_io_level_t)POWER_UP);
    R_BSP_PinAccessDisable();

    i2c_master_status_t i2c_status;
    R_IIC_MASTER_StatusGet (&g_i2c_master1_ctrl, &i2c_status);

    /* only open if not opened */
    if(i2c_status.open != true) // I2c not open
    {
        status = R_IIC_MASTER_Open(&g_i2c_master1_ctrl, &g_i2c_master1_cfg);
    }

    R_IIC_MASTER_SlaveAddressSet( &g_i2c_master1_ctrl, BSP_I2C_SLAVE_ADDR_CAMERA, I2C_MASTER_ADDR_MODE_7BIT);
    rdSensorReg16_8(REG_PIDH, &reg_val1); // PIDH  PID MSB
    rdSensorReg16_8(REG_PIDL, &reg_val2); // PIDH  PID LSB REV2c - 0x4C, REV2a = 0x41, REV1a=0x40 otherwise error

    if ((reg_val2 == 0x40) || (reg_val2 == 0x41) || (reg_val2 == 0x4C))
    {
        // Valid Camera


        // Open camera module
        status = R_CEU_Open(&g_ceu_ctrl, &g_ceu_cfg);

        if(use_test_mode == false)
        {
            /* VGA TEST PATTERN */
            bsp_camera_write_array((st_ov_reg_t *)&ov3640_fmt_yuv422_vga_test_mode);
        }
        else
        {
            /* LIVE DATA */
            bsp_camera_write_array((st_ov_reg_t *)&ov3640_fmt_yuv422_vga);
        }
        initialised_state = true;
    }

    return (initialised_state);
}

/*
void rot90_clock(uint8_t* input_image, uint8_t* output_image, int n_ch, int ip_w, int ip_h)
{
    for (int row=0; row<ip_h; row++)
    {
        for(int col=0; col<ip_w; col++)
        {
            int src_idx = row*ip_w*n_ch + col*n_ch;
            int dst_idx = (ip_h-row-1)*n_ch + col*ip_h*n_ch;
            int *d_ptr = output_image + dst_idx;
            int *s_ptr = input_image + src_idx;

            for(int ch=0; ch<n_ch; ch++)
            {
            	//*d_ptr++ = *s_ptr++;
                output_image[dst_idx+ch] = input_image[src_idx+ch];
            }
        }

    }
}*/
void rot90_clock(uint8_t* input_image, uint8_t* output_image, int n_ch, int ip_w, int ip_h) {
    // Make sure n_ch is 2 for RGB565
    if (n_ch != 2) {
        return;
    }

    for (int y = 0; y < ip_h; y++) {
        for (int x = 0; x < ip_w; x++) {
            // For each pixel, find its new position after a 90 degree rotation
            int newX = ip_h - 1 - y;
            int newY = x;

            // Calculate source and destination offsets
            int srcOffset = (y * ip_w + x) * n_ch;
            int dstOffset = (newY * ip_h + newX) * n_ch;

            // Copy the RGB565 value from input to output image
            output_image[dstOffset] = input_image[srcOffset];
            output_image[dstOffset + 1] = input_image[srcOffset + 1];
        }
    }
}

uint32_t adjust_brightness(void* inbuf, uint8_t* rgb2_buf, uint32_t width, uint32_t height) {
  // Calculate the average brightness of the image.
    uint32_t rows, columns;
    int32_t  y;
    uint8_t  *yuv_buf = (uint8_t *)inbuf;
    uint32_t y_pos = 1;
    volatile float average_brightness = 0.0f;
    volatile float brightness_factor = 1.0f;
    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            y = yuv_buf[y_pos];
            average_brightness += y;
            y_pos += 2;
        }
    }
    /*Average brightbness calculation*/
    average_brightness /= (width * height);
    y_pos = 1;
    /*Brightness adjustment*/
    if(average_brightness > 230)
        brightness_factor = 0.3f;
    else if(average_brightness > 190 &&  average_brightness <=230)
        brightness_factor = 0.4f;
    else if(average_brightness > 150 &&  average_brightness <=190)
        brightness_factor = 0.6f;
    else if(average_brightness > 130 &&  average_brightness <=150)
        brightness_factor = 0.8f;
    else if(average_brightness > 100 &&  average_brightness <=130)
        brightness_factor = 1.5f;
    else if(average_brightness > 80 &&  average_brightness <=100)
        brightness_factor = 1.8f;
    else if(average_brightness > 60 &&  average_brightness <=80)
            brightness_factor = 2.4f;
    else if(average_brightness > 40 &&  average_brightness <=60)
        brightness_factor = 3.0f;
    else if(average_brightness <=40)
        brightness_factor = 3.5f;

    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            // Extract pixel Y U V byte from buffer
            y = yuv_buf[y_pos];
            y = y * brightness_factor;
            y = y > 255 ? 255 : y;
            //y = y < 0 ? 0 : y;
            rgb2_buf[y_pos] = y;
            // Move to next pixel
            y_pos += 2;
        }
    }
    return average_brightness;
}

void bsp_camera_capture_image(void)
{
    fsp_err_t   err;

    g_capture_ready = false;
    err = R_CEU_CaptureStart(&g_ceu_ctrl, (uint8_t * const )&bsp_camera_out_buffer);

#if USE_DEBUG_BREAKPOINTS
    if (FSP_SUCCESS != err)
    {
        __BKPT(0);
    }
#else
    (void) err;
#endif

    // Wait for capture to finish
    while(!g_capture_ready);
    SCB_CleanDCache();
    SCB_EnableDCache();
    adjust_brightness(bsp_camera_out_buffer, bsp_camera_out_buffer, BSP_CAM_WIDTH, BSP_CAM_HEIGHT);
    bsp_camera_yuv422_to_rgb565(bsp_camera_out_buffer, bsp_camera_out_buffer565, BSP_CAM_WIDTH, BSP_CAM_HEIGHT);
    bsp_camera_yuv422_to_rgb888(bsp_camera_out_buffer, bsp_camera_out_buffer888, BSP_CAM_WIDTH, BSP_CAM_HEIGHT);

    rot90_clock(bsp_camera_out_buffer565, bsp_camera_out_rot_buffer565, 2, BSP_CAM_WIDTH, BSP_CAM_HEIGHT);
    SCB_DisableDCache();
    SCB_CleanDCache();
    g_capture_ready = false;
}

void g_ceu_user_callback (capture_callback_args_t * p_args)
{

    if (CEU_EVENT_FRAME_END == p_args->event )
    {
        g_capture_ready = true;
    }
    g_last_cam_event = p_args->event;
}
