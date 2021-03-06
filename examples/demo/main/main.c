/* Simple firmware for a ESP32 displaying a static image on an EPaper Screen.
 *
 * Write an image into a header file using a 3...2...1...0 format per pixel,
 * for 4 bits color (16 colors - well, greys.) MSB first.  At 80 MHz, screen
 * clears execute in 1.075 seconds and images are drawn in 1.531 seconds.
 */

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

#include "epd_driver.h"
#ifdef CONFIG_EPD_DISPLAY_TYPE_ED060SC4
#include "firasans_12pt.h"
#else
#include "firasans.h"
#endif
#include "giraffe.h"
#include "img_board.h"

uint8_t *img_buf;

uint8_t *framebuffer;
uint8_t *original_image_ram;

void delay(uint32_t millis) { vTaskDelay(millis / portTICK_PERIOD_MS); }

uint32_t millis() { return esp_timer_get_time() / 1000; }

void loop() {

  printf("current temperature: %f\n", epd_ambient_temperature());
  delay(300);


  epd_poweron();
  volatile uint32_t t1 = millis();
  epd_clear();
  volatile uint32_t t2 = millis();
  printf("EPD clear took %dms.\n", t2 - t1);
  epd_poweroff();

  epd_draw_hline(20, 20, EPD_WIDTH - 40, 0x00, framebuffer);
  epd_draw_hline(20, EPD_HEIGHT - 20, EPD_WIDTH - 40, 0x00, framebuffer);
  epd_draw_vline(20, 20, EPD_HEIGHT - 40 + 1, 0x00, framebuffer);
  epd_draw_vline(EPD_WIDTH - 20, 20, EPD_HEIGHT - 40 + 1, 0x00, framebuffer);

  Rect_t area = {
      .x = 25,
      .y = 25,
      .width = giraffe_width,
      .height = giraffe_height,
  };
  epd_copy_to_framebuffer(area, (uint8_t *)giraffe_data, framebuffer);

#ifdef CONFIG_EPD_DISPLAY_TYPE_ED060SC4
  int cursor_x = 20 + giraffe_width + 20;
#else
  int cursor_x = 50 + giraffe_width + 20;
#endif
  int cursor_y = 100;
  write_string((GFXfont *)&FiraSans,
        "➸ 16 color grayscale\n"
        "➸ ~630ms for full frame draw 🚀\n"
        "➸ Use with 6\" or 9.7\" EPDs\n"
        "➸ High-quality font rendering ✎🙋",
  &cursor_x, &cursor_y, framebuffer);

  epd_poweron();
  t1 = millis();
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  t2 = millis();
  epd_poweroff();
  printf("EPD draw took %dms.\n", t2 - t1);

  delay(1000);
  cursor_x = 500;
#ifdef CONFIG_EPD_DISPLAY_TYPE_ED060SC4
  cursor_y = 450;
#else
  cursor_y = 600;
#endif
  char *string = "➠ With partial clear...";
  epd_poweron();
  writeln((GFXfont *)&FiraSans, string, &cursor_x, &cursor_y, NULL);
  epd_poweroff();
  delay(1000);

  Rect_t to_clear = {
      .x = 50 + giraffe_width + 20,
#ifdef CONFIG_EPD_DISPLAY_TYPE_ED060SC4
  .y = 300,
#else
  .y = 400,
#endif
      .width = EPD_WIDTH - 70 - 25 - giraffe_width,
      .height = 400,
  };
  epd_poweron();
  epd_clear_area(to_clear);
  epd_poweroff();

  cursor_x = 500;
  cursor_y = 390;
  string = "And partial update!";
  epd_poweron();
  writeln((GFXfont *)&FiraSans, string, &cursor_x, &cursor_y, NULL);

  Rect_t board_area = {
      .x = 50 + giraffe_width + 50,
#ifdef CONFIG_EPD_DISPLAY_TYPE_ED060SC4
  .y = 300,
#else
  .y = 400,
#endif
      .width = img_board_width,
      .height = img_board_height,
  };

  epd_draw_grayscale_image(board_area, (uint8_t*)img_board_data);
  epd_poweroff();

  delay(2000);
}

void epd_task() {
  epd_init();

  ESP_LOGW("main", "allocating...\n");

  framebuffer = (uint8_t *)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 2, MALLOC_CAP_SPIRAM);
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  while (1) {
    loop();
  };
}

void app_main() {
  ESP_LOGW("main", "Hello World!\n");

  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
  heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);

  xTaskCreatePinnedToCore(&epd_task, "epd task", 10000, NULL, 2, NULL, 1);
}
