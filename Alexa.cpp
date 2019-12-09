
/*
 * Alexa.cpp
 *
 *  Created on: 12 lis 2019
 *      Author: jant
 *      Majority of code is from https://github.com/torinnguyen/ESP8266Wemo
 */
#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "../Switcher/Debug.hpp"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <cstdio>
#include <pgmspace.h>
#include <Relay.h>
#include "../Switcher/Switcher.h"


extern ESP8266WebServer server;
String friendlyName = (HOSTNAME);           // Alexa and/or Home Assistant will use this name to identify your device
const char* serialNumber = ("221517K0101768");                  // anything will do
const char* uuid = "904bfa3c-1de2-11v2-8728-4c11ae0D20fc";
//		"//fd8eebaf492d";
//+WiFi.macAddress().c_str();;
///unsigned int len = WiFi.macAddress().length();
//const char* mac[len+1] ;
//const char*	uuidmac ="112233445566";
// WiFi.macAddress().toCharArray(uuidmac,6);
// uint8_t uuidmac[6];
// char macStr[18] = { 0 };
 //wifi_get_macaddr(STATION_IF, uuidmac);
// wifi_get_macaddr(STATION_IF, mac);
// uid += uuid2;
//		WiFi.macAddress().toCharArray(mac,len);
//fd8eebaf492d");    // anything will do

// Deklaracje multiemisji do wykrywania
IPAddress ipMulti(239, 255, 255, 250);
const unsigned int portMulti = 1900;

// TCP port to listen on
extern const int port;
//const unsigned int webserverPort = port; //49153;

/*
extern const int LED_PIN;
extern const int RELAY_PIN;
extern const int BUTTON_PIN;
*/

extern Relay r;
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

//int status = WL_IDLE_STATUS;

WiFiUDP Udp;
byte packetBuffer[ UDP_TX_PACKET_MAX_SIZE]; //bufor do przechowywania pakietów przychodzących i wychodzących
//Start TCP server
//ESP8266WebServer server(webserverPort);


//-----------------------------------------------------------------------
// UDP Multicast Server
//-----------------------------------------------------------------------
//UDP Server
void serverUdp(){
Udp.beginMulticast(WiFi.localIP(),  ipMulti, portMulti);

DEBUG(F("Udp multicast server started at "));
DEBUGVAL(ipMulti);
DEBUG(":");
DEBUGVAL(portMulti);

}


char* getDateString()
{
  //Nie ma znaczenia, która data i godzina będą działać
  //Opcjonalnie: zamień na implementację klienta NTP
  return (char*)F("Wed, 29 Jun 2016 00:13:46 GMT");
}

void responseToSearchUdp(IPAddress& senderIP, unsigned int senderPort)
{

 DEBUG(F("responseToSearchUdp"));

  //Jest to absolutnie konieczne, ponieważ Udp.write nie obsługuje poprawnie adresu IP ani liczb, takich jak Serial.print
  IPAddress myIP = WiFi.localIP();
  char ipChar[20];
//  snprintf(ipChar, 20, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  sprintf(ipChar, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  char portChar[7];
//  snprintf(portChar, 7, ":%d", webserverPort);
  sprintf(portChar,":%d", port);
  Udp.beginPacket(senderIP, senderPort);
  Udp.write(PSTR("HTTP/1.1 200 OK\r\nCACHE-CONTROL: max-age=86400\r\nDATE: "));
  Udp.write(getDateString());
  Udp.write(PSTR("\r\nEXT:\r\nLOCATION: http://"));
  Udp.write(ipChar);
  Udp.write(portChar);
  Udp.write(PSTR("/setup.xml\r\nOPT: \"http://schemas.upnp.org/upnp/1/0/\"); ns=01\r\n01-NLS: "));
  Udp.write(uuid);
  Udp.write(PSTR("\r\nSERVER: Unspecified, UPnP/1.0, Unspecified\r\nX-User-Agent: redsonic\r\nST:"));
  Udp.write(PSTR("upnp:rootdevice\r\nUSN: uuid:Socket-1_0-"));
  Udp.write(serialNumber);
  Udp.write(PSTR("upnp:rootdevice\r\n\r\n"));
  Udp.endPacket();
}

void UdpMulticastServerLoop()
{
  int numBytes = Udp.parsePacket();
  if (numBytes <= 0)
    return;

  IPAddress senderIP = Udp.remoteIP();
  unsigned int senderPort = Udp.remotePort();

  // read the packet into the buffer
  Udp.read(packetBuffer, numBytes);

  // print out the received packet
  //Serial.write(packetBuffer, numBytes);

  // sprawdź, czy jest to M-SEARCH dla urządzenia WeMo
  String request = String((char *)packetBuffer);
  int mSearchIndex = request.indexOf("M-SEARCH");
  if (mSearchIndex >= 0)
    //return;
    responseToSearchUdp(senderIP, senderPort);
}


//-----------------------------------------------------------------------
// HTTP Server
//-----------------------------------------------------------------------

/*void handleRoot()
{

  DEBUG(F("handleRoot"));

  server.send(200, "text/plain", F("Tell Alexa to discover devices"));
}*/

void handleEventXml()
{

  DEBUG(F("HandleEventXML"));

  String eventservice_xml = F( "<scpd xmlns=\"urn:Belkin:service-1-0\">"
        "<actionList>"
          "<action>"
            "<name>SetBinaryState</name>"
            "<argumentList>"
              "<argument>"
                "<retval/>"
                "<name>BinaryState</name>"
                "<relatedStateVariable>BinaryState</relatedStateVariable>"
                "<direction>in</direction>"
                "</argument>"
            "</argumentList>"
          "</action>"
          "<action>"
            "<name>GetBinaryState</name>"
            "<argumentList>"
              "<argument>"
                "<retval/>"
                "<name>BinaryState</name>"
                "<relatedStateVariable>BinaryState</relatedStateVariable>"
                "<direction>out</direction>"
                "</argument>"
            "</argumentList>"
          "</action>"
      "</actionList>"
        "<serviceStateTable>"
          "<stateVariable sendEvents=\"yes\">"
            "<name>BinaryState</name>"
            "<dataType>Boolean</dataType>"
            "<defaultValue>0</defaultValue>"
           "</stateVariable>"
           "<stateVariable sendEvents=\"yes\">"
              "<name>level</name>"
              "<dataType>string</dataType>"
              "<defaultValue>0</defaultValue>"
           "</stateVariable>"
        "</serviceStateTable>"
        "</scpd>\r\n"
        "\r\n");

  String header = F("HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: ");
  header += eventservice_xml.length();
  header +=F( "\r\nDate: ");
  header += getDateString();
  header += F("\r\nLAST-MODIFIED: Sat, 01 Jan 2000 00:00:00 GMT\r\n");
  header += F("SERVER: Unspecified, UPnP/1.0, Unspecified\r\n");
  header += F("X-User-Agent: redsonic\r\nconnection: close\r\n\r\n");
  header += eventservice_xml;

  DEBUGVAL(header);
  server.sendContent(header);
}

void handleSetupXml()
{
     DEBUG(F("handleSetupXml"));

  String body = F( "<?xml version=\"1.0\"?>\r\n"
  "<root xmlns=\"urn:Belkin:device-1-0\">\r\n"
  "<specVersion>\r\n"
    "<major>1</major>\r\n"
    "<minor>0</minor>\r\n"
        "</specVersion>\r\n"
        "<device>\r\n"
          "<deviceType>urn:Belkin:device:controllee:1</deviceType>\r\n"
          "<friendlyName>");
  body += friendlyName;
  body += F( "</friendlyName>\r\n"
              "<manufacturer>Belkin International Inc.</manufacturer>\r\n"
              "<manufacturerURL>http://www.belkin.com</manufacturerURL>\r\n"
              "<modelDescription>Belkin Plugin Socket 1.0</modelDescription>\r\n"
              "<modelName>Socket</modelName>\r\n"
              "<modelNumber>1.0</modelNumber>\r\n"
              "<UDN>uuid:Socket-1_0-");
  body += uuid;
  body += F("</UDN>\r\n"
              "<modelURL>http://www.belkin.com/plugin/</modelURL>\r\n"
            "<serialNumber>");
  body += serialNumber;
  body += F("</serialNumber>\r\n"
            "<serviceList>\r\n"
              "<service>\r\n"
                "<serviceType>urn:Belkin:service:basicevent:1</serviceType>\r\n"
                "<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>\r\n"
                "<controlURL>/upnp/control/basicevent1</controlURL>\r\n"
                "<eventSubURL>/upnp/event/basicevent1</eventSubURL>\r\n"
                "<SCPDURL>/eventservice.xml</SCPDURL>\r\n"
              "</service>\r\n"
            "</serviceList>\r\n"
          "</device>\r\n"
        "</root>\r\n"
        "\r\n");

  String header = F("HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: ");
  header += body.length();
  header += F("\r\nDate: ");
  header += getDateString();
  header += F("\r\nLAST-MODIFIED: Sat, 01 Jan 2000 00:00:00 GMT\r\n");
  header += F("SERVER: Unspecified, UPnP/1.0, Unspecified\r\n");
  header += F("X-User-Agent: redsonic\r\n");
  header += F("connection: close\r\n\r\n");
  header += body;
 DEBUGVAL(header);
  server.sendContent(header);
}

void handleUpnpControl()
{
 DEBUG(F("handleUpnpControl"));

  //Extract raw body
  //Ponieważ przed „1.0” występuje „=”, daje to następujące informacje:
  //"1.0" encoding="utf-8"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:SetBinaryState xmlns:u="urn:Belkin:service:basicevent:1"><BinaryState>1</BinaryState></u:SetBinaryState></s:Body></s:Envelope>
  String body = server.arg(0);

  //Sprawdź poprawne żądanie
  boolean isOn = body.indexOf("<BinaryState>1</BinaryState>") >= 0;
  boolean isOff = body.indexOf("<BinaryState>0</BinaryState>") >= 0;
  boolean isQuestion = body.indexOf("GetBinaryState") >= 0;

  DEBUG(F("body:"));
  DEBUGVAL(body);

  boolean isValid = isOn || isOff || isQuestion;
  if (!isValid) {
  DEBUG(F("Bad request from Amazon Echo"));
    //Serial.println(body);

    server.send(400, "text/plain", F("Bad request from Amazon Echo"));
    return;
  }

  //On/Off Logic
  if (isOn) {
	  r.setOn();
 //    digitalWrite(LED_PIN, 0);
 //   digitalWrite(RELAY_PIN, 1);
#ifdef DEBUG
      Serial.println(F("Alexa is asking to turn ON a device"));
#endif
      String body = F(
      "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>\r\n"
      "<u:SetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">\r\n"
      "<BinaryState>1</BinaryState>\r\n"
      "</u:SetBinaryStateResponse>\r\n"
      "</s:Body> </s:Envelope>");
      String header = F("HTTP/1.1 200 OK\r\nContent-Length: ");
      header += body.length();
      header += F("\r\nContent-Type: text/xml\r\nDate: ");
      header += getDateString();
      header += F("\r\nEXT:\r\n");
      header += F("SERVER: Linux/2.6.21, UPnP/1.0, Portable SDK for UPnP devices/1.6.18\r\n");
      header += F("X-User-Agent: redsonic\r\n\r\n");
      header += body;
      server.sendContent(header);
  }
  else if (isOff) {
	  r.setOff();
 //     digitalWrite(LED_PIN, 1);
 //     digitalWrite(RELAY_PIN, 0);
#ifdef DEBUG
      Serial.println(F("Alexa is asking to turn OFF a device"));
#endif
      String body = F(
      "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>\r\n"
      "<u:SetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">\r\n"
      "<BinaryState>0</BinaryState>\r\n"
      "</u:SetBinaryStateResponse>\r\n"
      "</s:Body> </s:Envelope>");
      String header = F("HTTP/1.1 200 OK\r\nContent-Length: ");
      header += body.length();
      header += F("\r\nContent-Type: text/xml\r\nDate: ");
      header += getDateString();
      header += F("\r\nEXT:\r\n");
      header += F("SERVER: Linux/2.6.21, UPnP/1.0, Portable SDK for UPnP devices/1.6.18\r\n");
      header += F("X-User-Agent: redsonic\r\n\r\n");
      header += body;
      server.sendContent(header);
  }

  else if (isQuestion) {
      String body = F(
	  "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>\r\n"
      "<u:GetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">\r\n"
      "<BinaryState>");
//      body += String(digitalRead(12));
      body += String(r.read());
//    body += r.isOn() ? "1" : "0";
      body += F("</BinaryState>\r\n"
      "</u:GetBinaryStateResponse>\r\n"
      "</s:Body> </s:Envelope>");
      String header = F("HTTP/1.1 200 OK\r\nContent-Length: ");
      header += body.length();
      header += F("\r\nContent-Type: text/xml\r\nDate: ");
      header += getDateString();
      header += F("\r\nEXT:\r\n");
      header += F("SERVER: Linux/2.6.21, UPnP/1.0, Portable SDK for UPnP devices/1.6.18\r\n");
      header += F("X-User-Agent: redsonic\r\n\r\n");
      header += body;
      server.sendContent(header);
  }
}

/*void handleNotFound()
{
  DEBUG(F("handleNotFound()"));

  String message = F("File Not Found\n\nURI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? F("GET") : F("POST");
  message += F("\nArguments: ");
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}*/

//Web Server
void serverAlexa(){
//server.on("/", handleRoot);
server.on("/setup.xml", handleSetupXml);
server.on("/eventservice.xml", handleEventXml);
server.on("/upnp/control/basicevent1", handleUpnpControl);
//server.onNotFound(handleNotFound);
//server.begin();
//  DEBUG(PSTR("HTTP server started on port "));
//  DEBUGVAL(webserverPort);

}






