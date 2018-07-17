#include <SPI.h>
#include <WiFi.h>
#include <FS.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <string.h>

HardwareSerial Serial1(2);
// WiFi
//const char* ssid     = "ESP32";
//const char* password = "812345679";

//char ssid_sta[] = "Pinetwork";
//char password_sta[] = "Ch@rt2o23";

//char ssid_sta[]     = "IOD-GNOC";
//char password_sta[] = "1p5t@r99";

char ssid_sta[] = "Tenda1";     //  your network SSID (name)
char password_sta[] = "812345679";  // your network password


struct SerialData {
    String magic, SBit, KBit, CrcBit, AntStatus, SysStatus, SatName, Rssi, LocFreq, TracFreq, GpsLat, GpsLon, Head, Bow, AzDeg;
    String ElDeg, PolDeg, VoltIn, VoltAnt, VoltLnb, EndBit;
    String end_string;
} __attribute__((packed));


WebServer server(80);

bool is_authentified() {
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}

void handle_login() {
  String msg;
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie login: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if ( cookie == "ESPSESSIONID=1") {
      Serial.println("Logged in Successful123");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      String path = "/setup.html";
      String contentType = get_content_type(path);
      if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return;
      }

    }
  }
  if (server.hasArg("DISCONNECT")) {
    Serial.println("Disconnection");
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
    server.send(301);
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
    if (server.arg("USERNAME") == "admin" &&  server.arg("PASSWORD") == "admin") {
      Serial.println("Log2 in Successful");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      String path1 = "/setup.html";
      String contentType = get_content_type(path1);
      if (SPIFFS.exists(path1)) {
        File file = SPIFFS.open(path1, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return;
      }
    }
    msg = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String path = "/login.html";
  String contentType = get_content_type(path);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return;
  }
  Serial.println("\File not found");
  return;
}

void handle_info() {
  String path = "/info.html";
  String contentType = get_content_type(path);
  if (SPIFFS.exists(path)) {
    Serial.println("DONE1");
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return;
  }
}

void handle_data(){
  SerialData serialdata;
  String data_buff;
  int startflag = 0;
  int timeout = 0;
  int datalength;
  Serial.println("handle data");
  //Request data
  Serial1.print("[Q1]");
  while (timeout < 15){
    //Serial.print("TimeOut:");
    //Serial.println(timeout);
    if(Serial1.available() > 0 and startflag < 2 and data_buff.length() < 90){
      char msg = Serial1.read();
      if(msg == '[' or startflag == 1){
        data_buff += msg;
        startflag = 1;
        timeout = 0;
        if (msg == ']' and data_buff.length() == 89){
          Serial.println(data_buff.length());
          startflag = 2;
          timeout = 15;
          Serial.println("END");
        }
      }else{
        data_buff = "";
        timeout = timeout + 1 ;
        delay(1);
        Serial.println("NO DATA");
        Serial1.print("[Q1]");
      }
    }else{
        Serial.print("dataLength:");
        Serial.println(data_buff.length());
        data_buff = "";
        timeout = timeout + 1 ;
        delay(1);
        Serial.println("NO DATA");
        Serial1.print("[Q1]");
    }
  }
  serialdata.magic = data_buff.substring(0,1);
  serialdata.SBit = data_buff.substring(1,2);
  serialdata.KBit = data_buff.substring(2,3);
  serialdata.AntStatus = data_buff.substring(3,4);
  serialdata.SysStatus = data_buff.substring(4,5);
  serialdata.SatName = data_buff.substring(5,25);
  serialdata.Rssi = data_buff.substring(25,28);
  serialdata.LocFreq = data_buff.substring(28,33);
  serialdata.TracFreq = data_buff.substring(33,38);
  serialdata.GpsLat = data_buff.substring(38,48);
  serialdata.GpsLon = data_buff.substring(48,58);
  serialdata.Head = data_buff.substring(58,62);
  serialdata.Bow = data_buff.substring(62,66);
  serialdata.AzDeg = data_buff.substring(66,70);
  serialdata.ElDeg = data_buff.substring(70,74);
  serialdata.PolDeg = data_buff.substring(74,78);
  serialdata.VoltIn = data_buff.substring(78,81);
  serialdata.VoltAnt = data_buff.substring(81,84);
  serialdata.VoltLnb = data_buff.substring(84,87);
  serialdata.EndBit = data_buff.substring(87);
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["AntStatus"] = serialdata.AntStatus;
  root["SysStatus"] = serialdata.SysStatus;
  root["SatName"] = serialdata.SatName;
  root["Rssi"] = serialdata.Rssi;
  root["LocFreq"] = serialdata.LocFreq;
  root["TracFreq"] = serialdata.TracFreq;
  root["GpsLat"] = serialdata.GpsLat;
  root["GpsLon"] = serialdata.GpsLon;
  root["Head"] = serialdata.Head;
  root["Bow"] = serialdata.Bow;
  root["AzDeg"] = serialdata.AzDeg;
  root["ElDeg"] = serialdata.ElDeg;
  root["PolDeg"] = serialdata.PolDeg;
  root["VoltIn"] = serialdata.VoltIn;
  root["VoltAnt"] = serialdata.VoltAnt;
  root["VoltLnb"] = serialdata.VoltLnb;
  String msg;
  root.printTo(msg);
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200,"application/json",msg);  
  return;
}

void handle_setup() {
  String msg;
  String StartBit = "[";
  String StopBit = "]";
  String SBit = "W";
  String KBit;
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if ( cookie == "ESPSESSIONID=1") {
      Serial.println("Logged in Successful123");
      if (server.hasArg("plain")){
        String data = server.arg("plain");
        Serial.println(data);
        StaticJsonBuffer<600> json_rx_buffer;
        JsonObject& jObject = json_rx_buffer.parseObject(data);
        if (jObject["topic"] == "set_heading"){
          int heading = jObject["data"]["heading"];
          Serial.println(heading);
          int bowofs = jObject["data"]["bow"];
          Serial.println(bowofs);
          KBit = "2";
          // Serial1.print(StartBit);
          // Serial1.print(KBit);
          // Serial1.printf("%04d", heading);
          // Serial1.printf("%04d", bowofs);
          // Serial1.print(StopBit);
          char heading_buff[4];
          char bowofs_buff[4];
          sprintf(heading_buff, "%04d",heading);
          sprintf(bowofs_buff, "%04d",bowofs);
          int crc_value = crc_encoding(String(heading_buff)+String(bowofs_buff));
          String msg_send = StartBit+SBit+KBit+heading_buff+bowofs_buff+crc_value+StopBit;
          Serial.print(msg_send);
          serial_write_string(msg_send);
          bool result_set_data = check_setup_data(KBit);
          Serial.print("RESULT1:");
          Serial.print(result_set_data);
          StaticJsonBuffer<50> json_tx_buffer;
          JsonObject& root = json_tx_buffer.createObject();
          if (result_set_data == true){
            root["result_set_heading"] = "SUCCESS";
            String msg;
            root.printTo(msg);
            server.sendHeader("Cache-Control", "no-cache");
            server.send(200,"application/json",msg);  
          }else{
            root["result_set_heading"] = "FAILURE";
            String msg;
            root.printTo(msg);
            server.sendHeader("Cache-Control", "no-cache");
            server.send(200,"application/json",msg); 
          }
        }else if (jObject["topic"] == "sat_select"){
          String sat_id = jObject["sat_id"];
          if (sat_id == "1"){
            String sat_info = read_file_data(SPIFFS, "/profile/thaicom4.cfg");
            Serial.println(sat_info);
            server.send(200,"application/json",sat_info);
          }else if (sat_id == "2"){
            String sat_info = read_file_data(SPIFFS, "/profile/thaicom5.cfg");
            Serial.println(sat_info);
            server.send(200,"application/json",sat_info);
          }else if (sat_id == "3"){
            String sat_info = read_file_data(SPIFFS, "/profile/thaicom6.cfg");
            Serial.println(sat_info);
            server.send(200,"application/json",sat_info);
          }else if (sat_id == "4"){
            String sat_info = read_file_data(SPIFFS, "/profile/thaicom8.cfg");
            Serial.println(sat_info);
            server.send(200,"application/json",sat_info);
          }
        }else if(jObject["topic"] == "set_satellite"){
          write_file(SPIFFS, "/conf/satellite.conf",jObject["data"]);
          int sateliite_id = jObject["data"]["sateliite_id"];
          String satellite_name = jObject["data"]["satellite_name"];
          String satellite_location = jObject["data"]["satellite_location"];
          int local_frequency = jObject["data"]["local_frequency"];
          int lnb_tone = jObject["data"]["lnb_tone"];
          int rx_pol = jObject["data"]["rx_pol"];
          int tx_pol = jObject["data"]["tx_pol"];
          int modem_freq = jObject["data"]["modem_freq"];
          int symbol_rate = jObject["data"]["symbol_rate"];
          String nid = jObject["data"]["nid"];
          KBit = "3";
          //
          char modem_freq_buff[8];
          char symbol_rate_buff[6];
          char local_frequency_buff[5];
          char rx_pol_buff[1];
          char tx_pol_buff[1];
          char lnb_tone_buff[1];
          // Serial1.print(StartBit);
          // Serial1.print(KBit);
          // Serial1.printf("%08d", modem_freq);
          // Serial1.printf("%06d", symbol_rate);
          // Serial1.print(padding_string(satellite_name,20));
          // Serial1.print(satloct_convert(satellite_location));
          // Serial1.printf("%05d", local_frequency);
          // Serial1.printf("%01d",rx_pol);
          // Serial1.printf("%01d",tx_pol);
          // Serial1.printf("%01d",lnb_tone);
          // Serial1.print(StopBit);
          //
          sprintf(modem_freq_buff, "%08d", modem_freq);
          sprintf(symbol_rate_buff, "%06d", symbol_rate);
          String satellite_name_buff = padding_string(satellite_name,20);
          String satloct_convert_buff = satloct_convert(satellite_location);
          sprintf(local_frequency_buff,"%05d", local_frequency);
          sprintf(rx_pol_buff, "%01d", rx_pol);
          sprintf(tx_pol_buff, "%01d", tx_pol);
          sprintf(lnb_tone_buff, "%01d", lnb_tone);
          String msg_data = String(modem_freq_buff)+String(symbol_rate_buff)+satellite_name_buff+satloct_convert_buff+String(local_frequency_buff)+String(local_frequency_buff)+String(rx_pol_buff)+String(tx_pol_buff)+String(lnb_tone_buff);
          int crc_value = crc_encoding(msg_data);
          String msg_send = StartBit+SBit+KBit+msg_data+crc_value+StopBit;
          
          Serial.println(msg_send);
          serial_write_string(msg_send);
          bool result_set_data = check_setup_data(KBit);
          Serial.print("RESULT1:");
          Serial.print(result_set_data);
          StaticJsonBuffer<50> json_tx_buffer;
          JsonObject& root = json_tx_buffer.createObject();
          if (result_set_data == true){
            root["result_set_satellite"] = "SUCCESS";
            String msg;
            root.printTo(msg);
            server.sendHeader("Cache-Control", "no-cache");
            server.send(200,"application/json",msg);  
          }else{
            root["result_set_satellite"] = "FAILURE";
            String msg;
            root.printTo(msg);
            server.sendHeader("Cache-Control", "no-cache");
            server.send(200,"application/json",msg); 
          }
        }else if(jObject["topic"] == "set_angle"){
          int az = jObject["data"]["az"];
          int el = jObject["data"]["el"];
          int pol = jObject["data"]["pol"];
          KBit = "4";
          char az_buff[4];
          char el_buff[4];
          char pol_buff[4];
          sprintf(az_buff, "%04d", az);
          sprintf(el_buff, "%04d", el);
          sprintf(pol_buff, "%04d", pol);
          // Serial1.print(StartBit);
          // Serial1.print(KBit);
          // Serial1.printf("%04d", az);
          // Serial1.printf("%04d", el);
          // Serial1.printf("%04d", pol);
          // Serial1.print(StopBit);
          String msg_data = String(az_buff)+String(el_buff)+String(pol_buff);
          int crc_value = crc_encoding(msg_data);
          String msg_send = StartBit + SBit + KBit + msg_data + crc_value + StopBit;
          Serial.print(msg_send);
          serial_write_string(msg_send);
          bool result_set_data = check_setup_data(KBit);
          Serial.print("RESULT1:");
          Serial.print(result_set_data);
          StaticJsonBuffer<50> json_tx_buffer;
          JsonObject& root = json_tx_buffer.createObject();
          if (result_set_data == true){
            root["result_set_angle"] = "SUCCESS";
            String msg;
            root.printTo(msg);
            server.sendHeader("Cache-Control", "no-cache");
            server.send(200,"application/json",msg);  
          }else{
            root["result_set_angle"] = "FAILURE";
            String msg;
            root.printTo(msg);
            server.sendHeader("Cache-Control", "no-cache");
            server.send(200,"application/json",msg); 
          }
        }else if(jObject["topic"] == "set_voltage"){
          int vsatvolt = jObject["data"]["VsatVolt"];
          KBit = "5";
          char vsatvolt_buff[2];
          // Serial1.print(StartBit);
          // Serial1.print(KBit);
          // Serial1.printf("%02d", vsatvolt);
          // Serial1.print(StopBit);
          sprintf(vsatvolt_buff, "%02d", vsatvolt);
          String msg_data = String(vsatvolt_buff);
          int crc_value = crc_encoding(msg_data);
          String msg_send = StartBit + SBit + KBit + msg_data + crc_value + StopBit;
          Serial.print("VOLTAGE:");
          Serial.print(msg_send);
          serial_write_string(msg_send);
          bool result_set_data = check_setup_data(KBit);
          Serial.print("RESULT1:");
          Serial.print(result_set_data);
          StaticJsonBuffer<50> json_tx_buffer;
          JsonObject& root = json_tx_buffer.createObject();
          if (result_set_data == true){
            root["result_set_voltage"] = "SUCCESS";
            String msg;
            root.printTo(msg);
            server.sendHeader("Cache-Control", "no-cache");
            server.send(200,"application/json",msg);  
          }else{
            root["result_set_voltage"] = "FAILURE";
            String msg;
            root.printTo(msg);
            server.sendHeader("Cache-Control", "no-cache");
            server.send(200,"application/json",msg); 
          }
        }else{
            Serial.println("faileddddd");
        }
      }
      Serial.println("web completed1");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      String path = "/setup.html";
      String contentType = get_content_type(path);
      if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return;
        }
      
      }
    }
    if (server.hasArg("DISCONNECT")) {
      Serial.println("Disconnection");
      server.sendHeader("Location", "/login");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
      server.send(301);
      return;
    }
    if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
      if (server.arg("USERNAME") == "admin" &&  server.arg("PASSWORD") == "admin") {
        Serial.println("Log1 in Successful");
        String data = server.arg("plain");
        Serial.println(data);
        StaticJsonBuffer<1000> jBuffer;
        JsonObject& jObject = jBuffer.parseObject(data);
        server.sendHeader("Cache-Control", "no-cache");
        server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
        String path = "/setup.html";
        String contentType = get_content_type(path);
        if (SPIFFS.exists(path)) {
          File file = SPIFFS.open(path, "r");
          size_t sent = server.streamFile(file, contentType);
          file.close();
          return;
        }
      }
      msg = "Wrong username/password! try again.";
      Serial.println("Log in Failed");
    }
    String path = "/login.html";
    String contentType = get_content_type(path);
    if (SPIFFS.exists(path)) {
      File file = SPIFFS.open(path, "r");
      size_t sent = server.streamFile(file, contentType);
      file.close();
      return;
    }
    Serial.println("\File not found");
    return;
  
}


void get_data(){
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if ( cookie == "ESPSESSIONID=1") {
        Serial.println("Logged in Successful123");
        String sat_info = read_file_data(SPIFFS, "/conf/satellite.conf");
        if(sat_info != NULL){
          server.send(200,"application/json",sat_info);
        }
    }
  }
}

void handle_data_setup(){
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if ( cookie == "ESPSESSIONID=1") {
        Serial.println("Logged in Successful1234");
        String sat_info = read_file_data(SPIFFS, "/conf/satellite.conf");
        if(sat_info != NULL){
          server.send(200,"application/json",sat_info);
        }
    }
  }
}
//root page can be accessed only if authentification is ok
void handle_root() {
  Serial.println("Enter handle_root");
  String header;
  if (!is_authentified()) {
    server.sendHeader("Location", "/info");
    server.sendHeader("Cache-Control", "no-cache");
    return;
  }
}

void handle_not_found() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

String get_content_type(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}


void list_dir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        list_dir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void read_file(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }
  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
}

String read_file_data(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);
  File filecfg = SPIFFS.open(path);
  if (!filecfg || filecfg.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return "failed";
  }
  Serial.println("- read from file:");
  String data = filecfg.readString();
  return data;
}

void write_file(fs::FS &fs, const char * path, String message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

bool handle_file_read(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "info.html";
  String contentType = get_content_type(path);
  //String sat_info="aaaa";
  //server.send(200,"text/plain",sat_info);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
     
    file.close();
    return true;
  }
  Serial.println("\File not found");
  return false;
}
String padding_string(String str,int str_length){
  int num_length = str_length - str.length();
  for(int i = 0; i < num_length ; i++){
    str += '.';
  }
  return str;
}
int crc_encoding(String str){
  int crc ;
  for( int i = 0; i < str.length(); i++){
    crc = crc^str[i];
  }
  return crc;
}
bool check_setup_data(String KBit){
  int count = 0;
  int DATA_LENGTH = 5;
  int flag = 0;
  String data_buff;
  String SUCCESS_RESULT = "1";
  String correct_msg = "[R"+ KBit + SUCCESS_RESULT +"]";
  while (count < 15){
    Serial.print("START:");
    Serial.println(count);
    if(Serial1.available() > 0 and flag < 2 and data_buff.length() < DATA_LENGTH){
      char msg = Serial1.read();
      Serial.println(msg);
      if (msg == '[' or flag == 1){
        data_buff += msg;
        flag = 1;
        count = 0;
        Serial.println(data_buff);
        if(msg == ']' and data_buff.length() == DATA_LENGTH){
          Serial.println(data_buff);
          flag = 2;
          count = 15;
        }
      }else{
        data_buff = "";
        count = count+1;
        Serial.println("NO DATA");

      }
    }else{
      data_buff = "";
      count = count+1;
      Serial.println("NO DATA");
    }
  }
  Serial.println(data_buff);
  if( data_buff == correct_msg){
    return true;
  }else{
    return false;
  }


}
void serial_write_string(String stringData) { 

  for (int i = 0; i < stringData.length(); i++)
  {
    Serial1.write(stringData[i]);  
  }

}
String satloct_convert(String str){
  int str_length = str.length();
  char buff[5];
  char direction_name = str.charAt(str_length-1);
  String direction_str;
  if (direction_name == 'E'){
    direction_str = "+";
  }else if (direction_name == 'W'){
    direction_str = "-";
  }
  String loct = str.substring(0,str_length-1);
  int loct1 = loct.toFloat() * 10;
  sprintf(buff, "%04d",loct1);
  direction_str += buff;
  //Serial.println(direction1);
  return direction_str;
}
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200); 
  /*
  //ACCESS POINT MODE
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
  */
  Serial.print("Connecting to ");
  Serial.println(ssid_sta);
  WiFi.disconnect();
  WiFi.begin(ssid_sta, password_sta);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid_sta);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //SPIFFS.begin(true);
  if (!SPIFFS.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  list_dir(SPIFFS, "/", 0);
  Serial.println( "Test complete" );

  server.onNotFound([]() {
    if (!handle_file_read(server.uri()))
      server.send(404, "text/plain", "404: NOT FOUND");
  });
  server.on("/requestdata",handle_data);
  server.on("/requestdatasetup",handle_data_setup);
  server.on("/", handle_root);
  server.on("/info", handle_info);
  server.on("/setup", handle_setup);
  server.on("/setup.html", handle_setup);
  server.on("/login", handle_login);
  // server.on("/inline", []() {
  //   server.send(200, "text/plain", "this works without need of authentification");
  // });
  const char * headerkeys[] = {"User-Agent", "Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  Serial.println("HTTP server started");
   
  
}

void loop(void) {
  server.handleClient();
  //GetData();
  
  
  /*
  if(server.handleClient()){
    if(Serial1.available()>0){
    String msg = Serial1.readString();
    Serial.print("Echo:");
    Serial.print(msg);
  }
  }
  */
}
