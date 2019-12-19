
//#define DEBUG_OUT Serial
//#define PRINTSTREAM_FALLBACK
//#include "Debug.hpp"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <functional>
//#include <EEPROM.h>
#include "../Switcher/WebPagers.h"
#include <Relay.h>	//https:/github.com/poljant/relay
#include "../Switcher/Switcher.h"

#include "secrets.h"
#ifdef THERMOMETER
#include <DallasTemperature.h>
#include <OneWire.h>
#define pinOW 4  //D2	// pin z szyną 1-Wire GPIO4
float temp;
OneWire oneWire(pinOW);
DallasTemperature sensors(&oneWire);
#endif
/*
#ifdef USEOTA
#include <ArduinoOTA.h>
#endif
*/

#ifndef WEBPAGEWIFISCAN
#include <WiFiManager.h>
#endif

extern ESP8266WebServer server;

Relay r;
Relay led;

#define BUTTON_PIN  0 //D3	// GPIO0
#ifdef BUTTON
bool button_state = false;
//	void Button(void);
#endif
#define RELAY_PIN 15
#define LED_PIN LED_BUILTIN
//15
//dane dla AP
//const char* ap_ssid = hostname().c_str();   // SSID AP
//const char* ap_pass = "12345678";  // password do AP
int ap_channel = 7; //numer kanału dla AP
String version = VERSION;



/*
const char* myssid = "xxxxxx";
const char* mypass = "xxxxxxxxxx";
*/

IPAddress IPadr(10,110,3,86); //stały IP
//IPAddress IPadr(10,110,0,84); //stały IP Zdzichu
IPAddress netmask(255,255,0,0);
IPAddress gateway(10,110,0,1);
//////////////////////////////

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  };
};
#ifdef THERMOMETER
bool readtemperature;
unsigned long timestart;
unsigned long timeread;

#endif
void WiFiconnect(void) {

	WiFi.mode(WIFI_AP_STA);
#ifdef IP_STATIC

		WiFi.config(IPadr, gateway, netmask);
		WiFi.begin(myssid, mypass);
#else
		WiFi.begin();

#endif

	int i = 0;
	while ((WiFi.status() != WL_CONNECTED) && (i <= 20)) { //  czekaj na połączenie z wifi
		delay(500);
		i += 1;
		Serial.print(".");
	}

	if (i>=0){
	Serial.println("");
	Serial.print(F("WiFi connected"));
	Serial.println(WiFi.localIP());          // IP server
//	Serial.println(WiFi.macAddress());      // MAC address
	}else{
		WiFi.mode(WIFI_AP_STA);
		WiFi.begin();
		Serial.print( F("WiFi not connect : "));
		Serial.println(WiFi.status());
	}

}
///////////////////////////////////////////
String hostname(void) {
	uint32_t chipId = ESP.getChipId();
	char uid[12];
	sprintf_P(uid, PSTR("%s%02x%02x"),HOSTNAME,
	//        (uint16_t) ((chipId >> 16) & 0xff),
			(uint16_t)((chipId >> 8) & 0xff), (uint16_t) chipId & 0xff);

	return String(uid);
}
#ifdef BUTTON
void Button(void) {
	if (!digitalRead(BUTTON_PIN) and !button_state) {
		delay(10);
		if (!digitalRead(BUTTON_PIN)) {
			r.read() ? r.setOff() : r.setOn();
			button_state = true;
		}
	}
	if (button_state and digitalRead(BUTTON_PIN))
		button_state = false;
}
#endif
//
/*
#ifdef USEOTA
void Set_OTA(){
	  ArduinoOTA.setHostname(hostname().c_str());
	 // ArduinoOTA.setPassword("esp8266");

	  ArduinoOTA.onStart([]() {
	   DEBUG("Start");
	  });
	  ArduinoOTA.onEnd([]() {
	    DEBUG("\nEnd");
	  });
	  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
	    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	  });
	  ArduinoOTA.onError([](ota_error_t error) {
	    Serial.printf("Error[%u]: ", error);
	    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
	    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
	    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
	    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
	    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
	  });
	  ArduinoOTA.begin();
//	  DEBUG("OTA ready");
}
#endif
*/
void setup() {

	Serial.begin(115200);
#ifdef BUTTON
	pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif

#ifdef WEBPAGEWIFISCAN
	WiFiconnect();
#else
#include <WiFiManager.h>
  WiFiManager wifiManager;
//   //wifiManager.resetSettings();
#ifdef IP_STATIC
  wifiManager.setSTAStaticIPConfig(IPadr, netmask, gateway);
#endif
  wifiManager.autoConnect(hostname().c_str());
  delay(100);
#endif
  MDNS.begin(hostname(),WiFi.localIP());
/*	DEBUGVAL(hostname());
	DEBUGVAL(String(WiFi.macAddress()));
	DEBUGVAL(hostname());
	DEBUGVAL(WiFi.localIP());*/
	r.begin(RELAY_PIN);
	led.begin(LED_PIN, true);
    setservers();

/*
#ifdef USEOTA
//	 ArduinoOTA.setHostname(hostname().c_str());
	Set_OTA();
#endif
*/
}
;

void loop() {
	server.handleClient();
/*
#ifdef USEOTA
	ArduinoOTA.handle();
#endif
*/

#ifdef BUTTON
	Button();
#endif
	if (WiFi.status() != WL_CONNECTED) {
		WiFi.mode(WIFI_AP_STA); //tryb AP+STATION
		led.setOn(); // włącz LED gdy nie jest połączenie z WiFi
	} else {
		WiFi.mode(WIFI_STA); //tryb STATION
		led.setOff(); // wyłącz LED gdy jest połączenie z WiFi
	}
#ifdef THERMOMETER
	if(!readtemperature and millis()>timestart){
		sensors.requestTemperatures(); //uruchom odczyt czyjników temperatury
		timeread = millis()+750;
		readtemperature = true;
	}
	if(readtemperature and (millis()>timeread)){
		temp = sensors.getTempCByIndex(0); //czytaj temperaturę w ºC
		timestart= millis()+59250;
		readtemperature = false;
	}
#endif

 MDNS.update();
}

