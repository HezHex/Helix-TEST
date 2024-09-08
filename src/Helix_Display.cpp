#include <Arduino.h>
#include <ui_events.h>
#include <ui.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "Helix_Display.h"
#include "touch.h"
#include "Helix_Logger.h"
#include "Helix_Events.h"
// #include "Helix_Indicators.h"
#include "Helix_Memory.h"
#include "qrcode.h"

H_log LD("Display"); // Helix Logging

bool GUI_ON = false;             // Turn true when Display and Gui are completely initialized (used principally to start all loops in main.cpp)
bool LVGL_Log = true;            // Show gui log in serial and in log label in GUI
bool GFX_Initialized = false;    // check if gfx->begin was called to use with bsod or rsod
void my_log_cb(const char *buf); // Log callback function definition to enable lvgl logging

#pragma region Display Settings

#define TFT_Backlight_Channel 0         // Set Backlight channel for control of backlight
#define TFT_Backlight_Frequency 2000    // Set Backlight frequency
#define TFT_Backlight_Resolution_Bits 8 // Set backlight resolutions bits
#define TFT_BL 2                        // Set Backlight pin

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
    0 /* hsync_polarity */, 1 /* hsync_front_porch */, 1 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
    0 /* vsync_polarity */, 3 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 9000000 /* prefer_speed */);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480 /* width */, 272 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */);

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

/*******************************************************************************
 * Please config the touch panel in touch.h
 ******************************************************************************/

/* Change to your screen resolution */
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

#pragma endregion

#pragma region Display_Fade_Declarations

//void Display_Activity_CB(bool Forced);

void Display_Fade_CB(void *arg);
const esp_timer_create_args_t Display_Fade_Timer_args = { //   TIMER Screen fade out
    .callback = &Display_Fade_CB,                         //
    .name = "Display_Fade_Timer"};                        //
esp_timer_handle_t Display_Fade_Timer;                    //

void Display_Fade_BACKLIGHT_CB(void *arg);
const esp_timer_create_args_t Display_Fade_BACKLIGHT_args = { //   TIMER per backlight fade out
    .callback = &Display_Fade_BACKLIGHT_CB,                   //
    .name = "Display_Fade_BACKLIGHT_Timer"};                  //
esp_timer_handle_t Display_Fade_BACKLIGHT_Timer;              //

#pragma endregion

#pragma region Display flush and touch read

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
 

     Display_Activity_CB();
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

#pragma endregion

#pragma region Black and Red Screen of death
void Helix_Display_RSOD(String RSOD_TEXT)
{
  if (!GFX_Initialized)
  {
    gfx->begin(80000000);
    GFX_Initialized = true;
  }
  gfx->fillScreen(RED);
  int Disp_width = gfx->width();
  int Disp_height = gfx->height();
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  gfx->getTextBounds(RSOD_TEXT, 0, 0, &x1, &y1, &width, &height);
  gfx->setCursor((Disp_width - width) / 2, (Disp_height - height) / 2);
  gfx->print(RSOD_TEXT);
}

void Helix_Display_BSOD(String BSOD_TEXT)
{
  if (!GFX_Initialized)
  {
    gfx->begin(80000000);
    GFX_Initialized = true;
  }
  gfx->fillScreen(BLACK);
  int Disp_width = gfx->width();
  int Disp_height = gfx->height();
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  gfx->getTextBounds(BSOD_TEXT, 0, 0, &x1, &y1, &width, &height);
  gfx->setCursor((Disp_width - width) / 2, (Disp_height - height) / 2);
  gfx->print(BSOD_TEXT);
}

#pragma endregion

void Helix_Display_init()
{

#if LV_USE_LOG
  lv_log_register_print_cb(my_log_cb);
#endif

  LD.println("Initializing display", 1);
  LD.println("Setting display Communication speed", 1);
  if (!GFX_Initialized)
  {
    gfx->begin(80000000);
    GFX_Initialized = true;
  }

  Helix_Display_BSOD("System Starting");

#ifdef TFT_BL
  LD.println("Configuring backlight", 1);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  ledcSetup(TFT_Backlight_Channel, TFT_Backlight_Frequency, TFT_Backlight_Resolution_Bits);
  ledcAttachPin(TFT_BL, TFT_Backlight_Channel);
  ledcWrite(TFT_Backlight_Channel, Screen_OFF_Brightness); /* Screen brightness can be modified by adjusting this parameter. (0-255) */
#endif
  LD.println("Initializing GUI Library", 1);
  lv_init();
  delay(20);
  LD.println("Initializing Touch Screen Library", 1);
  LD.println("Ignore the following gpio pin errors...", 1);
  touch_init();

  LD.println("Adapt GUI to display size", 1);
  screenWidth = gfx->width();
  screenHeight = gfx->height();
#ifdef ESP32
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
  disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4);
#endif
  if (!disp_draw_buf)
  {
    LD.println("LVGL disp_draw_buf allocate failed!");
  }
  else
  {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 4);
    LD.println("LVGL display buffer initialized!", 1);
    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    LD.println("LVGL display driver initialized!", 1);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    LD.println("LVGL display driver registered!", 1);

    /* Initialize the (dummy) input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    LD.println("LVGL touch driver Initialized!", 1);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    LD.println("LVGL touch driver registered!", 1);
    delay(20);
    ui_init();
    LD.println("User Interface initialized!", 1);

    /* Initialize the display fade out/in on touch */
    esp_timer_create(&Display_Fade_Timer_args, &Display_Fade_Timer); // Creo il timer per il Fade out dello screen
    LD.println("D.F.S. timer allocated", 1);
    esp_timer_create(&Display_Fade_BACKLIGHT_args, &Display_Fade_BACKLIGHT_Timer); // Creo il timer per il Fade out del backlight
    LD.println("D.F.B. timer allocated", 1);
    Display_Activity_CB(true); // forzo il callback di risveglio display

    GUI_ON = true;         // Attivo i loop che agiscono sulla grafica su main.cpp
    
    LD.println("Setup done", 1);
  }
}

void my_log_cb(const char *buf) // Log callback Function to enable lvgl logging
{
  if (LVGL_Log)
  {
    LD.print("[LVGL]    ", 1);
    LD.print("LOG:", 1);
    LD.println(buf);
  }
}

#pragma region Display_Fade

int NOW_Display_bright;
void Display_Fade_BACKLIGHT_CB(void *arg)
{
  if (NOW_Display_bright >= Screen_OFF_Brightness)
  {
    NOW_Display_bright--;
    ledcWrite(TFT_Backlight_Channel, NOW_Display_bright);
  }
  else
  {
    esp_timer_stop(Display_Fade_BACKLIGHT_Timer);
     if (lv_scr_act() != ui_SCREENIdle)
    {
        if(FRESET_Mbox_EXISTS) { 
        lv_msgbox_close(FResetMbox);
        FRESET_Mbox_EXISTS = false;
         }
         
       lv_indev_wait_release(lv_indev_get_act());
       _ui_screen_change(&ui_SCREENIdle, LV_SCR_LOAD_ANIM_FADE_IN, (Screen_Fade_Out_Animation_Delay * 100), 0, &ui_SCREENIdle_screen_init);

     }
  }
}

void Display_Fade_in()
{
  if (esp_timer_is_active(Display_Fade_BACKLIGHT_Timer))
  {
    esp_timer_stop(Display_Fade_BACKLIGHT_Timer);
  }

  if (lv_scr_act() == ui_SCREENIdle)
  {
    lv_indev_wait_release(lv_indev_get_act());
    _ui_screen_change(&ui_SCREENmain, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_SCREENmain_screen_init);
  }

  int NOW_bright = ledcRead(0);
  if (NOW_bright != Screen_ON_Brightness)
  {
    while (NOW_bright <= Screen_ON_Brightness)
    {
      NOW_bright++;
      ledcWrite(TFT_Backlight_Channel, NOW_bright);
      delay(Screen_Fade_In_Animation_Delay);
    }
  }
}

void Display_Fade_out()
{
  NOW_Display_bright = ledcRead(0);
  esp_timer_start_periodic(Display_Fade_BACKLIGHT_Timer, (Screen_Fade_Out_Animation_Delay * 1000));
}

void Display_Fade_CB(void *arg)
{
  Display_Fade_out();
}

int deb_mill= 0;                                    //Display debounce system -- Variable
void Display_Activity_CB(bool Forced)
{




  int deb_diff = (millis() - deb_mill);             //Display debounce system -- i check how many milliseconds between every touch input

 // Serial.println(lv_refr_get_fps_avg());
if((35 >= deb_diff) && (deb_diff >= 25) || Forced == true) {          //Display debounce system -- if the difference are between 25 and 35 milliseconds probably is a real touch
  if(Display_CALLBACK_LOG) { 

    LD.print("Wake Up Display ");
    LD.print("Forced Mode:" );
    if(Forced) Serial.println("TRUE");
    Serial.println(Forced);
  }
  Display_Fade_in();

  if (esp_timer_is_active(Display_Fade_Timer))
  {
    esp_timer_stop(Display_Fade_Timer);
    if (lv_scr_act() == ui_SCREENConnections)
    {
      esp_timer_start_once(Display_Fade_Timer, (Screen_Connections_Inactivity_Delay * 1000));
    }
    else
    {
      esp_timer_start_once(Display_Fade_Timer, (Screen_Inactivity_Delay * 1000));
    }
  }
  else
  {
    if (lv_scr_act() == ui_SCREENConnections)
    {
      esp_timer_start_once(Display_Fade_Timer, (Screen_Connections_Inactivity_Delay * 1000));
    }
    else
    {
      esp_timer_start_once(Display_Fade_Timer, (Screen_Inactivity_Delay * 1000));
    }
  }
}
else {                                                          //Display debounce system -- here are detected the ghost touch input
    if(Display_CALLBACK_LOG) { 
    LD.print(String(millis() - deb_mill) + "---");
    LD.print(String(millis()) + "---");
    LD.print("Wake Up Display -- DEBOUNCED ---");
 LD.println("Forced Mode:" + String(Forced));
  }
}
  deb_mill = millis();                                          //Display debounce system -- i set the millisecond value to check the next touch input
}

#pragma endregion