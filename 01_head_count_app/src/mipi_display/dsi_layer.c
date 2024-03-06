#include "r_ioport.h"
#include "r_mipi_dsi_api.h"

#include "hal_data.h"
#include "dsi_layer.h"

#define PIN_DISPLAY_RESET       BSP_IO_PORT_10_PIN_01
#define PIN_DISPLAY_BACKLIGHT   BSP_IO_PORT_04_PIN_04

void dsi_layer_hw_reset()
{
    /* Reset Display - active low
     * NOTE: Sleep out may not be issued for 120 ms after HW reset */
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_RESET, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(7, BSP_DELAY_UNITS_MICROSECONDS); // Set active for 5-9 us
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_RESET, BSP_IO_LEVEL_HIGH);
    return;
}

void dsi_layer_enable_backlight()
{
  R_BSP_PinAccessEnable();
  R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_BACKLIGHT, BSP_IO_LEVEL_HIGH);
  R_BSP_PinAccessDisable();
  return;
}

void dsi_layer_disable_backlight(void)
{
  R_BSP_PinAccessEnable();
  R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_BACKLIGHT, BSP_IO_LEVEL_LOW);
  R_BSP_PinAccessDisable();
  return;
}

