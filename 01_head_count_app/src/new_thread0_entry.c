#include "new_thread0.h"
#include <stdbool.h>
#include "common_data.h"
#include "camera_layer.h"


usb_status_t event;
fsp_err_t status = FSP_SUCCESS;
#define NUM_STRING_DESCRIPTOR               (7U)
#define READ_BUF_SIZE               (8U)
#define LINE_CODING_LENGTH          (0x07U)

extern uint8_t g_apl_device[];
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_hs_configuration[];
extern uint8_t g_apl_qualifier_descriptor[];
extern uint8_t *g_apl_string_table[];

extern uint8_t cam_out_rgb_888_buff[];
extern uint8_t bsp_camera_out_buffer888[];
#define GREYSCALE_SIZE (BSP_CAM_WIDTH * BSP_CAM_HEIGHT)
#define USB_MAX_PACKET_SIZE	9600
#define IMAGE_SIZE	(320 * 240 * 3)
#define MAX_USB_ITERATIONS	(IMAGE_SIZE / USB_MAX_PACKET_SIZE)

uint32_t uart_head_count;
uint32_t uart_inference_time;
uint8_t *img_buf;

//static bool  b_usb_attach = false;
uint8_t g_buf[READ_BUF_SIZE]            = {0};
usb_event_info_t    event_info          = {0};
static usb_pcdc_linecoding_t g_line_coding;
uint32_t len = 0;
char* buff[8];

const usb_descriptor_t g_usb_descriptor =
{
 g_apl_device,                   /* Pointer to the device descriptor */
 g_apl_configuration,            /* Pointer to the configuration descriptor for Full-speed */
 g_apl_hs_configuration,         /* Pointer to the configuration descriptor for Hi-speed */
 g_apl_qualifier_descriptor,     /* Pointer to the qualifier descriptor */
 g_apl_string_table,             /* Pointer to the string descriptor table */
 NUM_STRING_DESCRIPTOR
};

void usb_init();
void myUsbCallback(usb_event_info_t *event, usb_hdl_t task_handle, usb_onoff_t status);
void read_cam();
extern fsp_err_t R_USB_Callback (usb_callback_t * p_callback);

static volatile usb_pcdc_ctrllinestate_t g_control_line_state = {
    .bdtr = 0,
    .brts = 0,
};

/* My Thread entry function */
/* pvParameters contains TaskHandle_t */
void new_thread0_entry(void *pvParameters) {
	FSP_PARAMETER_NOT_USED(pvParameters);
	usb_init();

	/* TODO: add your own code here */
	while (1) {
		vTaskDelay(1);
	}
}

// Function to initialize USB
void usb_init() {

    // Open USB
	status = R_USB_Open(&g_basic0_ctrl, &g_basic0_cfg);
	if(status != FSP_SUCCESS)
	{
		//printtoconsole->error
	}
	R_USB_Callback(&myUsbCallback);
}

// Define the callback function matching the usb_callback_t signature
void myUsbCallback(usb_event_info_t *event_info, usb_hdl_t task_handle, usb_onoff_t status)
{
	FSP_PARAMETER_NOT_USED(task_handle);
	FSP_PARAMETER_NOT_USED(status);
	event = event_info->event;

	fsp_err_t err;
    usb_setup_t setup;

    switch (event_info->event)
            {
                case USB_STATUS_CONFIGURED :
                	R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, USB_CLASS_PCDC);

                break;
                case USB_STATUS_WRITE_COMPLETE :
                  R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, USB_CLASS_PCDC);

                break;
                case USB_STATUS_READ_COMPLETE :
				if(g_buf[0] == 'c')
				{
					R_USB_Write (&g_basic0_ctrl, &uart_head_count, sizeof(uart_head_count), USB_CLASS_PCDC);
				}
				else if(g_buf[0] == 'i')
				{
					uint8_t buff_index = g_buf[1];

					if(g_buf[1] == 0)
					{
						read_cam();
					}
					if(buff_index <= MAX_USB_ITERATIONS)
					{
						err = R_USB_Write (&g_basic0_ctrl, &cam_out_rgb_888_buff[(buff_index * USB_MAX_PACKET_SIZE)], USB_MAX_PACKET_SIZE, USB_CLASS_PCDC);
						if(err != FSP_SUCCESS)
						{
							while(1);
						}
					}
				}
                break;
                case USB_STATUS_REQUEST : /* Receive Class Request */
                    R_USB_SetupGet(event_info, &setup);
                    if (USB_PCDC_SET_LINE_CODING == (setup.request_type & USB_BREQUEST))
                    {
                        R_USB_PeriControlDataGet(event_info, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH);
                    }
                    else if (USB_PCDC_GET_LINE_CODING == (setup.request_type & USB_BREQUEST))
                    {
                        R_USB_PeriControlDataSet(event_info, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH);
                    }
                    else if (USB_PCDC_SET_CONTROL_LINE_STATE == (event_info->setup.request_type & USB_BREQUEST))
                    {
                        fsp_err_t err = R_USB_PeriControlDataGet(event_info, (uint8_t *) &g_control_line_state, sizeof(g_control_line_state));
                        if (FSP_SUCCESS == err)
                        {
                            g_control_line_state.bdtr = (unsigned char)((event_info->setup.request_value >> 0) & 0x01);
                            g_control_line_state.brts = (unsigned char)((event_info->setup.request_value >> 1) & 0x01);
                        }
                    }
                    else
                    {
                        /* none */
                    }
                break;
                case USB_STATUS_REQUEST_COMPLETE :
                    __NOP();
                break;
                case USB_STATUS_SUSPEND :
                case USB_STATUS_DETACH :
                case USB_STATUS_DEFAULT :
                    __NOP();
                break;
                default :
                    __NOP();
                break;
            }

}

void read_cam()
{
//	xSemaphoreTake(&g_new_mutex0, ( TickType_t ) 10);
//copy the src image to a buffer
	memcpy(cam_out_rgb_888_buff, bsp_camera_out_buffer888,  BSP_CAM_WIDTH  * BSP_CAM_HEIGHT * 3);
//Release critical section for other threads to use
//	xSemaphoreGive(&g_new_mutex0);
}
