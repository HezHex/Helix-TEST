#include <Arduino.h>
#include "ui.h"
#include <SHT21_TTGO.h>
#include "Helix_Logger.h"
#include "Helix_Memory.h"

#include "Helix_WiFi.h"

// #define SDA 12 // Pin for I2C SDA
// #define SCL 13 // Pin for I2C SCL

// TwoWire twi = TwoWire(1);
// SHT21_TTGO sht = SHT21_TTGO(&twi);

SHT21_TTGO sht;

H_log LS("SENSOR");

float TEMP;         // variable to store temperature
float HUMIDITY;     // variable to store humidity
float ReadTemp;     // Temporary variable to compare temperature
float ReadHumidity; // Temporary variable to compare humidity

void Helix_Sensors_loop();
void GUI_Temp_Update(void *ptr);
void GUI_Hum_update(void *ptr);

void GUI_SetT_Update(void *ptr);

void WriteTempSet_CB(void *arg);
const esp_timer_create_args_t WriteTempSetTimer_args = { //   TIMER per scrittura Setpoint auto resettante per pressioni multiple
    .callback = &WriteTempSet_CB,                        //   Setta il setpoint dopo 3 secondi dall'ultima pressione
    .name = "WriteTempSetTimer"};                        //
esp_timer_handle_t WriteTempSet_Timer;                   //

void Helix_Sensors_init()
{

  // if (!twi.begin(SDA, SCL)) // begin I2C of SHT sensor
  //  {
  //    LS.println("I2C FAIL to initialize", 4);
  //  }
  //  else
  //  {
  //    LS.println("I2C Started", 1);
  //  }

  char ChrSetp[8];
  dtostrf(Room_Setpoint, 1, 1, ChrSetp); // Adatta il setpoint ad essere scritta
  lv_label_set_text(ui_LABRoomSetpoint, ChrSetp);

  esp_timer_create(&WriteTempSetTimer_args, &WriteTempSet_Timer); // Creo il timer per la scrittura setpoint con touch multipli

  LS.println("initialized", 1);
}

void Helix_Sensors_loop()
{

  ReadTemp = sht.getTemperature(); // get temp from SHT

  ReadTemp = ReadTemp + TEMPERATURE_SENSOR_CALIBRATION_VALUE; // parametro di calibrazione TEMPERATURA

  char str[6];                  //
  dtostrf(ReadTemp, 5, 1, str); // arrotondamento brutale della TEMPERATURA ad una cifra dopo la virgola
  ReadTemp = std::stof(str);    //

  ReadHumidity = sht.getHumidity(); // get humidity from SHT
  str == "";                        //
  dtostrf(ReadHumidity, 5, 1, str); // arrotondamento brutale della UMIDITA' ad una cifra dopo la virgola
  ReadHumidity = std::stof(str);    //

  if (ReadTemp != TEMP) // Controlla se il valore è cambiato (Controllo a differenza del valore)
  {
    TEMP = ReadTemp;                      // Carica il nuovo al posto del vecchio (Controllo a differenza del valore)
    lv_async_call(GUI_Temp_Update, NULL); // chiamata thread safe per aggiornare il label Temperatura
    ajax_event(String(TEMP).c_str(), "temperature", millis());
    // NB qui devi inviare i messaggi MQTT della temperatura
  }

  if (ReadHumidity != HUMIDITY) // Controlla se il valore è cambiato (Controllo a differenza del valore)
  {
    HUMIDITY = ReadHumidity;             // Carica il nuovo al posto del vecchio (Controllo a differenza del valore)
    lv_async_call(GUI_Hum_update, NULL); // chiamata thread safe per aggiornare il label umidità
    ajax_event(String(HUMIDITY).c_str(), "humidity", millis());
    //  NB qui devi inviare i messaggi MQTT della umidità
  }
}

void GUI_Temp_Update(void *ptr)
{

  char TempString[8];
  dtostrf(TEMP, 1, 1, TempString); // Adatta la teperatura ad essere scritta
  if (Sensors_LOG)
  {
    LS.print("Temp: "); // print readings
    LS.println(TempString);
  }

  if (lv_obj_is_valid(ui_LABRoomTemp)) // controlla se esiste il label
  {
    lv_label_set_text_fmt(ui_LABRoomTemp, "%s°", TempString); // scrive la temperatura sul label del mainscreen
  }
  if (lv_obj_is_valid(ui_LABIdleTemp)) // controlla se esiste il label
  {
    lv_label_set_text_fmt(ui_LABIdleTemp, "%s°", TempString); // scrive la temperatura sul label dell'idlescreen
  }
}

void GUI_Hum_update(void *ptr)
{
  char HumString[8];
  dtostrf(HUMIDITY, 1, 1, HumString); // Adatta l'umidita' ad essere scritta
  if (Sensors_LOG)
  {
    LS.print("Humidity: ");
    LS.println(HumString);
  }
  String totalHum = HumString;
  totalHum += "%";
  if (lv_obj_is_valid(ui_LABRoomHum))
  {
    lv_label_set_text_fmt(ui_LABRoomHum, "RH %s", totalHum);
    // lv_label_set_text(ui_LABRoomHum, HumString);
  }
}

void Set_Temp(bool up, float NewSetPoint /*= NULL*/)
{
  if (NewSetPoint == NULL)
  {
    if (up == true)
    {
      Room_Setpoint = (Room_Setpoint + SETPOINT_INC_VALUE);
    }
    else
    {
      Room_Setpoint = (Room_Setpoint - SETPOINT_INC_VALUE);
    }
  }
  else
  {
    Room_Setpoint = NewSetPoint;
  }

  lv_async_call(GUI_SetT_Update, NULL);
  ajax_event(String(Room_Setpoint).c_str(), "setpoint", millis());
  //    TIMER per scrittura Setpoint auto resettante per pressioni multiple
  if (esp_timer_is_active(WriteTempSet_Timer))
  {                                                    //    se il timer è attivo
    esp_timer_stop(WriteTempSet_Timer);                //    lo fermo
    esp_timer_start_once(WriteTempSet_Timer, 3000000); //    e lo faccio ripartire
  }                                                    //
  else
  {                                                    //    altrimenti
    esp_timer_start_once(WriteTempSet_Timer, 3000000); //    lo avvio per la prima volta
  }
}

void GUI_SetT_Update(void *ptr)
{
  char ChrSetp[8];
  dtostrf(Room_Setpoint, 1, 1, ChrSetp); // Adatta il setpoint ad essere scritto
  lv_label_set_text(ui_LABRoomSetpoint, ChrSetp);
}

void WriteTempSet_CB(void *arg)
{

  Helix_Update_Settings(); // aggiorna tutte le variabili sulla SD compresa la temperatura appena settata
  // ajax_event(String(Room_Setpoint).c_str(), "setpoint", millis());                                 // invia un evento ajax per aggiornare il valore sulla pagina web

  // MQTT SEND Room_Setpoint                                                                             MQTTSEND ROOM SETPOINT
}
