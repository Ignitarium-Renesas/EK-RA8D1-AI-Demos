#include "new_thread0.h"
#include <stdbool.h>


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


#define GREYSCALE_SIZE	(192 * 192)

uint32_t uart_head_count = 30;
uint32_t uart_inference_time = 20;
uint8_t *img_buf;

static bool  b_usb_attach = false;
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


void recieve_send_USB();
static fsp_err_t print_to_console(char *p_data);
static fsp_err_t check_for_write_complete(void);
void myUsbCallback(usb_event_info_t *event, usb_hdl_t task_handle, usb_onoff_t status);


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
//		recieve_send_USB();
//    	print_to_console("\r\nhello...");

//    	R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, USB_CLASS_PCDC);

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


    usb_setup_t             setup;

    switch (event_info->event)
            {
                case USB_STATUS_CONFIGURED :
                	R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, USB_CLASS_PCDC);

                break;
                case USB_STATUS_WRITE_COMPLETE :
                  R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, USB_CLASS_PCDC);
//
                break;
                case USB_STATUS_READ_COMPLETE :
                    if(g_buf[0] == 'c')
                    {
                    	R_USB_Write (&g_basic0_ctrl, &uart_head_count, sizeof(uart_head_count), USB_CLASS_PCDC);
                    }
                    else if(g_buf[0] == 't')
                    {
//                    	R_USB_Write (&g_basic0_ctrl, uart_inference_time, sizeof(uart_inference_time), USB_CLASS_PCDC);
                    }
                    else if(g_buf[0] == 'i')
                    {
//                    	R_USB_Write (&g_basic0_ctrl, img_buf, GREYSCALE_SIZE, USB_CLASS_PCDC);
                    }
                    else
                    {
//                    	R_USB_Write (&g_basic0_ctrl, "Not_C_or_T", sizeof("Not_C_or_T"), USB_CLASS_PCDC);
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
