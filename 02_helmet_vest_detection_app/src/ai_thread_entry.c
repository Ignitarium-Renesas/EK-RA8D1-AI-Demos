#include <stdarg.h>
#include "ai_thread.h"
#include "common_data.h"
#include "common_init.h"
#include "common_utils.h"
#include "menu_camview.h"
#include "timer.h"
#include "string.h"


#define MENU_EXIT_CRTL           (0x20)


static char_t s_print_buffer[BUFFER_LINE_LENGTH] = {};
extern int run_apps ();
extern bool g_capture_ready;

st_ai_detection_point_t g_ai_detection[20] = {};

void update_detection_result(signed short index, signed short x, signed short  y, signed short  w, signed short  h, signed short c)
{
    if(index < 20)
    {
        g_ai_detection[index].m_x = x;
        g_ai_detection[index].m_y = y;
        g_ai_detection[index].m_w = w;
        g_ai_detection[index].m_h = h;
        g_ai_detection[index].m_c = c;
    }

}

int e_printf(const char *format, ...)
{
#if 1
    va_list args;
    va_start(args, format);
    int result = vsprintf(s_print_buffer, format, args);
    va_end(args);
    sprintf(s_print_buffer, "%s\r\n", s_print_buffer);
    print_to_console((void*)s_print_buffer);
    return result;
#endif
    return 0;
}

/**********************************************************************************************************************
 * Function Name: main_display_menu
 * Description  : .
 * Return Value : The main menu controller.
 *********************************************************************************************************************/
/* AI Thread entry function */
/* pvParameters contains TaskHandle_t */
void ai_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    timer_init();
	vTaskDelay(5000);
    while (1)
    {
        static uint64_t t1 = 0;
        t1 = get_timestamp();
        SCB_CleanDCache();
        SCB_EnableDCache();
        run_apps();
        SCB_DisableDCache();
        SCB_CleanDCache();
        vTaskDelay(180);
    }
}
