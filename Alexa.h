/*
 * Alexa.h
 *
 *  Created on: 12 lis 2019
 *      Author: jant
 *   Majority of code is from https://github.com/torinnguyen/ESP8266Wemo
 */

#ifndef ALEXA_H_
#define ALEXA_H_
#include <ESP8266WiFi.h>
char* getDateString();
void responseToSearchUdp(IPAddress& , unsigned int);
void UdpMulticastServerLoop();
//void handleRoot();
void handleEventXml();
void handleSetupXml();
void handleUpnpControl();
//void handleNotFound();
void serverAlexa();
void serverUdp();



#endif /* ALEXA_H_ */
