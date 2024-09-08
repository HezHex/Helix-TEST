#ifndef _Helix_Memory_h_
#define _Helix_Memory_h_

#include <Arduino.h>

#define SettingFileALLOC_SIZE 1024*2

extern void Helix_Settings_Init();
extern void Helix_Factory_Reset();
extern void Helix_Update_Settings();
extern void LV_FS_Initialize();
extern bool Helix_Memory_State;
extern bool Helix_Memory_Writing;
extern String SW_Version;
extern String SettingsFile;
extern void sc_filelist();
extern void sc_mkdir(const char *path);
extern void sc_rmdir(const char * path);
extern void sc_rmfile(const char *path);
extern void sc_writefile(const char * path,const char * message);
extern void sc_Settings_List();
extern void sc_Settings_write(String key, String val);
extern void sc_RAW_Settings_write(String key, String val);
extern void EXT_Settings_write(String key, String val);
extern String Helix_Type_Reader(String Variable);
extern void Helix_Settings_Writer(String WKey, String WValue);

#pragma region SDVar Globalization
/*Blocco di Globalizzazione variabili*/ /*[VARIABILI SD]*/
extern String AP_Prefix_SSID_Name ;
extern String AP_Password ;
extern String WiFi_SSID ;
extern String WiFi_Password ;
extern int WiFi_Connections_Retry ;
extern float Room_Setpoint ;
extern float SETPOINT_INC_VALUE;
extern float TEMPERATURE_SENSOR_CALIBRATION_VALUE ;
extern int SENSOR_READINGS_DELAY ;
extern int Screen_ON_Brightness ;
extern int Screen_OFF_Brightness ;
extern int Screen_Inactivity_Delay ;
extern int Screen_Fade_Out_Animation_Delay ;
extern int Screen_Fade_In_Animation_Delay ;
extern int Helix_LOG_Priority;
extern String login_User;
extern String login_Password;
extern int AjaxEvents_DELAY;
extern String Room_Name;
extern int Starts_Counter;
extern String ntpServer;
extern long gmtOffset_sec;
extern int daylightOffset_sec;
extern bool Ap_Start;
extern String MQTT_Server;
extern String MQTT_User;
extern String MQTT_Password;
extern String MQTT_Topic;
extern int MQTT_Delay;
extern int Screen_Connections_Inactivity_Delay;
extern bool Sensors_LOG;
extern bool Display_CALLBACK_LOG;
extern bool WebTerminal_Enabled;
extern bool Fileman_Enabled;
extern bool ADV_WebSettings_Enabled;
//@@@@END_BGV
#pragma endregion

#endif