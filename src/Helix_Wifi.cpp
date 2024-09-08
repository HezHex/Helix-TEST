#include <Arduino.h>
#include "Helix_Logger.h"
#include "Helix_Indicators.h"
#include "Helix_Sensors.h"
#include "Helix_Display.h"
#include "Helix_Memory.h"
#include "Helix_serialComm.h"
#include "mbedtls/md.h"        //needed for authentication system
#include "ESPAsyncWebServer.h" //webserver library
#include <WiFi.h>              //Wifi library
#include <esp_wifi.h>          //to change mac address
#include "SD.h"                //needed for filemanager and loading page from sd
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

H_log WL("WiFi Module"); // Helix Logging

void Helix_Main_Server_Init();

AsyncWebServer server(80);
void IRAM_ATTR WiFiEvent(WiFiEvent_t event);
bool WiFi_Error = false;
bool Wifi_Connected = false;
String processor(const String &var);

#pragma region Autentication
const char *L_User = "LoginUser";     // campi del form di autenticazione
const char *L_Password = "LoginPass"; //

String sha1(String payloadStr)
{
  const char *payload = payloadStr.c_str();

  int size = 20;

  byte shaResult[size];

  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;

  const size_t payloadLength = strlen(payload);

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *)payload, payloadLength);
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);

  String hashStr = "";

  for (uint16_t i = 0; i < size; i++)
  {
    String hex = String(shaResult[i], HEX);
    if (hex.length() < 2)
    {
      hex = "0" + hex;
    }
    hashStr += hex;
  }

  return hashStr;
}

void handleLogin(AsyncWebServerRequest *request)
{
  WL.println("Handle login", 1);
  String msg;
  if (request->hasHeader("Cookie"))
  {
    // Print cookies
    WL.print("Found cookie: ", 1);
    String cookie = request->header("Cookie");
    WL.println(cookie);
  }

  if (request->hasArg(L_User) && request->hasArg(L_Password))
  {
    WL.print("Found parameter: ", 1);

    if (request->arg(L_User) == String(login_User) && request->arg(L_Password) == String(login_Password))
    {
      AsyncWebServerResponse *response = request->beginResponse(SD, "/home.html", "text/html", false, processor); // Sends 301 redirect
                                                                                                                  //   response->addHeader("Location", "/");
      response->addHeader("Cache-Control", "no-cache");

      String token = sha1(String(login_User) + ":" + String(login_Password) + ":" + request->client()->remoteIP().toString());
      WL.print("Token: ", 1);
      WL.println(token);
      response->addHeader("Set-Cookie", "ESPSESSIONID=" + token);

      request->send(response);
      WL.println("Log in Successful", 1);
      return;
    }
    msg = "Wrong username/password! try again.";
    WL.println("Log in Failed", 4);

    AsyncWebServerResponse *response = request->beginResponse(SD, "/Login.html", "text/html", false, processor);

    response->addHeader("msg", msg);
    request->send(response);
    return;
  }
}

/**
 * Manage logout (simply remove correct token and redirect to login form)
 */
void handleLogout(AsyncWebServerRequest *request)
{
  WL.println("Disconnection", 1);
  AsyncWebServerResponse *response = request->beginResponse(301); // Sends 301 redirect

  // response->addHeader("Location", "/login.html?msg=User disconnected");
  response->addHeader("Location", "/");
  response->addHeader("Cache-Control", "no-cache");
  response->addHeader("Set-Cookie", "ESPSESSIONID=0");
  request->send(response);
  return;
}
// Check if header is present and correct
bool is_authenticated(AsyncWebServerRequest *request)
{
  WL.println("Enter is_authenticated", 1);
  if (request->hasHeader("Cookie"))
  {
    WL.print("Found cookie: ", 4);
    String cookie = request->header("Cookie");
    WL.println(cookie, 1);

    String token = sha1(String(login_User) + ":" +
                        String(login_Password) + ":" +
                        request->client()->remoteIP().toString());
    //  token = sha1(token);

    if (cookie.indexOf("ESPSESSIONID=" + token) != -1)
    {
      WL.println("Authentication Successful", 1);
      return true;
    }
  }
  WL.println("Authentication Failed", 4);

  return false;
}

#pragma endregion

#pragma region File_Manager
String listFiles(bool ishtml = false);
String humanReadableSize(const size_t bytes)
{
  if (bytes < 1024)
    return String(bytes) + " B";
  else if (bytes < (1024 * 1024))
    return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024))
    return String(bytes / 1024.0 / 1024.0) + " MB";
  else
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
String listFiles(bool ishtml)
{
  String returnText = "";
  WL.println("Listing files stored on SD");
  File root = SD.open("/");
  File foundfile = root.openNextFile();
  if (ishtml)
  {
    returnText += " \n";
    returnText += R"rawliteral(<select name="File_List" multiple="single">)rawliteral";
    returnText += " \n";
  }
  while (foundfile)
  {
    if (ishtml)
    {
      returnText += R"rawliteral(<option value=")rawliteral" + String(foundfile.name()) + R"rawliteral(">)rawliteral" + String(foundfile.name()) + R"rawliteral(</option> )rawliteral";
      returnText += " \n";
    }
    else
    {
      returnText += "File: " + String(foundfile.name()) + "\n";
      returnText += " \n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml)
  {
    returnText += "</select>";
    returnText += " \n";
  }
  root.close();
  foundfile.close();
  return returnText;
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (Fileman_Enabled)
  {
    if (!is_authenticated(request))
    {
      WL.println(F("Go on not login!"));
      request->redirect("/");
    }
    else
    {
      String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
      WL.println(logmessage);

      if (!index)
      {
        logmessage = "Upload Start: " + String(filename);
        // open the file on first call and store the file handle in the request object
        request->_tempFile = SD.open("/" + filename, "w");
        WL.println(logmessage);
      }

      if (len)
      {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
        logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
        WL.println(logmessage);
      }

      if (final)
      {
        logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
        // close the file handle as the upload is now done
        request->_tempFile.close();
        WL.println(logmessage);
        request->redirect("/file_manager");
      }
    }
  }
  else
  {
    WL.println("Filmanager Switch not enabled", 1);
  }
}
void handleDelete(String filename) { SD.remove(filename); }
#pragma endregion

#pragma region Network SSid Scanner
String netsString = ""; // Contains networks ssid
void HtmlNets()
{
  WL.println("WiFi Scan Started", 1);
  String returnText;
  returnText = "";
  returnText += " \n";
  int n = WiFi.scanNetworks();
  WL.println("scan done", 1);
  if (n == 0)
  {
    WL.println("no networks found", 4);
  }
  else
  {

    WL.println(" networks found", 1);
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      WL.print((String)(i + 1), 1);
      WL.print(": ");
      WL.print(WiFi.SSID(i), 1);
      WL.print(" (", 1);
      WL.print((String)WiFi.RSSI(i), 1);
      WL.print(")", 1);
      WL.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*", 1);

      returnText += R"rawliteral(<option value=")rawliteral" +
                    String(WiFi.SSID(i)) +
                    R"rawliteral(">)rawliteral" +
                    String(WiFi.SSID(i)) +
                    R"rawliteral(</option> )rawliteral" +
                    " \n";
      delay(10);
    }
  }
  WL.println("", 1);
  returnText += " \n";
  netsString = returnText;
}

#pragma endregion

#pragma region Ajax_Engine

AsyncEventSource events("/events");

void ajax_event(const char *message, const char *event, uint32_t id)
{
  events.send(message, event, id);
}

AsyncEventSource AJAXSettings("/AJAXSettings");

void ajax_settings_send(const char *message, const char *event)
{
  AJAXSettings.send(message, event, millis());
}

void AJAXSettings_Populate()
{ // populate fields in settings page on load
  WL.println("AJAX Settings page fields population", 1);
  AJAXSettings.send(login_User.c_str(), "login_User", millis());
  AJAXSettings.send(login_Password.c_str(), "login_Password", millis());
  AJAXSettings.send(WiFi_SSID.c_str(), "WiFi_SSID", millis());
  AJAXSettings.send(WiFi_Password.c_str(), "WiFi_Password", millis());
  AJAXSettings.send(Room_Name.c_str(), "Room_Name", millis());
  AJAXSettings.send(MQTT_Server.c_str(), "MQTT_Server", millis());
  AJAXSettings.send(MQTT_Password.c_str(), "MQTT_Password", millis());
  AJAXSettings.send(MQTT_User.c_str(), "MQTT_User", millis());
}

AsyncEventSource AJTerm("/AJTerm");

void AJAXTerminal_Send(const char *message)
{
  if (WebTerminal_Enabled) /// change to terminal enabled
  {
    AJTerm.send(message, "TERMINAL_in", millis());
  }
}

void AJAXTerminal_Send_NOCR(const char *message)
{
  if (WebTerminal_Enabled) /// change to terminal enabled
  {

    AJTerm.send(message, "TERMINAL_in_NOCR", millis());
  }
}

void HomeAjaxUpdate_CB()
{
  WL.println("Ajax Event on Homepage load sent", 1);
  ajax_event(String(HUMIDITY).c_str(), "humidity", millis());
  ajax_event(String(TEMP).c_str(), "temperature", millis());
  ajax_event(String(Room_Setpoint).c_str(), "setpoint", millis());
}

#pragma endregion

#pragma region UTILITY
String ipToString(IPAddress ip)
{
  String s = "";
  for (int i = 0; i < 4; i++)
    s += i ? "." + String(ip[i]) : String(ip[i]);
  return s;
}
String WebDateTime = "";
void printLocalTime() /// forse lo aggiungerei alla home ed al logging system
{
  if (Wifi_Connected)
  {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
      WL.println("Failed to obtain time", 2);
      return;
    }

    WebDateTime = "[" + String(timeinfo.tm_mday) + "/" + String(timeinfo.tm_mon) + "/" + String((timeinfo.tm_year + 1900)) + "-" + String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min) + "] ";
    GUI_Indicators_Time(String(timeinfo.tm_hour), String(timeinfo.tm_min));
  }
}

#pragma endregion

String processor(const String &var)
{
#pragma region Processor File manager
  if (var == "FILELIST")
  {
    return listFiles(true);
  }
  if (var == "HOME_BUTTONS")
  {
    String ret;
    if (Fileman_Enabled)
    {

      ret += R"rawliteral(<div><div class="logout_container">)rawliteral"
             "\n";
      ret += R"rawliteral(<form action="/file_manager">)rawliteral"
             "\n";
      ret += R"rawliteral(<input type = "submit" value = "File Manager">)rawliteral"
             "\n";
      ret += R"rawliteral(</form> </div> </div> )rawliteral"
             "\n";
    }
    if (WebTerminal_Enabled)
    {

      ret += R"rawliteral(<div><div class="logout_container">)rawliteral"
             "\n";
      ret += R"rawliteral(<form action="/TERMINAL">)rawliteral"
             "\n";
      ret += R"rawliteral(<input type = "submit" value = "Terminal">)rawliteral"
             "\n";
      ret += R"rawliteral(</form> </div> </div> )rawliteral"
             "\n";
    }
    if (ADV_WebSettings_Enabled)
    {

      ret += R"rawliteral(<div><div class="logout_container">)rawliteral"
             "\n";
      ret += R"rawliteral(<form action="/ADV_Settings">)rawliteral"
             "\n";
      ret += R"rawliteral(<input type = "submit" value = "Advanced Settings">)rawliteral"
             "\n";
      ret += R"rawliteral(</form> </div> </div> )rawliteral"
             "\n";
    }

    return ret;
  }
  if (var == "SETTINGS_BUTTONS")
  {
    String ret;
    if (ADV_WebSettings_Enabled)
    {

      ret += R"rawliteral(<div><div class="logout_container">)rawliteral"
             "\n";
      ret += R"rawliteral(<form action="/ADV_Settings">)rawliteral"
             "\n";
      ret += R"rawliteral(<input type = "submit" value = "Advanced Settings">)rawliteral"
             "\n";
      ret += R"rawliteral(</form> </div> </div> )rawliteral"
             "\n";
    }
    return ret;
  }

  if (var == "FREESPIFFS")
  {
    return humanReadableSize((SD.totalBytes() - SD.usedBytes()));
  }
  if (var == "USEDSPIFFS")
  {
    return humanReadableSize(SD.usedBytes());
  }
  if (var == "TOTALSPIFFS")
  {
    return humanReadableSize(SD.totalBytes());
  }
#pragma endregion
  if (var == "SIGNALSTRENGTH")
  {
    int strength = 100 + WiFi.RSSI();
    return "WiFi signal power: " + String(strength) + "%";
  }
  if (var == "SW_VERSION")
  {
    return SW_Version;
  }
  if (var == "WIFINETS")
  {
    return netsString;
  }
  if (var == "ROOM_NAME")
  {
    return Room_Name;
  }
  if (var == "SUGGESTEDSERVER")
  {
    return "SERVERS";
  }
  if (var == "ADV_SETTINGS_BODY")
  {

    String ret;

    File file = SD.open(SettingsFile);
    DynamicJsonDocument doc(SettingFileALLOC_SIZE);
    deserializeJson(doc, file);
    JsonObject root = doc.as<JsonObject>();

    for (JsonPair kv : doc.as<JsonObject>())
    {

      String KEY(kv.key().c_str());
      String VALUE = "";
      String val2;

#pragma region ValueType_Identifier                                                 //gira ma è sbagliato, in caso di errori guarda in helix memory.cpp e usa Helix_Type_Reader(String Variable)

      if (kv.value().is<String>())
      {
        VALUE = kv.value().as<String>();
      }
      else if (kv.value().is<int>())
      {

        VALUE = String(kv.value().as<int>());
      }
      else if (kv.value().is<float>())
      {

        float value = kv.value().as<float>();
        char buffer[10];
        sprintf(buffer, "%.2f", value);

        int lastZero = strlen(buffer);
        for (int i = strlen(buffer) - 1; i >= 0; i--)
        {
          if (buffer[i] == '\0' || buffer[i] == '0' || buffer[i] == '.')
            lastZero = i;
          else
            break;
        }

        if (lastZero == 0)
          lastZero++;
        char newValue[lastZero + 1];
        strncpy(newValue, buffer, lastZero);
        newValue[lastZero] = '\0';

        VALUE = String(newValue);
      }
      else if (kv.value().is<long>())
      {

        VALUE = String(kv.value().as<long>());
      }
      else if (kv.value().is<bool>())
      {

        VALUE = String(kv.value().as<bool>());
      }

#pragma endregion

#pragma region BoolValue_Builder
      if (kv.value().is<bool>())
      {
        val2 = R"rawliteral(
                      <select name="@KEY@" id="@KEY@" value="@VAL@"style="border: 1px #4c7faf solid !important;">
                      <option value="true" @SEL_TRUE@>true</option>  
                      <option value="false" @SEL_FALSE@>false</option>  
                      </select>
                      <br><br>
                    )rawliteral";
        if (kv.value().as<bool>())
        {
          val2.replace("@SEL_TRUE@", "selected");
          val2.replace("@SEL_FALSE@", "");
        }
        else
        {
          val2.replace("@SEL_TRUE@", "");
          val2.replace("@SEL_FALSE@", "selected");
        }
      }
#pragma endregion

#pragma region OtherValue_Builder
      else
      {

        val2 = R"rawliteral(
        <input  
                type="text" 
                id="@KEY@" 
                name="@KEY@" 
                value="@VAL@" 
                style=" font-size:1.6em/1.4; 
                        border: 1px #000000 solid !important;"
        >
                        <br><br>
                    )rawliteral";
      }
#pragma endregion

#pragma region Pre-Post_HTML_Code

      String PreCode = R"rawliteral(
                                    <div class="column"> 
                                      <div class="card">
                                        <p"><i></i>@KEY@</p>
                        )rawliteral";

      String PostCode = R"rawliteral(
                                        </p>
                                      </div>
                                    </div>
                          )rawliteral";

#pragma endregion

      val2.replace("@KEY@", KEY);
      val2.replace("@VAL@", VALUE);
      val2 += "\n";
      PreCode.replace("@KEY@", KEY);
      ret = ret + PreCode + val2 + PostCode + "\n";
    }
    file.close();
    return ret;
  }
  return String();
}

void handleSettings(AsyncWebServerRequest *request)
{

  int paramsNr = request->params();
  Serial.println(paramsNr);

  for (int i = 0; i < paramsNr; i++)
  {
    AsyncWebParameter *p = request->getParam(i);
    char ALLOWED_WEB_SETTINGS[] = "login_User login_Password WiFi_Password WiFi_SSID Room_Name MQTT_Server MQTT_User MQTT_Password";
    char *trust = strstr(ALLOWED_WEB_SETTINGS, p->name().c_str());
    if (trust)
    {
      WL.println("SETTING ALLOWED");
      WL.print("Param name: ", 1);
      WL.println(p->name(), 1);
      WL.print("Param value: ", 1);
      WL.println(p->value(), 1);
      WL.println("------", 1);
      Helix_Settings_Writer(p->name(), p->value());
      if (p->name() == "WiFi_Password")
      {
        GUI_ON = false; // turn off gui loop
        WL.println("Restarting Helix Control Panel", 4);
        Helix_Display_BSOD("Updating WiFi Credentials");
        delay(1000);
        ESP.restart();
      }
    }
    else
    {
      WL.println("Parameter: " + p->name() + " NOT ALLOWED - Skipped");
      WL.println("WARNING: possible web settings hack!", 4);
      request->send(200, "text/plain", "Parameter: " + p->name() + " NOT ALLOWED - Skipped, this action will be reported to admin");
    }
  }
  delay(100);
  Helix_Settings_Init();
  AJAXSettings_Populate();
  request->send(SD, "/settings.html", "text/html", false, processor);
}

void handle_ADV_Settings(AsyncWebServerRequest *request)
{
  if (ADV_WebSettings_Enabled)
  {
    int paramsNr = request->params();

    for (int i = 0; i < paramsNr; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      String WValue = p->value();
      String WKey = p->name();
      Helix_Settings_Writer(WKey, WValue);
    }

    Serial.println("Settings Writed");

    delay(1000);
    request->redirect("/ADV_Settings");
    ESP.restart();
  }
  else
  {
    WL.println("ADVSettings Switch not enabled", 1);
  }
}

void handleTerminalSend(AsyncWebServerRequest *request)
{
  if (WebTerminal_Enabled) /// change to terminal enabled
  {
    int paramsNr = request->params();
    for (int i = 0; i < paramsNr; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      String WValue = p->value();
      String WKey = p->name();

      if (WKey == "TB_Send")
      {
        Helix_Sercomm_EXEC(WValue);
        WL.println("Send:" + WValue + " From /TERMINAL page", 4);
      }
    }
    request->send(SD, "/Terminal.html", "text/html", false, processor);
  }
  else
  {
    WL.println("WEBTerminal Switch not enabled", 1);
  };
}

void Helix_Wifi_init()
{

  HtmlNets();

  String hostname = "HELIX";

  WL.println("Wifi Starting", 1);
  WL.check_Mem();

#pragma region MAC HACK

  WL.println("ACTUAL Mac Address: " + WiFi.macAddress());
  uint8_t baseMac[6];                         // Mac address variable
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);    // READ base mac
  baseMac[0] = 0x00;                          // I change the first three values ​​of the mac address with:
  baseMac[1] = 0x09;                          // 00:09:0E which corresponds to the Base Mac of Helix Technologies
  baseMac[2] = 0x0E;                          // that came from: https://macvendors.com/
  esp_wifi_set_mac(WIFI_IF_STA, &baseMac[0]); // Set the hacked mac address
  WL.println("HACKED Mac Address: " + WiFi.macAddress());

#pragma endregion

  server.end();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str()); // define hostname
  WiFi.onEvent(WiFiEvent);
  WiFi.setAutoReconnect(true); // SET RECONNECT TRUE
  WiFi.begin(WiFi_SSID, WiFi_Password);

  WL.check_Mem();

  int retry = 0;
  WL.println("Connecting to WiFi..", 1);

  bool Connect_Timeout = true;

  while (retry <= WiFi_Connections_Retry)
  {
    retry++;
    if (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      WL.println(".", 1);
    }
    else
    {
      // Print ESP Local IP Address
      WL.println("Control Room Ip: " + ipToString(WiFi.localIP()), 1);
      WL.println("RSSI (WiFi strength): " + String(WiFi.RSSI()), 1);
      GUI_Indicators_QrCode(WiFi.localIP().toString()); // SET QRCODE AND LABELS                  //TODO
      retry = WiFi_Connections_Retry + 1;
      Connect_Timeout = false;
      break;
    }
  }
  if (Connect_Timeout)
  {
    WL.println("Too many connection retry! Unable to connect!", 4);
    WiFi_Error = true;
    Force_WARNING = true;
    GUI_IndicatorCB();
  }
  else
  {
    Helix_Main_Server_Init();
  }
}

void Helix_AP_init()
{

  HtmlNets();

  String hostname = "HELIX";

  WL.println("AP Starting", 1);
  WL.check_Mem();

#pragma region MAC HACK

  WL.println("ACTUAL Mac Address: " + WiFi.macAddress());
  uint8_t baseMac[6];                         // Mac address variable
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);    // READ base mac
  baseMac[0] = 0x00;                          // I change the first three values ​​of the mac address with:
  baseMac[1] = 0x09;                          // 00:09:0E which corresponds to the Base Mac of Helix Technologies
  baseMac[2] = 0x0E;                          // that came from: https://macvendors.com/
  esp_wifi_set_mac(WIFI_IF_STA, &baseMac[0]); // Set the hacked mac address
  WL.println("HACKED Mac Address: " + WiFi.macAddress());

#pragma endregion

  server.end();
  WiFi.disconnect();

  WiFi.mode(WIFI_AP);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  
  WiFi.setHostname(hostname.c_str()); // define hostname
  WiFi.onEvent(WiFiEvent);
 // WiFi.setAutoReconnect(true); // SET RECONNECT TRUE
//  WiFi.begin(WiFi_SSID, WiFi_Password);
WiFi.begin();

  WL.check_Mem();

  int retry = 0;
  WL.println("Connecting to WiFi..", 1);

  bool Connect_Timeout = true;

  while (retry <= WiFi_Connections_Retry)
  {
    retry++;
    if (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      WL.println(".", 1);
    }
    else
    {
      // Print ESP Local IP Address
      WL.println("Control Room Ip: " + ipToString(WiFi.localIP()), 1);
      WL.println("RSSI (WiFi strength): " + String(WiFi.RSSI()), 1);
      GUI_Indicators_QrCode(WiFi.localIP().toString()); // SET QRCODE AND LABELS                  //TODO
      retry = WiFi_Connections_Retry + 1;
      Connect_Timeout = false;
      break;
    }
  }
  if (Connect_Timeout)
  {
    WL.println("Too many connection retry! Unable to connect!", 4);
    WiFi_Error = true;
    Force_WARNING = true;
    GUI_IndicatorCB();
  }
  else
  {
    Helix_Main_Server_Init();
  }
}


void IRAM_ATTR WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {

  // case SYSTEM_EVENT_STA_CONNECTED:
  case SYSTEM_EVENT_STA_GOT_IP:
    WL.println("WiFi EVENT: Connected to access point", 1);
    Wifi_Connected = true;
    GUI_Indicators_QrCode(WiFi.localIP().toString()); // SET QRCODE AND LABELS                        //TODO
    GUI_IndicatorCB();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    if (Wifi_Connected)
      WL.println("WiFi EVENT: Disconnected from WiFi access point", 1);
    GUI_Indicators_QrCode("Not Connected"); // TODO
    Wifi_Connected = false;
    GUI_IndicatorCB();
    break;
  case ARDUINO_EVENT_WIFI_AP_START:
    Serial.println("WiFi EVENT: Access point started");
    break;
  case ARDUINO_EVENT_WIFI_AP_STOP:
    Serial.println("WiFi EVENT: Access point  stopped");
    break;
  default:
    break;
  }
}

void Helix_Main_Server_Init()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
                if (is_authenticated(request))
                {
                   request->redirect("/home");                                                  //se loggato redirect to home
                }
                else {
             request->send(SD, "/Login.html", "text/html", false, processor); 
             
                } });

#pragma region Autentication
  server.on("/auth", HTTP_POST, handleLogin);

  server.on("/logout", HTTP_GET, handleLogout);
#pragma endregion

#pragma region Home_Page

  // Setting Temperatura da remoto

  server.on("/tempset", HTTP_GET, [](AsyncWebServerRequest *request)
            {          
                if (!is_authenticated(request)) {
                                                  WL.println(F("Go on not login!"));
                                                  request->redirect("/");
                                                } 
                  else {  
                         request->send(SD, "/home","text/html",false, processor); 
                        } });

  server.on("/UpdHomeAjax", HTTP_POST, [](AsyncWebServerRequest *request) { // questo viene chiamato da una funzione sulla home page on load, e serve per il primo aggiornamento totale
    HomeAjaxUpdate_CB();                                                    // dei valori ajax di temperatura umidità e setpoint, che poi si aggiorneranno solo se cambiano
    request->send(200, "text/plain", "Ajax refreshed");
  });

  server.on("/home", HTTP_GET, [](AsyncWebServerRequest *request)
            {
            if (!is_authenticated(request)) {
                                              WL.println(F("Go on not login!"));
                                              request->redirect("/");
                                            } 
            else {
                  if (request->hasParam("Command")){
                                                      WL.println("Request has param");
                                                      String HTMLCommand = request->getParam("Command")->value();
                                                      WL.println(HTMLCommand);
                                                    if (HTMLCommand == "upSET"){
                       	                                                        Set_Temp(true); 	
                                                                                Display_Activity_CB(true);
                                                                               }
                                                    else if (HTMLCommand == "downSET"){
                       	                                                                Set_Temp(false); 	
                                                                                        Display_Activity_CB(true);
                                                                                      }
                                                    }

            String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();
            WL.println(logmessage);
            request->send(SD, "/home.html","text/html",false, processor);
            } });
#pragma endregion

#pragma region File_Manager
  server.on("/file_manager", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if (Fileman_Enabled)
              {
                if (!is_authenticated(request))
                {
                  WL.println(F("Go on not login!"));
                  request->redirect("/");
                }
                else
                {
                  String logmessage = "Client:" + request->client()->remoteIP().toString() + +" " + request->url();
                  WL.println(logmessage);
                  request->send(SD, "/FileManager.html", "text/html", false, processor);
                }
              }
              else
              {
                WL.println("Filmanager Switch not enabled", 1);
              } });

  server.on(
      "/upload", HTTP_POST, [](AsyncWebServerRequest *request)
      { 
         if (Fileman_Enabled) { request->send(200);}
         else { WL.println("Filmanager Switch not enabled",1); } },
      handleUpload);

  server.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request)
            { 
                if (Fileman_Enabled) { 
              if (!is_authenticated(request)) {
                                                  WL.println(F("Go on not login!"));
                                                  request->redirect("/");
                                                } 
                  else {  
              int params = request->params();
              for (int i = 0; i < params; i++)
                   {
                    AsyncWebParameter* p = request->getParam(i);
                    WL.print("Param name: ");
                    WL.println(p->name());
                    WL.print("Param value: ");
                    WL.println(p->value());
                      if (p->name() == "File_List") {
                          WL.println("try to delete file");
                          handleDelete("/" + p->value());
                                                    }
                   }
              request->redirect("/file_manager"); }}
                else {
  WL.println("Filmanager Switch not enabled",1);
} });

  server.on("/download", HTTP_POST, [](AsyncWebServerRequest *request)
            { 
if (Fileman_Enabled) { 
                      if (!is_authenticated(request)) {
                                                        WL.println(F("Go on not login!"));
                                                        request->redirect("/");
                                                      } 
                      else {  
                             int params = request->params();
                            for (int i = 0; i < params; i++)
                                                            {
                                                              AsyncWebParameter* p = request->getParam(i);
                                                              WL.print("Param name: ");
                                                              WL.println(p->name());
                                                              WL.print("Param value: ");
                                                              WL.println(p->value());
                                                              if (p->name() == "File_List") {
                                                                if(p->value() != NULL){
                                                                                              String filename = "/" + p->value();
                                                                                              WL.println("Downloading: " + filename);
                                                                                              request->send(SD,filename.c_str(),"text/plain",true);
                                                                                      }
                                                                                            }
                                                            }
                            }
                      }
else {
      WL.println("Filmanager Switch not enabled",1);
     } });

#pragma endregion

#pragma region Settings_Page
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                   if (!is_authenticated(request)) {
                                                  WL.println(F("Go on not login!"));
                                                  request->redirect("/");
                                                } 
                  else {  
    String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();
    WL.println(logmessage);
    request->send(SD, "/settings.html","text/html",false, processor); 

                  } });

  server.on("/settings_set", handleSettings);

  server.on("/UpdSettingsAjax", HTTP_POST, [](AsyncWebServerRequest *request) { // questo viene chiamato da una funzione sulla Settings page on load, e serve per il primo aggiornamento totale
    AJAXSettings_Populate();                                                    // dei campi variabili, che poi si aggiorneranno solo se cambiano
    request->send(200, "text/plain", "Ajax refreshed");
  });

  server.on("/ADV_Settings", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if (ADV_WebSettings_Enabled)
              {
                if (!is_authenticated(request))
                {
                  WL.println(F("Go on not login!"));
                  request->redirect("/");
                }
                else
                {
                  String logmessage = "Client:" + request->client()->remoteIP().toString() + +" " + request->url();
                  WL.println(logmessage);
                  request->send(SD, "/Advanced_Settings.html", "text/html", false, processor);
                }
              }
              else
              {
                WL.println("ADV_Settings Switch not enabled", 1);
              } });
  server.on("/ADV_settings_set", handle_ADV_Settings);
#pragma endregion

#pragma region Web_Terminal

  server.on("/TERMINAL", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if (WebTerminal_Enabled)                                                                                                   ///change to terminal enabled
              {
                if (!is_authenticated(request))
                {
                  WL.println(F("Go on not login!"));
                  request->redirect("/");
                }
                else
                {
                  String logmessage = "Client:" + request->client()->remoteIP().toString() + +" " + request->url();
                  WL.println(logmessage);
                  request->send(SD, "/Terminal.html", "text/html", false, processor);
                }
              }
              else
              {
                WL.println("WebTerminal Switch not enabled", 1);
              } });
  server.on("/TERMINAL_SEND", handleTerminalSend);

#pragma endregion

#pragma region ServeStatics
  server.serveStatic("/HELIX.css", SD, "HELIX.css");
  server.serveStatic("/Font.otf", SD, "Font.otf");
  server.serveStatic("/Micro.ttf", SD, "Micro.ttf");
  server.serveStatic("/Logo.png", SD, "Logo.png");
  //  server.serveStatic("/jquery.min.js", SD, "jquery.min.js");
#pragma endregion

#pragma region Ajax_system
  // Handle Web Server Events (Ajax engine)

  AJTerm.onConnect([](AsyncEventSourceClient *client)
                   {
                      if(client->lastId()){
                        Serial.print(WL.H_Prefix(""));
                        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
                                          }
                      // send event with message "hello!", id current millis
                      // and set reconnect delay to 1 second
                    client->send("hello!", NULL, millis(), 10000); });
  server.addHandler(&AJTerm);

  events.onConnect([](AsyncEventSourceClient *client)
                   {
                      if(client->lastId()){
                        Serial.print(WL.H_Prefix(""));
                        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
                                          }
                      // send event with message "hello!", id current millis
                      // and set reconnect delay to 1 second
                    client->send("hello!", NULL, millis(), 10000); });
  server.addHandler(&events);

  AJAXSettings.onConnect([](AsyncEventSourceClient *client)
                         {
                      if(client->lastId()){
                        Serial.print(WL.H_Prefix(""));
                        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
                                          }
                      // send event with message "hello!", id current millis
                      // and set reconnect delay to 1 second
                    client->send("hello!", NULL, millis(), 10000); });
  server.addHandler(&AJAXSettings);

#pragma endregion

  // init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  printLocalTime();
  server.begin();
}
