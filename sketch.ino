#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


// ########### EINSTELLUNGEN #################
// WLAN Einstellungen
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Hombebridge Webhooks Einstellungen
String hbip = "192.168.178.51";
String hbport = "51828";
String hbid = "alarmanlage";

//Messintervall in Sekunden
int wait = 5;
// ###########################################


// Pin D1
int pinAlarmanlageStatus = D1;

// Pin D2
int pinAlarmanlageAusgeloest = D2;

// Alarmanlagen Status
String securitystatecurrent = "3"; // Default: Disarm
String securitystatetarget = "3"; // Default: Disarm

// Error Handling
String wlanstatus = "";
bool wlanerror = false;
String securitystatus = "";
bool securityerror = false;
String httprequeststatus = "";
bool httprequesterror = false;

// Webserver
WiFiServer server(80);

// Sender
HTTPClient sender;


void connectWifi() {
  
  WiFi.persistent(false);   // daten nicht in Flash speichern
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  // Verbindungsversuch max 5 Sekunden
  for (int x = 0; x < 10; x++) {
    delay(500);
  }

  // Prüfe Verbindung
  if (WiFi.status() != WL_CONNECTED)
  {
    // no connection
    wlanstatus = "Nicht Verbunden";
    wlanerror = true;
  }
  else
  {
    // connected
    wlanstatus = "Verbunden";
    wlanerror = false;
  }

}


void checkState(){

  //Hier wird der Wert an die Smarthome-Umgebung übertragen

  if (digitalRead(pinAlarmanlageStatus) == LOW)
    {
      // Open - Aus - Disarmed
      securitystatetarget = "3";
    }
  else
    {
       // Closed - An - Away
       securitystatetarget = "1";
    }

  if (digitalRead(pinAlarmanlageAusgeloest) == LOW)
    {
       // Open - Normal
    }
  else
    {
       // Closed - Ausgeloest - Triggered
       securitystatetarget = "4";
    }

  // Wenn Alarm ausgelöst
  if (securitystatetarget == "4")
    {
      // Send Current
      sendRequest("http://" + hbip + ":" + hbport + "/?accessoryId=" + hbid + "&currentstate=" + securitystatetarget);
    }
  else
    {
      // Send Target
      sendRequest("http://" + hbip + ":" + hbport + "/?accessoryId=" + hbid + "&targetstate=" + securitystatetarget);

      delay(1000);
      securitystatecurrent = securitystatetarget;
      
      // Send Current
      sendRequest("http://" + hbip + ":" + hbport + "/?accessoryId=" + hbid + "&currentstate=" + securitystatecurrent);
    }

  securitystatus = "Erfolgreich";

}

void sendRequest(String url) {

    if (sender.begin(url)){

    // HTTP-Code der Response speichern
    int httpCode = sender.GET();
   

    if (httpCode > 0) {
      
      // Anfrage wurde gesendet und Server hat geantwortet
      // Info: Der HTTP-Code für 'OK' ist 200
      if (httpCode == HTTP_CODE_OK) {

        // Hier wurden die Daten vom Server empfangen

        // String vom Webseiteninhalt speichern
        String payload = sender.getString();

        httprequeststatus = "Erfolgreich";  
        httprequesterror = false;
      }
      
    }else{
      // Falls HTTP-Error
      httprequeststatus = "HTTP-Error: ", sender.errorToString(httpCode).c_str();
      httprequesterror = true;
    }

    // Wenn alles abgeschlossen ist, wird die Verbindung wieder beendet
    sender.end();
    
  }else {
    httprequeststatus = "HTTP-Verbindung konnte nicht hergestellt werden!";
    httprequesterror = true;
  }
  
}



void setup() {

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Pin
  pinMode(pinAlarmanlageStatus, INPUT);
  pinMode(pinAlarmanlageAusgeloest, INPUT);
  
  // Connect to WiFi network
  if (WiFi.status() != WL_CONNECTED) connectWifi();

  // Start the server
  server.begin();

  // Set wait to sec
  wait = wait * 1000;
}

void loop() {

  // Error Handling LED
  if (wlanerror == false & securityerror == false & httprequesterror == false)
  {
    // Alles ok
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
    delay(200);                       // wait 
    digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
  }
  else if (wlanerror == true)
  {
    // Doppelblinken bei WLAN Problem
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
    delay(200);                       // wait 
    digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
    delay(100);                       // wait 
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
    delay(200);                       // wait 
    digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
  }
  else if (httprequesterror == true)
  {
    // Dreifachblinken bei HttpRequest Problem
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
    delay(200);                       // wait 
    digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
    delay(100);                       // wait 
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
    delay(200);                       // wait 
    digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
    delay(100);                       // wait 
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
    delay(200);                       // wait 
    digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
  }
  else
  {
    // Kein Blinken bei allen anderen Problemen
  }

  // Check WiFi network
  if (WiFi.status() != WL_CONNECTED)
  {
    // no connection
    wlanstatus = "Nicht Verbunden";
    wlanerror = true;
    connectWifi();
  }
  else
  {
    // connected
    wlanstatus = "Verbunden";
    wlanerror = false;
  }

  // Check Pin state and send request
  checkState();

  // Web UI
  WiFiClient client = server.available();
  if (client) {

    boolean blank_line = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
 
        if (c == '\n' && blank_line) {

          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();

          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head><meta http-equiv=refresh content=15></head><body bgcolor=white>");

          client.println("<h1>Status Alarmanlage</h1><h3>Hardware: Wemos D1 Mini</h3>");
          
          client.println("<div style=\"background:#d9d9d9;padding:10px;\"><b>STATUS LED</b><br>");
          client.println("- 1-fach Blinken &rArr; Alles ok<br>");
          client.println("- 2-fach Blinken &rArr; WLAN-Verbindungsproblem<br>");
          client.println("- 3-fach Blinken &rArr; HttpRequest Problem<br>");
          client.println("- Kein Blinken &rArr; Sonstiges Problem<br>");
          client.println("</div>");
          
          client.println("<br>");

          client.println("<div style=\"background:#d9d9d9;padding:10px;\"><b>WLAN</b><br>");
          client.println("Status &rArr; " + wlanstatus + "<br>");
          client.println("</br>");
          client.println("SSID &rArr; " + String(ssid) + "<br>");
          client.println("</div>");
          
          client.println("<br>");

          client.println("<div style=\"background:#d9d9d9;padding:10px;\"><b>INPUT - PIN</b><br>");
          client.println("Status &rArr; " + securitystatus + "<br>");
          client.println("</br>");
          client.println("Messintervall &rArr; " + String(wait / 1000) + " sec<br>");
          client.println("Alarmanlage Status (D1) &rArr; ");
          if (digitalRead(pinAlarmanlageStatus) == LOW)
            {
              client.println("Open"); 
            }
          else
            {
              client.println("Closed"); 
            }
          client.println("</br>");
          
          client.println("Alarmanlage Ausgeloest (D2) &rArr; "); 
          if (digitalRead(pinAlarmanlageAusgeloest) == LOW)
            {
              client.println("Open"); 
            }
          else
            {
              client.println("Closed"); 
            }
          client.println("</br>");
          client.println("Alarmanlage Status &rArr; " + securitystatecurrent + " (Stay=0 / Away=1 / Night=2 / Disarmed=3 / Triggered=4)<br>");  
          client.println("</div>");
          
          client.println("</br>");

          client.println("<div style=\"background:#d9d9d9;padding:10px;\"><b>OUTPUT - HTTP REQUEST</b><br>");
          client.println("Status &rArr; " + httprequeststatus + "<br>");
          client.println("</br>");
          client.println("Request Webhooks &rArr; http://" + hbip + ":" + hbport + "/?accessoryId=" + hbid + "&amp;targetstate=" + securitystatetarget + "<br>");
          client.println("Request Webhooks &rArr; http://" + hbip + ":" + hbport + "/?accessoryId=" + hbid + "&amp;currentstate=" + securitystatecurrent + "<br>"); 
          client.println("</div>");
          
          client.println("<br><h3>Sketch: Von Jan mit Liebe gemacht</h3>"); 

          client.println("</body></html>"); 
          break;
        }
        if (c == '\n') {

          blank_line = true;
        }
        else if (c != '\r') {

          blank_line = false;
        }
      }
    } 

    delay(1);
    client.stop();

  }
  
  delay(wait);
}
