
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <functional>
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

#ifndef WEBPAGEWIFISCAN
#include <WiFiManager.h>
#endif

extern ESP8266WebServer server;
String hostname();

Relay r;
Relay led;
extern unsigned long fminutes(unsigned int);
unsigned long aptime;
unsigned int apminutes = 10;
#define BUTTON_PIN  0 //D3	 GPIO0
#ifdef BUTTON
bool button_state = false;
#endif
#define RELAY_PIN 15
#define LED_PIN LED_BUILTIN


String version = VERSION;



/*
const char* myssid = "xxxxxx";
const char* mypass = "xxxxxxxxxx";
*/

IPAddress IPadr(10,110,3,86); //stały IP
IPAddress netmask(255,255,0,0);
IPAddress gateway(10,110,0,1);
//////////////////////////////

//format bytes
String formatBytes(size_t bytes) {
	if (bytes < 1024) {
		return String(bytes) + "B";
	} else if (bytes < (1024 * 1024)) {
		return String(bytes / 1024.0) + "KB";
	} else if (bytes < (1024 * 1024 * 1024)) {
		return String(bytes / 1024.0 / 1024.0) + "MB";
	} else {
		return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
	};
}
;
#ifdef THERMOMETER
bool readtemperature;
unsigned long timestart;
unsigned long timeread;
#endif
//dane dla AP
const char* ap_ssid = hostname().c_str();   // SSID AP
const char* ap_pass = "12345678";  // password do AP
int ap_channel = 7; //numer kanału dla AP

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
	Serial.print(F("WiFi connected. IP "));
	Serial.println(WiFi.localIP());          // IP server
//	Serial.println(WiFi.macAddress());      // MAC address
	}else{
		WiFi.mode(WIFI_AP_STA);
		WiFi.begin(ap_ssid, ap_pass, ap_channel);
		Serial.print( F("WiFi not connect : "));
		Serial.println(WiFi.status());
	}

}
///////////////////////////////////////////
String hostname(void) {
	uint32_t chipId = ESP.getChipId();
	char uid[12];
	sprintf_P(uid, PSTR("%s%02x%02x"), HOSTNAME,
	//        (uint16_t) ((chipId >> 16) & 0xff),
			(uint16_t) ((chipId >> 8) & 0xff), (uint16_t) chipId & 0xff);

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
	MDNS.begin(hostname(), WiFi.localIP());
	DEBUG_OUTLN(hostname());
	DEBUG_OUTLN(WiFi.localIP());
	r.begin(RELAY_PIN);
	led.begin(LED_PIN, true);
	setservers();
	aptime = fminutes(apminutes);
}

void loop() {
	server.handleClient();
#ifdef BUTTON
	Button();
#endif
	if (WiFi.status() != WL_CONNECTED) {
		WiFi.mode(WIFI_AP_STA); //tryb AP+STATION
		led.setOn(); // włącz LED gdy nie jest połączenie z WiFi
		aptime = fminutes(apminutes);  // licz czas trwania AP po połaczeniu
	} else {
		if (aptime <= millis()){  // gdy minął czas twania AP przełącz na STATION
		WiFi.mode(WIFI_STA); //tryb STATION
		}
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

