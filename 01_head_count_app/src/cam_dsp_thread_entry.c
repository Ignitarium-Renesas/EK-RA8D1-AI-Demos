#include "cam_dsp_thread.h"
/* Camera Viewer entry function */
/* pvParameters contains TaskHandle_t */
void cam_dsp_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
    while (1)
    {
        vTaskDelay (1);
    }
}
