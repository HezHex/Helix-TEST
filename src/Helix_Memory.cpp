#include <Arduino.h>
#include "Helix_Logger.h"
#include "Helix_Indicators.h"
#include "Helix_serialComm.h"
#include <WiFi.h> // Necessario per settare il mac address come Room name in caso di mancanza settaggio o factory reset
#include "SPI.h"
#include "SD.h"
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include "esp_timer.h"

H_log LMS("Settings Module");

#pragma region SD_CARD_DEFINES //Sd Card pins and class

#define SD_CARD_CS 10
#define SPI_CARD_MOSI 11
#define SPI_CARD_MISO 13
#define SPI_CARD_SCK 12
static SPIClass sdCardSpi;

#pragma endregion






#pragma region Settings_Errors_and_Retry_Timer

static void SD_Write_Retry_timer_callback(void *arg);       //
const esp_timer_create_args_t SD_Write_Retry_timer_args = { //   TIMER per tentativi di scrittura su sd in caso di errore
    .callback = &SD_Write_Retry_timer_callback,             //   si attiva una volta ogni 5 secodni
    .name = "SD_Write_Retry"};                              //
esp_timer_handle_t SD_Write_Retry_timer;                    //
// void SD_Settings_Write_Error();
void SD_Settings_Write_Error() // Funzione Errore di scrittura su SD
{
  Helix_Memory_State = false;                          // memoria in errore accendo il flag per indicators.cpp
  Helix_Memory_Writing = false;                        // spengo il flag di scrittura su memoria per indicators.cpp
  GUI_IndicatorCB();                                   // aggiorno forzatamente la gui di indicators.cpp altrimenti si aggiornerebbe da sola in base al ciclo task di indicators.cpp (ma potrebbe avvenire in ritardo)
  esp_timer_start_once(SD_Write_Retry_timer, 5000000); // Avvio il Timer per Ritentare la scrittura su SD dopo 5 secondi
}
static void SD_Write_Retry_timer_callback(void *arg) // Callback Tentativo di scrittura
{
  Helix_Update_Settings();
}

#pragma endregion







#pragma region Settings_File_Definitions_And_Json

String SettingsFile = "/config.cfg";              // HELIX Settings        FILE name  (path + file)
String SettingsTypes = "/config.types";           // HELIX Settings Types  FILE name  (path + file)
File HFS_File;                                    // HELIX Settings        FILE       (oggetto del file system)
File HFTS_File;                                   // HELIX Settings Types  FILE       (oggetto del file system)
DynamicJsonDocument jset(SettingFileALLOC_SIZE);  // HELIX Settings        FILE json  (oggetto Json Document)
DynamicJsonDocument jtset(SettingFileALLOC_SIZE); // HELIX Type Settings   FILE json  (oggetto Json Document)
#define ARDUINOJSON_ENABLE_COMMENTS 1

#pragma endregion






#pragma region Other_VARS
bool Helix_Memory_State = false;   //   VARIABILI per indicators.cpp
bool Helix_Memory_Writing = false; //
String SW_Version;                 // Software version variable needed for visualization on webpage and ui.
#pragma endregion





#pragma region BCV //Var definition and default value
/*Blocco di Creazione variabili*/
String AP_Prefix_SSID_Name = "Helix_AP_";          // used
String AP_Password = "1234567890";                 // used
String WiFi_SSID = "HEZHEX";                       // used
String WiFi_Password = "wonderfulanny854";         // used
int WiFi_Connections_Retry = 20;                   // used
float Room_Setpoint = 20.5;                        // used
float SETPOINT_INC_VALUE = 0.5;                    // used
float TEMPERATURE_SENSOR_CALIBRATION_VALUE = -1.5; // used
int SENSOR_READINGS_DELAY = 5000;                  // used
int Screen_ON_Brightness = 255;                    // used
int Screen_OFF_Brightness = 20;                    // used
int Screen_Inactivity_Delay = 5000;                // used
int Screen_Connections_Inactivity_Delay = 60000;
int Screen_Fade_Out_Animation_Delay = 10; // used
int Screen_Fade_In_Animation_Delay = 3;   // used
int Helix_LOG_Priority = 1;               // used
String login_User = "admin";              // used
String login_Password = "helix";          // used
int AjaxEvents_DELAY = 5000;              // used
String Room_Name = WiFi.macAddress();     // used
int Starts_Counter = 0;                   // used
String ntpServer = "pool.ntp.org";
long gmtOffset_sec = 3600;
int daylightOffset_sec = 3600;
bool Ap_Start = false;
String MQTT_Server = "192.168.1.4";
String MQTT_User = "HEZ";
String MQTT_Password = "testpassword";
String MQTT_Topic = "HELIX";
int MQTT_Delay = 120;
bool Sensors_LOG = false;
bool Display_CALLBACK_LOG = false;
 bool WebTerminal_Enabled = false;
 bool Fileman_Enabled = false;
 bool ADV_WebSettings_Enabled = false;
//@@@@END_BCV
#pragma endregion

void Setting_File_Init()
{

  if (SD.exists(SettingsFile))
  {
    LMS.println("Settings Found", 1);

#pragma region BLV  // Config.cfg reader and variable population
    HFS_File = SD.open(SettingsFile, FILE_READ);

    //  Deserialize the JSON document
    DeserializationError error = deserializeJson(jset, HFS_File);

    if (error)
    {
      LMS.println("Failed to read file, using default configuration", 2);
      LMS.println(error.c_str(), 2);
    }

    /*Blocco di Lettura variabili*/
    AP_Prefix_SSID_Name = jset["AP_Prefix_SSID_Name"].as<String>();
    AP_Password = jset["AP_Password"].as<String>();
    WiFi_SSID = jset["WiFi_SSID"].as<String>();
    WiFi_Password = jset["WiFi_Password"].as<String>();
    WiFi_Connections_Retry = jset["WiFi_Connections_Retry"].as<int>();
    Room_Setpoint = jset["Room_Setpoint"].as<float>();
    SETPOINT_INC_VALUE = jset["SETPOINT_INC_VALUE"].as<float>();
    TEMPERATURE_SENSOR_CALIBRATION_VALUE = jset["TEMPERATURE_SENSOR_CALIBRATION_VALUE"].as<float>();
    SENSOR_READINGS_DELAY = jset["SENSOR_READINGS_DELAY"].as<int>();
    Screen_ON_Brightness = jset["Screen_ON_Brightness"].as<int>();
    Screen_OFF_Brightness = jset["Screen_OFF_Brightness"].as<int>();
    Screen_Inactivity_Delay = jset["Screen_Inactivity_Delay"].as<int>();
    Screen_Connections_Inactivity_Delay = jset["Screen_Connections_Inactivity_Delay"].as<int>();
    Screen_Fade_Out_Animation_Delay = jset["Screen_Fade_Out_Animation_Delay"].as<int>();
    Screen_Fade_In_Animation_Delay = jset["Screen_Fade_In_Animation_Delay"].as<int>();
    Helix_LOG_Priority = jset["Helix_LOG_Priority"].as<int>();
    login_User = jset["login_User"].as<String>();
    login_Password = jset["login_Password"].as<String>();
    AjaxEvents_DELAY = jset["AjaxEvents_DELAY"].as<int>();
    Room_Name = jset["Room_Name"].as<String>();
    Starts_Counter = jset["Starts_Counter"].as<int>();
    ntpServer = jset["ntpServer"].as<String>();
    gmtOffset_sec = jset["gmtOffset_sec"].as<long>();
    daylightOffset_sec = jset["daylightOffset_sec"].as<int>();
    Ap_Start = jset["Ap_Start"].as<bool>();
    MQTT_Server = jset["MQTT_Server"].as<String>();
    MQTT_User = jset["MQTT_User"].as<String>();
    MQTT_Password = jset["MQTT_Password"].as<String>();
    MQTT_Topic = jset["MQTT_Topic"].as<String>();
    MQTT_Delay = jset["MQTT_Delay"].as<int>();
    Sensors_LOG = jset["Sensors_LOG"].as<bool>();
    Display_CALLBACK_LOG = jset["Display_CALLBACK_LOG"].as<bool>();
    WebTerminal_Enabled = jset["WebTerminal_Enabled"].as<bool>();
Fileman_Enabled = jset["Fileman_Enabled"].as<bool>();
ADV_WebSettings_Enabled = jset["ADV_WebSettings_Enabled"].as<bool>();
//@@@@END_BLV

    HFS_File.close();

#pragma endregion
 
  }
  else
  {
    LMS.println("Settings Not Found", 2);

#pragma region BSV  // Config.cfg builder
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    HFS_File = SD.open(SettingsFile, FILE_WRITE);

    // if the file opened okay, write to it:
    if (HFS_File)
    {
      LMS.println("Writing default settings", 1);
      /*Blocco di Scrittura variabili*/
      jset["AP_Prefix_SSID_Name"] = AP_Prefix_SSID_Name;
      jset["AP_Password"] = AP_Password;
      jset["WiFi_SSID"] = WiFi_SSID;
      jset["WiFi_Password"] = WiFi_Password;
      jset["WiFi_Connections_Retry"] = WiFi_Connections_Retry;
      jset["Room_Setpoint"] = Room_Setpoint;
      jset["SETPOINT_INC_VALUE"] = SETPOINT_INC_VALUE;
      jset["TEMPERATURE_SENSOR_CALIBRATION_VALUE"] = TEMPERATURE_SENSOR_CALIBRATION_VALUE;
      jset["SENSOR_READINGS_DELAY"] = SENSOR_READINGS_DELAY;
      jset["Screen_ON_Brightness"] = Screen_ON_Brightness;
      jset["Screen_OFF_Brightness"] = Screen_OFF_Brightness;
      jset["Screen_Inactivity_Delay"] = Screen_Inactivity_Delay;
      jset["Screen_Connections_Inactivity_Delay"] = Screen_Connections_Inactivity_Delay;
      jset["Screen_Fade_Out_Animation_Delay"] = Screen_Fade_Out_Animation_Delay;
      jset["Screen_Fade_In_Animation_Delay"] = Screen_Fade_In_Animation_Delay;
      jset["Helix_LOG_Priority"] = Helix_LOG_Priority;
      jset["login_User"] = login_User;
      jset["login_Password"] = login_Password;
      jset["AjaxEvents_DELAY"] = AjaxEvents_DELAY;
      jset["Room_Name"] = Room_Name;
      jset["Starts_Counter"] = Starts_Counter;
      jset["ntpServer"] = ntpServer;
      jset["gmtOffset_sec"] = gmtOffset_sec;
      jset["daylightOffset_sec"] = daylightOffset_sec;
      jset["Ap_Start"] = Ap_Start;
      jset["MQTT_Server"] = MQTT_Server;
      jset["MQTT_User"] = MQTT_User;
      jset["MQTT_Password"] = MQTT_Password;
      jset["MQTT_Topic"] = MQTT_Topic;
      jset["MQTT_Delay"] = MQTT_Delay;
      jset["Sensors_LOG"] = Sensors_LOG;
      jset["Display_CALLBACK_LOG"] = Display_CALLBACK_LOG;
      jset["WebTerminal_Enabled"] = WebTerminal_Enabled;
			jset["Fileman_Enabled"] = Fileman_Enabled;
			jset["ADV_WebSettings_Enabled"] = ADV_WebSettings_Enabled;
			//@@@@END_BSV

      if (serializeJsonPretty(jset, HFS_File) == 0)
      {
        LMS.println("Failed to write Settings", 2);
      }
      HFS_File.close();
#pragma endregion





#pragma region BSVT // Config.type builder
      HFTS_File = SD.open(SettingsTypes, FILE_WRITE);

      jtset["AP_Prefix_SSID_Name"] = "String";
      jtset["AP_Password"] = "String";
      jtset["WiFi_SSID"] = "String";
      jtset["WiFi_Password"] = "String";
      jtset["WiFi_Connections_Retry"] = "int";
      jtset["Room_Setpoint"] = "float";
      jtset["SETPOINT_INC_VALUE"] = "float";
      jtset["TEMPERATURE_SENSOR_CALIBRATION_VALUE"] = "float";
      jtset["SENSOR_READINGS_DELAY"] = "int";
      jtset["Screen_ON_Brightness"] = "int";
      jtset["Screen_OFF_Brightness"] = "int";
      jtset["Screen_Inactivity_Delay"] = "int";
      jtset["Screen_Connections_Inactivity_Delay"] = "int";
      jtset["Screen_Fade_Out_Animation_Delay"] = "int";
      jtset["Screen_Fade_In_Animation_Delay"] = "int";
      jtset["Helix_LOG_Priority"] = "int";
      jtset["login_User"] = "String";
      jtset["login_Password"] = "String";
      jtset["AjaxEvents_DELAY"] = "int";
      jtset["Room_Name"] = "String";
      jtset["Starts_Counter"] = "int";
      jtset["ntpServer"] = "String";
      jtset["gmtOffset_sec"] = "long";
      jtset["daylightOffset_sec"] = "int";
      jtset["Ap_Start"] = "bool";
      jtset["MQTT_Server"] = "String";
      jtset["MQTT_User"] = "String";
      jtset["MQTT_Password"] = "String";
      jtset["MQTT_Topic"] = "String";
      jtset["MQTT_Delay"] = "int";
      jtset["Sensors_LOG"] = "bool";
      jtset["Display_CALLBACK_LOG"] = "bool";
			jtset["WebTerminal_Enabled"] = "bool";
			jtset["Fileman_Enabled"] = "bool";
			jtset["ADV_WebSettings_Enabled"] = "bool";
			//@@@@END_BSVT

      if (serializeJsonPretty(jtset, HFTS_File) == 0)
      {
        LMS.println("Failed to write Settings", 2);
      }
      HFTS_File.close();
#pragma endregion

      LMS.println("Settings Writed", 1);
    }
    else
    {
      LMS.println("error opening Configuration file", 2);
      Helix_Memory_State = false;
    }
  }

  esp_timer_create(&SD_Write_Retry_timer_args, &SD_Write_Retry_timer); // Creo il timer per i tentativi di scrittura in caso di errore SD
}

void Helix_Settings_Init()
{
#pragma region SDCard_INIT
  sdCardSpi.begin(SPI_CARD_SCK, SPI_CARD_MISO, SPI_CARD_MOSI);
  if (!SD.begin(SD_CARD_CS))
  {
    LMS.println("Card Mount Failed", 1);
    return;
  }
  else
  {
    LMS.println("Card Mount Succeed", 1);
    Helix_Memory_State = true;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    LMS.println("No SD card attached", 2);
    return;
  }

  LMS.print("SD Card Type: ");

  if (cardType == CARD_MMC)
  {
    LMS.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    LMS.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    LMS.println("SDHC");
  }
  else
  {
    LMS.println("UNKNOWN");
  }
#pragma endregion

  Setting_File_Init();
}

void Helix_Update_Settings()
{
  Helix_Memory_Writing = true; // turn on memory writing indicator
  GUI_IndicatorCB();           // force gui indicator callback

  if (!SD.begin(SD_CARD_CS, sdCardSpi))
  {
    LMS.println("Card Not present", 2);
    SD_Settings_Write_Error(); // errore scrittura su sd
    return;
  }
  else
  {
    Helix_Memory_State = true;
    #pragma region BSV //variable updater SD Side
    HFS_File = SD.open(SettingsFile, FILE_WRITE);
    // if the file opened okay, write to it:
    if (HFS_File)
    {
      LMS.println("Updating settings");
      /*Blocco di Scrittura variabili*/
      jset["AP_Prefix_SSID_Name"] = AP_Prefix_SSID_Name;
      jset["AP_Password"] = AP_Password;
      jset["WiFi_SSID"] = WiFi_SSID;
      jset["WiFi_Password"] = WiFi_Password;
      jset["WiFi_Connections_Retry"] = WiFi_Connections_Retry;
      jset["Room_Setpoint"] = Room_Setpoint;
      jset["SETPOINT_INC_VALUE"] = SETPOINT_INC_VALUE;
      jset["TEMPERATURE_SENSOR_CALIBRATION_VALUE"] = TEMPERATURE_SENSOR_CALIBRATION_VALUE;
      jset["SENSOR_READINGS_DELAY"] = SENSOR_READINGS_DELAY;
      jset["Screen_ON_Brightness"] = Screen_ON_Brightness;
      jset["Screen_OFF_Brightness"] = Screen_OFF_Brightness;
      jset["Screen_Inactivity_Delay"] = Screen_Inactivity_Delay;
      jset["Screen_Connections_Inactivity_Delay"] = Screen_Connections_Inactivity_Delay;
      jset["Screen_Fade_Out_Animation_Delay"] = Screen_Fade_Out_Animation_Delay;
      jset["Screen_Fade_In_Animation_Delay"] = Screen_Fade_In_Animation_Delay;
      jset["Helix_LOG_Priority"] = Helix_LOG_Priority;
      jset["login_User"] = login_User;
      jset["login_Password"] = login_Password;
      jset["AjaxEvents_DELAY"] = AjaxEvents_DELAY;
      jset["Room_Name"] = Room_Name;
      jset["Starts_Counter"] = Starts_Counter;
      jset["ntpServer"] = ntpServer;
      jset["gmtOffset_sec"] = gmtOffset_sec;
      jset["daylightOffset_sec"] = daylightOffset_sec;
      jset["Ap_Start"] = Ap_Start;
      jset["MQTT_Server"] = MQTT_Server;
      jset["MQTT_User"] = MQTT_User;
      jset["MQTT_Password"] = MQTT_Password;
      jset["MQTT_Topic"] = MQTT_Topic;
      jset["MQTT_Delay"] = MQTT_Delay;
      jset["Sensors_LOG"] = Sensors_LOG;
      jset["Display_CALLBACK_LOG"] = Display_CALLBACK_LOG;
      jset["WebTerminal_Enabled"] = WebTerminal_Enabled;
			jset["Fileman_Enabled"] = Fileman_Enabled;
			jset["ADV_WebSettings_Enabled"] = ADV_WebSettings_Enabled;
			//@@@@END_BSV

      if (serializeJsonPretty(jset, HFS_File) == 0) // rendo il file json formattato per leggerlo agevolmente e lo scrivo su sd
      {
        LMS.println("Failed to write Settings", 2);
        SD_Settings_Write_Error();
      }
      HFS_File.close(); // chiudo il file
      #pragma endregion
   
      LMS.println("Settings Writed");
    }
    else
    {
      LMS.println("Error opening Configuration file", 2);
      SD_Settings_Write_Error();
    }
  }
  Helix_Memory_Writing = false;
}

void Helix_Factory_Reset()
{
  SD.remove(SettingsFile);
  LMS.println("Settings deleted", 2);
  LMS.println("Restarting Helix Control Panel", 2);
  delay(3000);
  ESP.restart();
}

#pragma region Settings_Writer //used by webpage and serialcomm

void Helix_Settings_Writer(String WKey, String WValue)
{

  HFS_File = SD.open(SettingsFile, FILE_WRITE);

  if (HFS_File)
  {

    if (Helix_Type_Reader(WKey).indexOf("string") >= 0)
    {

      jset[WKey] = WValue;
    }
    else if (Helix_Type_Reader(WKey).indexOf("int") >= 0)
    {

      jset[WKey] = WValue.toInt();
    }
    else if (Helix_Type_Reader(WKey).indexOf("float") >= 0)
    {

      jset[WKey] = WValue.toFloat();
    }
    else if (Helix_Type_Reader(WKey).indexOf("long") >= 0)
    {

      jset[WKey] = WValue.toInt();
    }
    else if (Helix_Type_Reader(WKey).indexOf("bool") >= 0)
    {
      if (WValue == "true")
      {
        jset[WKey] = true;
      }
      else
      {
        jset[WKey] = false;
      }
    }

    else
    {
      LMS.print(WKey, 2);
      LMS.println(" setting: TYPE not identified", 2);
      LMS.println("Settings.type value: " + Helix_Type_Reader(WKey), 2);
    }
  }
  if (serializeJsonPretty(jset, HFS_File) == 0)
  {
    LMS.println("Failed to write Settings", 4);
  }

  HFS_File.close();
}

String Helix_Type_Reader(String Variable)
{
  String TYP;
  HFTS_File = SD.open(SettingsTypes, FILE_READ);
  //  Deserialize the JSON document
  DeserializationError error = deserializeJson(jtset, HFTS_File);
  if (error)
  {
    LMS.println("(TYPE reader) Failed to read file, perform factory reset", 4);
    LMS.println(error.c_str(), 4);
    Helix_Factory_Reset();
  }
  TYP = jtset[Variable].as<String>();
  HFTS_File.close();
  TYP.trim();
  TYP.toLowerCase();
  return TYP;
}

#pragma endregion





#pragma region Serial_Com_File-System-Engine

/*file system for serial comm*/
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Sercomm_Printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
     Sercomm_Println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
     Sercomm_Println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Sercomm_Print("  DIR : ");
       Sercomm_Println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Sercomm_Print("  FILE: ");
      Sercomm_Print(file.name());
      Sercomm_Print("  SIZE: ");
      Sercomm_Println(String(file.size()));
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path)
{
  Sercomm_Printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path))
  {
     Sercomm_Println("Dir created");
  }
  else
  {
     Sercomm_Println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path)
{
  Sercomm_Printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path))
  {
     Sercomm_Println("Dir removed");
  }
  else
  {
     Sercomm_Println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path)
{
  Sercomm_Printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
     Sercomm_Println("Failed to open file for reading");
    return;
  }

  Sercomm_Print("Read from file: ");
  while (file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Sercomm_Printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
     Sercomm_Println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
     Sercomm_Println("File written");
  }
  else
  {
     Sercomm_Println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  Sercomm_Printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
     Sercomm_Println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
     Sercomm_Println("Message appended");
  }
  else
  {
     Sercomm_Println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
  Sercomm_Printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2))
  {
     Sercomm_Println("File renamed");
  }
  else
  {
     Sercomm_Println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path)
{
  Sercomm_Printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
     Sercomm_Println("File deleted");
  }
  else
  {
     Sercomm_Println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path)
{
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file)
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len)
    {
      size_t toRead = len;
      if (toRead > 512)
      {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  }
  else
  {
     Sercomm_Println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file)
  {
     Sercomm_Println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++)
  {
    file.write(buf, 512);
  }
  end = millis() - start;

  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

#pragma endregion





#pragma region Serial_Com_File-System-Callbacks

void sc_filelist()
{
  listDir(SD, "/", 0);
}
void sc_mkdir(const char *path)
{
  createDir(SD, path);
  sc_filelist();
}
void sc_rmdir(const char *path)
{
  removeDir(SD, path);
  sc_filelist();
}
void sc_rmfile(const char *path)
{
  deleteFile(SD, path);
  sc_filelist();
}
void sc_writefile(const char *path, const char *message)
{
  writeFile(SD, path, message);
  Sercomm_Print("CAZZOPATH: ");
   Sercomm_Println(path);
  readFile(SD, path);
}

#pragma endregion





#pragma region Serial_Com_Settings-System-Callbacks

void sc_Settings_List()
{

  if (SD.exists(SettingsFile))
  {
     Sercomm_Println("Settings Found");
    HFS_File = SD.open(SettingsFile, FILE_READ);

    //  Deserialize the JSON document
    DeserializationError error = deserializeJson(jset, HFS_File);

    if (error)
    {
       Sercomm_Println("Failed to read file, PERFORM FACTORY RESET!");
       Sercomm_Println(error.c_str());
    }
    else
    {
      String json_string;
      serializeJsonPretty(jset, Serial);
 serializeJsonPretty(jset, json_string);
  AJAXTerminal_Send(json_string.c_str());
    }
  }
  else
  {
     Sercomm_Println("Settings file not found, PERFORM FACTORY RESET!");
  }
}

void sc_Settings_write(String key, String val)
{
  Helix_Settings_Writer(key, val);
}

void sc_RAW_Settings_write(String key, String val)
{

  Helix_Memory_Writing = true; // turn on memory writing indicator
  GUI_IndicatorCB();           // force gui indicator callback

  if (!SD.begin(SD_CARD_CS, sdCardSpi))
  {
    LMS.println("Card Not present", 2);
    SD_Settings_Write_Error(); // errore scrittura su sd
    return;
  }
  else
  {
    Helix_Memory_State = true;
    HFS_File = SD.open(SettingsFile, FILE_WRITE);

    // if the file opened okay, write to it:
    if (HFS_File)
    {
      if (jset.containsKey(key))
      {
        jset[key] = val;
         Sercomm_Println("key: " + key);
         Sercomm_Println("value: " + val);
         Sercomm_Println("Updated");
         Sercomm_Println("a system reboot is recommended");
      }
      else
      {
         Sercomm_Println("key: " + key);
         Sercomm_Println("Not Found");
         Sercomm_Println("Check the name or add it with Helix Dev Suite");
      }
      if (serializeJsonPretty(jset, HFS_File) == 0) // rendo il file json formattato per leggerlo agevolmente e lo scrivo su sd
      {
         Sercomm_Println("Failed to write Settings");
        SD_Settings_Write_Error();
      }
      HFS_File.close(); // chiudo il file
       Sercomm_Println("Settings Writed");
    }
    else
    {
       Sercomm_Println("Error opening Configuration file");
      SD_Settings_Write_Error();
    }
  }
  Helix_Memory_Writing = false;
  Setting_File_Init();
}

#pragma endregion


void EXT_Settings_write(String key, String val) //deprecated
{

  Helix_Memory_Writing = true; // turn on memory writing indicator
  GUI_IndicatorCB();           // force gui indicator callback

  if (!SD.begin(SD_CARD_CS, sdCardSpi))
  {
    LMS.println("Card Not present", 2);
    SD_Settings_Write_Error(); // errore scrittura su sd
    return;
  }
  else
  {
    Helix_Memory_State = true;
    HFS_File = SD.open(SettingsFile, FILE_WRITE);

    // if the file opened okay, write to it:
    if (HFS_File)
    {
      if (jset.containsKey(key))
      {
        jset[key] = val;
      }
      else
      {
        LMS.print("key: " + key, 4);
        LMS.println("Not Found", 4);
        LMS.println("Check the name or add it with Helix Dev Suite", 4);
      }
      if (serializeJsonPretty(jset, HFS_File) == 0) // rendo il file json formattato per leggerlo agevolmente e lo scrivo su sd
      {
        LMS.println("Failed to write Settings", 4);
        SD_Settings_Write_Error();
      }
      HFS_File.close(); // chiudo il file
      LMS.println("Settings Writed");
    }
    else
    {
      LMS.println("Error opening Configuration file", 4);
      SD_Settings_Write_Error();
    }
  }
  Helix_Memory_Writing = false;
}
