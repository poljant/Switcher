
/*
 * WebPagers.cpp
 *
 *  Created on: 6 lis 2019
 *      Author: jant
 */
#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "../Switcher/Debug.hpp"
//#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
//#include <EEPROM.h>
#include "../Switcher/Switcher.h"
#include "Relay.h"
#ifdef ALEXA
#include "../Switcher/Alexa.h"
#endif
extern Relay r;
#ifdef THERMOMETER
//extern void ReadTemp();
extern float temp;
#endif
// login i hasło do sytemu (dla update)
const char* www_login = "admin";
const char* www_pass = "esp8266";
 const char* ssid;
 const char* pass;

const char* xpass;
const char* xssid;
const char* modes[] = {"NULL","STA","AP","STA+AP"};
const char* phymodes[] = { "","B", "G", "N"};
const char* encrypType[] = {"OPEN", "WEP", "WPA", "WPA2", "WPA_WPA2"};

const int port = 80;                 // port serwera www
ESP8266WebServer server(port);
ESP8266HTTPUpdateServer httpUpdate;
#ifdef ALEXA
extern void	 serverAlexa();
#endif

//unsigned int ilM=10;
unsigned long fminutes( unsigned int ile) {
	return (millis()+(1000*60*ile));
}
// funkcja zamieniająca adres IP na string
String IPtoStr(IPAddress IP){
  String result;
  for (int i = 0; i < 4; ++i) {
    result += String(IP[i]);
    if (i < 3)
      result += '.';
  }
  return result;
}

//char* IPtoChar(int ip)
//{
//  char* ip_str = new char[16];
//  sprintf(ip_str, "%d.%d.%d.%d",
//    (ip 	) & 0xFF,
//    (ip >> 8) & 0xFF,
//    (ip >> 16) & 0xFF,
//    (ip >> 24) & 0xFF);
//  return ip_str;
//}
String HTMLHeader() {           //  nagłówek strony
String  h =  F("<!DOCTYPE html>\n"
   "<html>"
   "<head>"
   "<title>");
  h += HOSTNAME;
  h += F("</title>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css\" >"
  "</head>"
  "<body style=\"text-align: center;color: white; background: black;font-size: 1.5em;\">\n");
  return h;
}

String HTMLFooter() {             //  stopka strony www
String  f = F("<p>Jan Trzciński &copy; 2019 V-");
	f +=VERSION;
	f +=("</p></td></tr>"
   "</body>\n"
   "</html>\n");
  return f;
}

String HTMLPage1() {      // pierwsza część strony www
String t = F("<h1>");
		t +=String(HOSTNAME);
		t += F("</h1>");
#ifdef THERMOMETER
 t += F("<p>Temperature: ");
 t += String(temp);
 t += F(" ºC</p>");
#endif
	t += ( (r.isOn()) ? F("<p><a href = \"/relay/0\"><button class=\"btn btn-danger\">Switch is ON</button></a></p>\n") \
	    : F("<p><a href = \"/relay/1\"><button class=\"btn btn-success\">Switch is OFF</button></a></p>\n"));
	t += F("<p><a href = \"/index.html\"><button class=\"btn btn-info\">Reload</button></a></p>");

 return t;
}
#ifdef WEBPAGEWIFISCAN
String HTMLWiFiScan(void){
	String p="";
	String ix="";
	uint8_t n = WiFi.scanNetworks();
  delay(100);
#ifdef POLISH
	if (n == 0) return F("<p>Brak sieci WiFi.</p>");
	p +=F("<div><h3>Skanowanie sieci WiFi</h3></div>");
#else
  if (n == 0) return F("<p>No WiFi networks.</p>");
  p += F("<div><h3>WiFi network scanning.</h3></div>");
#endif
  p += F("<table  align=\"center\" border=\"2\" >"
	"<thead ><tr><th> </th><th style = \"text-align: center;\">SSID</th>");
#ifdef POLISH
	p += F("<th>kanał</th><th style = \"text-align: center;\">MAC</th>"
   "<th>RSSI</th><th>zabezp</th><th>ukryta</th><tr>");
#else
  p +=F("<th>channel</th><th style = \"text-align: center;\">MAC</th>"
      "<th style = \"text-align: center;\">RSSI</th><th>encryption</th><th>hidden</th><tr>");
#endif
	p +=F("</thead><tbody>");
	 for (int i=0; i<n;i++){	//uint8_t
	 p += F("<tr><td>"
	 "<form action=\"/wifiset\" metod=\"post\">"
	 "<labe><input id=\"SSID\" type=\"radio\" name=\"SSID\" value=\"");
	 p += WiFi.SSID(i);
	 p += F("\"></label></td>");
	 p +="<td>"+WiFi.SSID(i)+"</td>";	//nazwa sieci
	 p +="<td>"+String(WiFi.channel(i))+"</td>";	//kanał
	 p +="<td>"+WiFi.BSSIDstr(i)+"</td>";	//MAC adres
	 p +="<td>"+String(WiFi.RSSI(i))+" dBm</td>"; //siła sygnału
	 p +="<td>";
     p +=(encrypType[WiFi.encryptionType(i)]);// szyfrowanie
	 p += F("</td><td>");
#ifdef POLISH
	 p +=((WiFi.isHidden(i)) ? "tak" : "nie");//czy sieć ukryta
#else
   p +=((WiFi.isHidden(i)) ? "yes" : "no");//czy sieć ukryta
#endif
	 p +=F("</td></tr>");
	 } //end for
	 p += F("</tbody></table><div><p></p> </div>");
   return p;
}
String HTMLWiFiScan1(void){
  String p="";
  String ix="";

	 // gdy połączenie z WiFi
	 if(WiFi.status() == WL_CONNECTED){
#ifdef POLISH
		p += F("<p>Jesteś połączony z siecią </p>");
#else
    p += F("<p>Connected with WiFi </p>");
#endif
		p += F("<table align=\"center\" border=\"2\" ><tbody>"
		"<thead ><tr><th style = \"text-align: center;\">SSID</th>");
#ifdef POLISH
		p += F("<th>kanał</th><th style = \"text-align: center;\">MAC</th>");
#else
    p += F("<th>channel</th><th style = \"text-align: center;\">MAC</th>");
#endif
		p += F("<th style = \"text-align: center;\">RSSI</th></thead><tbody>");
		p += "<td>"+WiFi.SSID()+"</td>";
		p += "<td>"+String(WiFi.channel())+"</td>";
		p += "<td>"+WiFi.BSSIDstr()+"</td>";
		p += "<td>"+String(WiFi.RSSI())+" dBm</td></tbody></table>";
		p +="<label>IP: ";
		p += IPtoStr(WiFi.localIP());
		p += F("</label>");
#ifdef POLISH
		p += F("<p>Aby zmienić sieć WiFi.</p>");
#else
    p += F("<p>To change the WiFi network.</p>");
#endif
	 }else{	//gdy brak połączenia z WiFi.
#ifdef POLISH
		 p += F("<p>Brak połączenia z siecią WiFi.</p>");
#else
     p += F("<p>No connection with WiFi.</p>");
#endif
		 p += F("<label>IP AP: ");
		 p += IPtoStr(WiFi.softAPIP());
		 p += F("</label>");
	 }	//end if
#ifdef POLISH
	 p += F("<p>Wybierz sieć powyżej i podaj hasło</p>"
	 "<label for=\"password\">Hasło do WiFi:</label>");
#else
   p += F("<p>Select the WiFi network above and enter the password</p>"
   "<label for=\"password\">WiFi password:</label>");
#endif
	 p += F("<input style=\"color: black;\" type=\"password\" name=\"password\" />") ; //required=\"required\"/>";
#ifdef POLISH
	 p += F("<input style=\"text-align: center;color: black;\" type=\"submit\" value=\"Połącz z WiFi.\"/></form>"
	  "<p><a href = \"/wifiscan\"><button class=\"btn btn-info\">Skanuj ponownie</button></a></p>"
      "<p><a href = \"/indfex.html\"><button class=\"btn btn-info\">Strona główna</button></a></p>");
#else
     p += F("<input style=\"text-align: center;color: black;\" type=\"submit\" value=\"Connect.\"/></form>"
      "<p><a href = \"/wifiscan\"><button class=\"btn btn-info\">Scan again</button></a></p>"
	  "<p><a href = \"/index.html\"><button class=\"btn btn-info\">Home</button></a></p>");
#endif
	return p;
}
String WebPageScan(){
	return (HTMLHeader()+HTMLWiFiScan()+HTMLWiFiScan1()+HTMLFooter());
}
#endif
 String WebPage() {       // połącz wszystkie części strony www
//#ifdef THERMOMETER
//
//		ReadTemp();
//#endif
  return (HTMLHeader()+HTMLPage1()+HTMLFooter());
 }
/* void handleRoot(){
   server->send(200, "text/plain", F("You should tell Alexa to discover devices"));
 }*/
// funkcja ustawia wszystkie strony www
void setservers() {

	httpUpdate.setup(&server, "/update", www_login, www_pass); // umożliwia aktualizację poprzez WiFi

	server.on("/", []() {      // odświerz stronę www


				server.send(200, "text/html", F("You should tell Alexa to discover devices"));
			});

	server.on("/index.html", []() {      // odświerz stronę www


				server.send(200, "text/html", WebPage());
			});

// server.on("/login", [](){
//   if(!server.authenticate(www_login, www_pass))
//     return server.requestAuthentication();
//   server.send(200, "text/html", WebPage());
// });

	server.on("/relay/0", [] ()     //  wyłącz przekaźnik 1
			{
				r.setOff();
				server.send(200, "text/html", WebPage());
			});

	server.on("/relay/1", []()      // załącz przekaźnik 1
			{
				r.setOn();
				server.send(200, "text/html", WebPage());
			});
//  /// WEBPAGEWIFISCAN
#ifdef WEBPAGEWIFISCAN
	server.on("/wifiscan", [] ()
	{
		server.send(200, "text/html", WebPageScan());
	});
//
//ustanawia połączenie z wybraną siecią wifi
	server.on("/wifiset", [] ()
	{
		//pobierz przysłane dane
			String ESSID = server.arg("SSID");
//		extern char pass;
			String Epass = server.arg("password");

			if (((sizeof(ESSID)>=4) && (sizeof(Epass)>=8))) { //pomiń gdy brak nazwy lub hasła
				//konwertuj ze string do char
				//const char*
				ssid = ESSID.c_str();//e
				//const char*
				pass = Epass.c_str();//e

				WiFi.disconnect();//rozłacz obecne połączenie
				WiFi.mode(WIFI_AP_STA);//ustw tryb AP+STA

				WiFi.begin(ssid,pass);//uruchom łączenie z wybraną siecią wifi
				delay(200);

				int it = 20;//ustal maksymalny czas czekania na 10sek.(20x500ms)
				while ((WiFi.status() != WL_CONNECTED and it>0)) { //  czekaj na połączenie z WiFi
//				BEDUG(".");
					delay(500);
					it--;
				}
//			}else{
			if (it<=0) {
				DEBUG( F("Błąd w ESSID lub Epass."));
				DEBUGVAL(WiFi.status());
				DEBUGVAL(ssid);
				DEBUGVAL(pass);
			};
		};
		server.send(200, "text/html", WebPageScan());
	});
#endif
#ifdef ALEXA
	 serverAlexa();
#endif
/////end WEBPAGEWIFISCAN
	server.begin();                // Start serwera www
	DEBUG(F("Server started"));
}




