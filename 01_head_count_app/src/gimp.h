/*
 * gimp.h
 *
 *  Created on: 19 Jun 2023
 *      Author: b3800117
 */

#ifndef GRAPHICS_GIMP_H_
#define GRAPHICS_GIMP_H_


typedef unsigned int  guint;
typedef unsigned char guint8;
typedef          char gchar;

typedef struct st_gimp_image {
  guint      width;
  guint      height;
  guint      bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
//   gchar     *comment; // include when images include comments
  guint8     pixel_data[64 * 64 * 2 + 1];
//  guint8     pixel_data[128 * 403 * 2 + 1];
//  guint8     pixel_data[480 * 854 * 2 + 1];
} st_gimp_image_t;

typedef struct st_gimp_weather_image {
  guint      width;
  guint      height;
  guint      bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  guint8     pixel_data[182 * 200 * 4  + 1];
} st_gimp_weather_image_t;

typedef struct st_gimp_mab_image {
  guint      width;
  guint      height;
  guint      bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  guint8     pixel_data[192 * 192 * 2  + 1];
} st_gimp_mab_image_t;

typedef struct st_gimp_led_image {
  guint      width;
  guint      height;
  guint      bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  guint8     pixel_data[332 * 240 * 2  + 1];
} st_gimp_led_image_t;

typedef struct st_gimp_ai_face_num_image {
  guint      width;
  guint      height;
  guint      bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  guint8     pixel_data[14 * 9 * 2  + 1];
} st_gimp_ai_face_num_image_t;

typedef struct st_gimp_font_full_image {
  guint      width;
  guint      height;
  guint      bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  guint8     pixel_data[28 * 691 * 2 + 1];
} st_gimp_font_full_image_t;

typedef struct st_gimp_bg_font_18_image {
  guint      width;
  guint      height;
  guint      bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  guint8     pixel_data[22 * 19 * 2 + 1];
} st_gimp_bg_font_18_image_t;

typedef enum image_data
{
    GI_SPLASH_SCREEN   = 0,              // 0x90040000
    GI_MAIN_SCREEN,                      // 0x90140000
    GI_MAIN_BLANK,                       // 0x90240000
    GI_MAIN_MENU,                        // 0x90340010
    GI_HELP_SCREEN,                      // 0x90440010
    GI_KIS_SCREEN,                       // 0x90540010
    GI_LED_GRAPHICS_SCREEN,              // 0x90640010
    GI_WEATHER_ICON_SCREEN,              // 0x90740010
    MODE_SLIDE_2,                        // 0x90840010
    MODE_SLIDE_3,                        // 0x90940010
    MODE_SLIDE_4,                        // 0x901040010
    MODE_SLIDE_5,                        // 0x90A40010
    MODE_SLIDE_6,                        // 0x90B40010
    MODE_SLIDE_7,                        // 0x90C40010
    MODE_SLIDE_8,                        // 0x90D40010
    MODE_SLIDE_9,                        // 0x90E40010
    MODE_SLIDE_10,                       // 0x90F40010
    MODE_SLIDE_11,                       // 0x91040010
    MODE_SLIDE_12,                       // 0x91140010
    MODE_SLIDE_13,                       // 0x91240010
    MODE_SLIDE_14,                       // 0x91340010
    MODE_SLIDE_15,                       // 0x91440010
    MODE_SLIDE_16,                       // 0x91540010
    MODE_SLIDE_17,                       // 0x91640010
    MODE_SLIDE_18,                       // 0x91740010
    MODE_SLIDE_19,                       // 0x91840010
    MODE_SLIDE_20,                       // 0x91940010
    MODE_SLIDE_21,                       // 0x91A40010
    MODE_SLIDE_22,                       // 0x91B40010
    MODE_SLIDE_23,                       // 0x91C40010
    MODE_SLIDE_24,                       // 0x91D40010
    MODE_SLIDE_25,                       // 0x91E40010
    GI_AI_OBJECT_DETECTION,               // 0x91F40010
    MODE_SLIDE_28,                       // 0x91E40010
    MODE_SLIDE_29,                       // 0x91F40010
    MODE_SLIDE_30,                       // 0x92040010
} st_image_data_t;

#endif /* GRAPHICS_GIMP_H_ */
