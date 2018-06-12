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
    int Heading;
    int BowOfs;
    int DvbFreq;
    int DvbSymb;
    int SatLoct;
    int SatFreq;
    int RxPol;
    int TxPol;
    int Gen22K;
    int SetAzDeg;
    int SetElDeg;
    int SetPolDeg;
    int SetVoltIn;
    int SetVoltAnt;
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
void handleLogin() {
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
      String contentType = getContentType(path);
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
      String contentType = getContentType(path1);
      if (SPIFFS.exists(path1)) {
        File file = SPIFFS.open(path1, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return;
      }
      //server.send(301);
      //return;
    }
    msg = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String path = "/login.html";
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return;
  }
  Serial.println("\File not found");
  return;
  //handleFileRead(server.uri())
  
}
void handleInfo() {
  
  String path = "/info.html";
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    Serial.println("DONE1");
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    
    file.close();
    Serial.println("DONE2");
    return;
  }
}

void handleData(){
  SerialData serialdata;
  String data_buff;
  int START_FLAG = 0;
  int timeout = 0;
  Serial.println("handle data");
  //Request data
  Serial1.print("[Q1]");
  while (timeout < 15){
    Serial.print("TIMEOUT:");
    Serial.println(timeout);
    if(Serial1.available() > 0 and START_FLAG < 2){
      char msg = Serial1.read();
      Serial.println(msg);
      
      if(msg == '[' or START_FLAG == 1){
        data_buff += msg;
        //Serial.println(data_buff);
        START_FLAG = 1;
        timeout = 0;
        if (msg == ']'){
          data_buff += msg;
          START_FLAG = 2;
          timeout = 15;
          Serial.println("END");
        }
      }else{
        data_buff = "";
        timeout = timeout + 1 ;
        delay(4000);
        Serial.println("NO DATA");
        Serial1.print("[Q1]");
      }
    }else{
        data_buff = "";
        timeout = timeout + 1 ;
        delay(4000);
        Serial.println("NO DATA");
        Serial1.print("[Q1]");
    }
  }
  Serial.println("FINISHEDDDD");
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
void handleSetup() {
  String msg;
  String StartBit = "[";
  String StopBit = "]";
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
        StaticJsonBuffer<800> jBuffer;
        JsonObject& jObject = jBuffer.parseObject(data);
        if (jObject["topic"] == "set_heading"){
          int heading = jObject["data"]["heading"];
          Serial.println(heading);
          int bowofs = jObject["data"]["bow"];
          Serial.println(bowofs);
          KBit = "2";
          Serial1.print(StartBit);
          Serial1.print(KBit);
          Serial1.printf("%04d", heading);
          Serial1.printf("%04d", bowofs);
          Serial1.print(StopBit);
        }else if (jObject["topic"] == "sat_select"){
          String sat_id = jObject["sat_id"];
          if (sat_id == "1"){
            String sat_info = readFile2(SPIFFS, "/profile/thaicom4.cfg");
            Serial.println(sat_info);
            server.send(200,"application/json",sat_info);
          }else if (sat_id == "2"){
            String sat_info = readFile2(SPIFFS, "/profile/thaicom5.cfg");
            Serial.println(sat_info);
            server.send(200,"application/json",sat_info);
          }else if (sat_id == "3"){
            String sat_info = readFile2(SPIFFS, "/profile/thaicom6.cfg");
            Serial.println(sat_info);
            server.send(200,"application/json",sat_info);
          }
        }else if(jObject["topic"] == "set_satellite"){
            writeFile(SPIFFS, "/conf/satellite.conf",jObject["data"]);
            String value2 = jObject["data"]["satellite_name"];
            Serial.println("value2");
            readFile(SPIFFS, "/conf/satellite.conf");
        }else if(jObject["topic"] == "set_angle"){
          int az = jObject["data"]["az"];
          int el = jObject["data"]["el"];
          int pol = jObject["data"]["pol"];
          KBit = "4";
          Serial1.print(StartBit);
          Serial1.print(KBit);
          Serial1.printf("%04d", az);
          Serial1.printf("%04d", el);
          Serial1.printf("%04d", pol);
          Serial1.print(StopBit);
        }else{
            Serial.println("faileddddd");
        }
        
      }
      Serial.println("web completed1");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      String path = "/setup.html";
      String contentType = getContentType(path);
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
        String contentType = getContentType(path);
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
    String contentType = getContentType(path);
    if (SPIFFS.exists(path)) {
      File file = SPIFFS.open(path, "r");
      size_t sent = server.streamFile(file, contentType);
      file.close();
      return;
    }
    Serial.println("\File not found");
    return;
  
}


void GetData(){
  /*
  String data_buff;
  //Serial.print("DATA:");
  if (Serial1.available() > 0){
    char msg = Serial1.read();
    Serial.print(msg);
  }else{
  
  }
  */
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if ( cookie == "ESPSESSIONID=1") {
        Serial.println("Logged in Successful123");
        String sat_info = readFile2(SPIFFS, "/conf/satellite.conf");
        if(sat_info != NULL){
          server.send(200,"application/json",sat_info);
        }
    }
  }
}

void handleDataSetup(){
  
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if ( cookie == "ESPSESSIONID=1") {
        Serial.println("Logged in Successful1234");
        String sat_info = readFile2(SPIFFS, "/conf/satellite.conf");
        if(sat_info != NULL){
          server.send(200,"application/json",sat_info);
        }
    }
  /*  
    String path = "/setup.html";
    String contentType = getContentType(path);
    if (SPIFFS.exists(path)) {
      File file = SPIFFS.open(path, "r");
      size_t sent = server.streamFile(file, contentType);
      file.close();
      return;
    }
  }
  
  Serial.println("CHECK DATA");
  String sat_info = readFile2(SPIFFS, "/conf/satellite.conf");
  if(sat_info != NULL){
    server.send(200,"application/json",sat_info);
  }
  */
  }
}
//root page can be accessed only if authentification is ok
void handleRoot() {
  Serial.println("Enter handleRoot");
  String header;
  if (!is_authentified()) {
    server.sendHeader("Location", "/info.html");
    server.sendHeader("Cache-Control", "no-cache");
    //server.send(301);
    return;
  }
}

void handleNotFound() {
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

String getContentType(String filename) {
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


void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
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
        listDir(fs, file.name(), levels - 1);
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

void readFile(fs::FS &fs, const char * path) {
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

String readFile2(fs::FS &fs, const char * path) {
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

void writeFile(fs::FS &fs, const char * path, String message) {
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

bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "antenna_info.html";
  String contentType = getContentType(path);
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

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  //Serial1.print("COM5 555555");
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
  listDir(SPIFFS, "/", 0);
  Serial.println( "Test complete" );

  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "404: NOT FOUND");
  });
  server.on("/requestdata",handleData);
  server.on("/requestdatasetup",handleDataSetup);
  server.on("/", handleRoot);
  server.on("/info.html", handleInfo);
  server.on("/setup.html", handleSetup);
  server.on("/login", handleLogin);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works without need of authentification");
  });
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
