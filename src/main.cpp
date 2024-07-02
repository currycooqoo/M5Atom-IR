/*
   M5StickC-IR v0.0.2 を元に利用した
   https://github.com/linclip/M5StickC-IR
   ベースとなっているminIRumも利用して受信もできるようにした
*/

/* 
   M5Stack ATOMS3を購入 その2 | Lang-ship
   https://lang-ship.com/blog/work/m5stack-atoms3-2/#toc2
   USB Serialについて
 */

// 無印Atom
#define S3USBSerial Serial

// Atom S3
// #define S3USBSerial USBSerial

// Atom S3
// #if ARDUINO_USB_CDC_ON_BOOT
//   #define S3USBSerial Serial
// #else
//   #if ARDUINO_USB_MODE
//     #define S3USBSerial USBSerial
//   #else
//     #error "Please, board settings -> USB CDC On Boot=Enabled"
//   #endif
// #endif

#include <M5Unified.h>
#include <FastLED.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiManager.h> // https://github.com/Brunez3BD/WIFIMANAGER-ESP32
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <aJSON.h>       // https://github.com/interactive-matter/aJson  https://gitlab.com/xarduino/lightsw/blob/master/patch/ajson-void-flush.patch
#include <IRremoteESP8266.h> // IRremoteESP8266
#include <IRsend.h> // IRremoteESP8266
#include <IRrecv.h> // IRremoteESP8266

byte CENTER_BTN_PIN = 39; // Atomは39、Atom S3は41
#define NUM_LEDS 1 // RGB LEDの数 (M5Atom Matrixなら25)
#define LED_DATA_PIN 27 // Atomは27、Atom S3は35

byte SEND_PIN = 23; // Atom内蔵IRは12, 自作は23
byte RECV_PIN = 33; // Atom 自作は33
// byte SEND_PIN = 7; // Atom S3内蔵IRは4, 自作は7
// byte RECV_PIN = 8; // Atom S3 自作は8
#define TIMEOUT kMaxTimeoutMs
#define CAPTURE_BUFFER_SIZE 1024
IRsend irsend(SEND_PIN);
IRrecv irrecv(RECV_PIN, CAPTURE_BUFFER_SIZE, TIMEOUT, true);

WebServer webServer(80);
String localName = "m5atom-";
String histIRcode;

CRGB leds[NUM_LEDS];

String dumpIRcode (decode_results *results) {
  String s = "";
  for (int i = 1;  i < results->rawlen;  i++) {
    s += results->rawbuf[i] * RAWTICK;
    if ( i < results->rawlen - 1 ) s += ",";
  }
  return s;
}

void handleIndex() {
  S3USBSerial.println("handleIndex");
  String s = "<html lang=\"en\"><head><meta charset=\"utf-8\"/><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>M5Atom-IR</title><link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/3.3.7/css/bootstrap.min.css\" /><script src=\"https://cdnjs.cloudflare.com/ajax/libs/jquery/3.2.1/jquery.min.js\"></script><script src=\"https://cdnjs.cloudflare.com/ajax/libs/twitter-bootstrap/3.3.7/js/bootstrap.min.js\"></script></head><body><div class=\"container\"><div class=\"row\"><div class=\"col-md-12\"><h1>M5Atom-IR <small>";
  s += localName;

  s += ".local</small></h1><p>IP address: ";
  s += String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
  s += "</p><hr><div class=\"form-group\">";
  s += "<textarea class=\"form-control\" id=\"message\" rows=\"10\"></textarea></div>";
  s += "<div class=\"row\"> ";
  s += "<button class=\"btn btn-primary\" id=\"get\">GET</button> ";
  s += "<button class=\"btn btn-success\" id=\"post\">POST</button> ";
  s += "<button class=\"btn btn-danger\" id=\"LEDON\">LED ON</button> ";
  s += "<button class=\"btn btn-info\" id=\"LEDOFF\">LED OFF</button>";
  // 無線リセット
  s += " <button class=\"btn\" id=\"WIFIRESET\">WiFi reset</button> </div>";

  s += "</div>";
  s += "<script>var xhr = new XMLHttpRequest();var textarea = document.getElementById(\"message\");";
  
  // IRコード受信
  s += "document.getElementById(\"get\").addEventListener(\"click\", function () {xhr.open('GET', '/messages', true);xhr.setRequestHeader('X-Requested-With', 'curl');xhr.onreadystatechange = function() {if(xhr.readyState == 4) {textarea.value =xhr.responseText;}};xhr.send(null);});";

  // IRコード送信
  s += "document.getElementById(\"post\").addEventListener(\"click\", function () {data = textarea.value;xhr.open('POST', '/messages', true);xhr.onreadystatechange = function() {if(xhr.readyState == 4) {alert(xhr.responseText);}};xhr.setRequestHeader('Content-Type', 'application/json');xhr.setRequestHeader('X-Requested-With', 'curl');xhr.send(data);});";

  // LED ON
  s += "document.getElementById(\"LEDON\").addEventListener(\"click\", function () {xhr.open('get', '/ledon', true);xhr.setRequestHeader('X-Requested-With', 'curl');xhr.onreadystatechange = function() {if(xhr.readyState == 4) {textarea.value =xhr.responseText;}};xhr.send(null);});";
  // LED OFF
  s += "document.getElementById(\"LEDOFF\").addEventListener(\"click\", function () {xhr.open('get', '/ledoff', true);xhr.setRequestHeader('X-Requested-With', 'curl');xhr.onreadystatechange = function() {if(xhr.readyState == 4) {textarea.value =xhr.responseText;}};xhr.send(null);});";
  // OnDemandAp Wifi設定 wifi reset
  s += "document.getElementById(\"WIFIRESET\").addEventListener(\"click\", function () {var result=confirm('Reset WiFi settings?');if(result){xhr.open('get', '/wifireset', true);xhr.setRequestHeader('X-Requested-With', 'curl');xhr.onreadystatechange = function() {if(xhr.readyState == 4) {textarea.value =xhr.responseText;}};xhr.send(null);}});";

  s += "</script></body></html>";
  webServer.send(200, "text/html", s);
}

void handleNotFound() {
  S3USBSerial.println(webServer.uri().c_str());
  S3USBSerial.println("404");
  webServer.send(404, "text/plain", "Not found.");
}

void handleMessages() {
  S3USBSerial.println(webServer.uri().c_str()); // add

  if (webServer.method() == HTTP_POST) {
    // irrecv.disableIRIn(); //
    String req = webServer.arg(0);
    char json[req.length() + 1];
    req.toCharArray(json, req.length() + 1);
    S3USBSerial.println(json); // 受信データ確認
    aJsonObject* root = aJson.parse(json);
    if (root != NULL) {
      aJsonObject* freq = aJson.getObjectItem(root, "freq");
      aJsonObject* data = aJson.getObjectItem(root, "data");
      if (freq != NULL && data != NULL) {
        const uint16_t d_size = aJson.getArraySize(data);
        uint16_t rawData[d_size]; // 型変更
        // unsigned int rawData[d_size]; // 型変更
        for (int i = 0; i < d_size; i++) {
          aJsonObject* d_int = aJson.getArrayItem(data, i);
          rawData[i] = d_int->valueint;
        }
        leds[0] = CRGB::Green;
        FastLED.setBrightness(20);
        FastLED.show();
        delay(200);
        irsend.sendRaw(rawData, d_size, (uint16_t)freq->valueint);
        leds[0] = CRGB::Black; // 消灯
        FastLED.setBrightness(0);
        FastLED.show();
        // irrecv.enableIRIn(); //
        req = "";
        aJson.deleteItem(root);
        webServer.sendHeader("Access-Control-Allow-Origin", "*");
        webServer.send(200, "text/plain", "ok");
      } else {
        webServer.send(400, "text/plain", "Invalid JSON format");
      }
    } else {
      webServer.send(400, "text/plain", "Request body is empty");
    }
  }  else if (webServer.method() == HTTP_GET) { // NOTE 追加した
    String s = "{\"format\":\"raw\",\"freq\":38,\"data\":[";
    s += histIRcode;
    s += "]}";
    histIRcode = "";
    webServer.send(200, "text/plain", s);
    S3USBSerial.println(s); // 受信データ確認
  }
}

void handleLedOn() {
  S3USBSerial.println("LED ON");
  leds[0] = CRGB::Blue;
  FastLED.setBrightness(50);
  FastLED.show();
  
  webServer.send(200, "text/plain", "ok");
}

void handleLedOff() {
  S3USBSerial.println("LED OFF");
  leds[0] = CRGB::Black; // 消灯
  FastLED.setBrightness(0);
  FastLED.show();
  
  webServer.send(200, "text/plain", "ok");
}

void handleWiFiReset(); // 宣言

void handleWiFiReset() { // WiFi reset
  S3USBSerial.println("reset WiFi settings");
  webServer.send(200, "text/plain", "reset WiFi settings...");
  WiFi.disconnect(true);   // still not erasing the ssid/pw. Will happily reconnect on next start
  WiFi.begin("0", "0");      // adding this effectively seems to erase the previous stored SSID/PW
  ESP.restart();
  delay(1000);
}

void setup() {  // ---------------------------------------------------------------
  auto cfg = M5.config();
  M5.begin(cfg);

  // 真ん中のボタン
  pinMode(CENTER_BTN_PIN, INPUT_PULLUP);

  // https://docs.m5stack.com/ja/api/m5unified/m5unified_appendix
  switch (M5.getBoard()) {
    case m5::board_t::board_M5AtomS3Lite:
       FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
       break;
    case m5::board_t::board_M5AtomS3:
       FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
       break;
    case m5::board_t::board_M5StampC3:
       FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
       break;
    case m5::board_t::board_M5StampC3U:
       FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
       break;
    case m5::board_t::board_M5Atom:
      FastLED.addLeds<WS2811, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
       break;
    case m5::board_t::board_M5AtomU:
       FastLED.addLeds<WS2811, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
       break;
    default:
       break;
  }

  leds[0] = CRGB::Black; // 消灯
  FastLED.setBrightness(0);
  FastLED.show();
  leds[0] = CRGB::Blue;
  FastLED.setBrightness(50);
  FastLED.show();

  delay(1000);
  S3USBSerial.println("\nM5Stack Atom IR server");

  irsend.begin();
  irrecv.enableIRIn();

  WiFiManager wifiManager;
  delay(1000);

  // Get MAC address for WiFi station
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

  S3USBSerial.print("MAC address   : ");
  S3USBSerial.println(baseMacChr);

  if (digitalRead(CENTER_BTN_PIN) == LOW) { // 起動時RSTボタンが押されてたら
    leds[0] = CRGB::Red;
    FastLED.setBrightness(20);
    FastLED.show();
    delay(200);
    while (digitalRead(CENTER_BTN_PIN) == LOW);
    leds[0] = CRGB::Black; // 消灯
    FastLED.setBrightness(0);
    FastLED.show();
    for (int i = 0; i <= 3; i++) {
      delay(200);
      leds[0] = CRGB::Red;
      FastLED.setBrightness(20);
      FastLED.show();
      
      delay(200);
      leds[0] = CRGB::Black; // 消灯
      FastLED.setBrightness(0);
      FastLED.show();
    }
    leds[0] = CRGB::Red;
    FastLED.setBrightness(20);
    FastLED.show();
    
    // 起動時のボタンでWiFi設定起動 WiFiManager
    if (!wifiManager.startConfigPortal("M5Atom-AP")) {
      S3USBSerial.println("failed to connect and hit timeout");
      delay(3000);
    }
    // reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(2000);

  } else {
    // wifiManager.autoConnect("M5StickC-AP"); //AutoConnectAP
    S3USBSerial.print("Connecting to Wi-Fi");
    WiFi.begin();
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) { // 接続まで最大10秒待つ
      if (i > 20) {
        break;
      }
      delay(500);
      i++;
      S3USBSerial.print(".");
    }
    // S3USBSerial.println("ok!");
  }

  IPAddress ipadr = WiFi.localIP();

  if (WiFi.status() == WL_CONNECTED) {
    S3USBSerial.print(" connected!");
    S3USBSerial.print("\r\nAccess point  : ");
    S3USBSerial.print(WiFi.SSID());
  } else {
    S3USBSerial.print(" not connected!!!");
  }
  S3USBSerial.println();

  delay(1000);

  byte mac[6];
  char buf[6];
  WiFi.macAddress(mac);
  sprintf(buf, "%02x%02x%02x", mac[3], mac[4], mac[5]);
  localName.concat(buf);
 
  // mDNSの設定
  if (MDNS.begin(localName)) {
    MDNS.addService("http", "tcp", 80);
    S3USBSerial.println("mDNS responder: Started");
  }

  // ウェブサーバーの設定
  webServer.on("/", handleIndex); // root
  webServer.on("/messages", handleMessages); // messages
  webServer.on("/ledon", handleLedOn); // LED ON
  webServer.on("/ledoff"    , handleLedOff); // LED OFF
  webServer.on("/wifireset", handleWiFiReset); // 設定やり直し
  webServer.on("/favicon.ico", handleNotFound); // faviconなし

  webServer.onNotFound(handleNotFound);
  webServer.begin();
  S3USBSerial.println("Web server    : Started");

  S3USBSerial.print("Hostname      : ");
  S3USBSerial.print(localName);
  S3USBSerial.println(".local");
  S3USBSerial.print("IP address    : ");
  S3USBSerial.println((String)ipadr[0] + "." + (String)ipadr[1] + "." + (String)ipadr[2] + "." + (String)ipadr[3]);
  S3USBSerial.println();

  leds[0] = CRGB::Black; // 消灯
  FastLED.setBrightness(0);
  FastLED.show();
}

void loop() {
  M5.update();

  // HOMEボタン
  // TODO Homeボタンを押したら受信モードに切り替える
  if (digitalRead(CENTER_BTN_PIN) == LOW) {
    S3USBSerial.println("HOME");
    leds[0] = CRGB::Green;
    FastLED.setBrightness(20);
    FastLED.show();
    // irsend.sendRC5(0xC0C, 12);
    delay(200);
    while (digitalRead(CENTER_BTN_PIN) == LOW);
    leds[0] = CRGB::Black; // 消灯
    FastLED.setBrightness(0);
    FastLED.show();
  }

  // クライアントからの要求に応じて処理を行う
  webServer.handleClient();

  decode_results  results;
  if (irrecv.decode(&results)) {
    histIRcode = dumpIRcode(&results);
    irrecv.resume();
  }

}
