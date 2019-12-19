/*
 * Switcher.h
 *
 *  Created on: 5 lis 2019
 *      Author: jant
 */

#ifndef SWITCHER_H_
#define SWITCHER_H_

//if not comment use define
// set comment not use options
#define VERSION "0.6"
//#define LOCATION "Saloon"
// debug program
//#define DEBUG
#ifdef DEBUG
#define DEBUG_OUT Serial.print
#define DEBUG_OUTLN Serial.println
#else
#define DEBUG_OUT
#define DEBUG_OUTLN
#endif
//add WebPage WiFi scan
#define WEBPAGEWIFISCAN
//add WebPage button set switch
#define WEBPAGESWITCH
//set static IP ( no DHCP)
// set IP in file Switcher.ino (lines 50 - 52)
//#define IP_STATIC
//add manual button
#define BUTTON
//title you web page
#define HOSTNAME "Switcher"
//Thermometer
//#define THERMOMETER

#endif /* SWITCHER_H_ */
