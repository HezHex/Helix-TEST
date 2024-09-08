#include <Arduino.h>
#include "ui.h"
#include "Helix_Display.h"
#include "Helix_Memory.h"
#include "Helix_Logger.h"
#include "Helix_Sensors.h"
#include "Helix_Events.h"
#include "Helix_Indicators.h"
#include "Helix_serialComm.h"
#include "Helix_Wifi.h"
#include "Version.h"

#define Helix_VERSION "2" // Main Version, builds are updated automatically

#define DEV_MODE 1 // force the activation of features for development NB DISABLE ON DEFINITIVE RELASE!!!!!

#define Forced_Log_Priority 0 // log priority in dev mode 0 - 4

#pragma region DEV_MODE_LOGGING_DEFINE
#if DEV_MODE == 1
#define ALL_LOG 1 // force the activation of ALL LOG NB DISABLE ON DEFINITIVE RELASE!!!!!
#else
#define ALL_LOG 0 // force the activation of ALL LOG NB DISABLE ON DEFINITIVE RELASE!!!!!
#endif
#pragma endregion

H_log LM("Main Module");

bool System_ready = false; // set true when setup end to start all loops
String partial_SW_Version;

/// @brief Spacer_utility for Helo message
/// @param Base_Message standard string ex: "|-----------------------------------------|"
/// @param message variable message ex: version
/// @param offset integer number of characters to add ex: |software version
/// @return
String spacer(String Base_Message, String message, int offset)
{
  String spaces;
  int message_LENGTH = message.length();
  int Base_Message_LENGTH = Base_Message.length();
  message_LENGTH = Base_Message_LENGTH - (message_LENGTH + offset);
  for (int i = 0; i <= message_LENGTH; i++)
  {
    spaces += ' ';
  }
  return spaces;
}

void setup()
{

#pragma region VERSIONING

  SW_Version = Helix_VERSION;
  SW_Version += ".";
  SW_Version += String(VERSION);
  SW_Version += "-";
  SW_Version += String(BUILD_TIMESTAMP);

  partial_SW_Version = Helix_VERSION;
  partial_SW_Version += ".";
  partial_SW_Version += String(VERSION);

#pragma endregion

#pragma region DEV_MODE_MARKING_AND_LOGGING_SET

#if DEV_MODE
  SW_Version += " [DEVMODE ENABLED-NOT OFFICIAL RELASE]";
  esp_log_level_set("*", ESP_LOG_VERBOSE);
#else
  esp_log_level_set("*", ESP_LOG_ERROR);
#endif

#pragma endregion

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("");
  Serial.println("");
  Serial.println("");
  LM.println("System Starting...", 1);
  LM.println("Serial: Initialized", 1);

#pragma region Serial_HELO_Message
  Serial.println("");
  Serial.println(" .----------------.  .----------------.  .----------------.  .----------------.  .----------------. ");
  Serial.println("| .--------------. || .--------------. || .--------------. || .--------------. || .--------------. |");
  Serial.println("| |  ____  ____  | || |  _________   | || |   _____      | || |     _____    | || |  ____  ____  | |");
  Serial.println("| | |_   ||   _| | || | |_   ___  |  | || |  |_   _|     | || |    |_   _|   | || | |_  _||_  _| | |");
  Serial.println("| |   | |__| |   | || |   | |_  \\_|  | || |    | |       | || |      | |     | || |   \\ \\  / /   | |");
  Serial.println("| |   |  __  |   | || |   |  _|  _   | || |    | |   _   | || |      | |     | || |    > `' <    | |");
  Serial.println("| |  _| |  | |_  | || |  _| |___/ |  | || |   _| |__/ |  | || |     _| |_    | || |  _/ /'`\\ \\_  | |");
  Serial.println("| | |____||____| | || | |_________|  | || |  |________|  | || |    |_____|   | || | |____||____| | |");
  Serial.println("| |              | || |              | || |              | || |              | || |              | |");
  Serial.println("| '--------------' || '--------------' || '--------------' || '--------------' || '--------------' |");
  Serial.println("|----------------'  '----------------'  '----------------'  '----------------'  '----------------' ");
  Serial.println("");
  Serial.println("|--------------------------------------------------------------|");
  Serial.println("|                                                              |");
  Serial.println("|                          Control Room                        |");
  Serial.println("|                                                              |");
  Serial.print("|Software Version: "); // 18 char
  Serial.print(SW_Version);
  Serial.print(spacer("|--------------------------------------------------------------|", SW_Version, 21));
  Serial.println("|");
  Serial.println("|                                                              |");
  Serial.println("|--------------------------------------------------------------|");
  Serial.println("|                                                              |");
  Serial.println("|type: help into serial monitor to see avaible serial commands.|");
  Serial.println("|                                                              |");
  Serial.println("|--------------------------------------------------------------|");
  Serial.println("");
#pragma endregion

  Helix_Settings_Init(); // SETTINGS Initialization

#if ALL_LOG
  Helix_LOG_Priority = Forced_Log_Priority; // force log level overriding config.cfg in SD card
 // Fileman_Enabled = true;                   // enable Web Filemanager
#endif

  Helix_Display_init(); // DISPLAY Initialization

  Helix_Events_Init(); // UI Events Initialization

  Helix_Display_BSOD("System Starting \n\n\n\n\n\n\n\n\n\n    SW: " +  partial_SW_Version);

  Helix_Sensors_init(); // SENSORS Initialization

  Helix_Indicators_Init(); // INDICATORS Initialization

  delay(100);

  Display_Activity_CB(true); // Wake Up Display

  System_ready = true; // TURN ON LOOPS

  Helix_Wifi_init(); // WI-FI Initialization
}

#pragma region LoopsDefines

// millis() va a zero dopo 49 giorni e 17 ore
int Gui_time = 0;
#define GUI_delay 5

int PrintTime_time = 0;
#define PrintTime_delay 1000

int Indicators_time = 0;
#define Indicators_delay 1000

int Sensors_time = 0;
// #define Sensors_delay  //ALREADY DEFINED IN SETTINGS.cfg on SD CARD

int Sercomm_time = 0;
#define Sercomm_delay 1000

#define LOOPS_RESET_DELAY 4294967295 // dopo 49 giorni e 17 ore resetto i loop timer (sperando che non sia crashato prima)

#pragma endregion

void loop()
{
  if (System_ready) // start the loops only if the setup was complete to avoid lvgl errors or calls to uninitialized components
  {

#pragma region Gui_Loop
    if (millis() >= Gui_time + GUI_delay)
    {
      Gui_time += GUI_delay;
      if (GUI_ON)
      {
        lv_timer_handler(); /* let the GUI do its work */
      }
    }
#pragma endregion

#pragma region PrintTime_Loop
    if (millis() >= PrintTime_time + PrintTime_delay)
    {
      PrintTime_time += PrintTime_delay;
      if (GUI_ON)
      {
        printLocalTime();
      }
    }
#pragma endregion

#pragma region Indicators_Loop
    if (millis() >= Indicators_time + Indicators_delay)
    {
      Indicators_time += Indicators_delay;
      GUI_IndicatorCB();
    }
#pragma endregion

#pragma region Sensors_Loop
    if (millis() >= Sensors_time + SENSOR_READINGS_DELAY)
    {
      Sensors_time += SENSOR_READINGS_DELAY;
      Helix_Sensors_loop();
    }
#pragma endregion

#pragma region Serial_Communication_Loop
    if (millis() >= Sercomm_time + Sercomm_delay)
    {
      Sercomm_time += Sercomm_delay;
      Helix_SerialComm_loop();
    }
#pragma endregion

#pragma region Millis_49Days_Reset
    if (millis() >= LOOPS_RESET_DELAY) // dopo 49 giorni e 17 ore resetto i loop timer (sperando che non sia crashato prima)
    {
      Indicators_time = 0;
      Sensors_time = 0;
      Gui_time = 0;
      Sercomm_time = 0;
      PrintTime_time = 0;
    }
#pragma endregion
  
  }
}
