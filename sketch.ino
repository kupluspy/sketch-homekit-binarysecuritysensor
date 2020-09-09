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

// Webserver
WiFiServer server(80);

// Sender
HTTPClient sender;

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

          
      }
      
    }else{
      // Falls HTTP-Error
      // HTTP-Error: ", sender.errorToString(httpCode).c_str()
    }

    // Wenn alles abgeschlossen ist, wird die Verbindung wieder beendet
    sender.end();
    
  }else {
    // HTTP-Verbindung konnte nicht hergestellt werden!
  }
  
}



void setup() {

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Pin
  pinMode(pinAlarmanlageStatus, INPUT);
  pinMode(pinAlarmanlageAusgeloest, INPUT);
  
  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Start the server
  server.begin();

  // Set wait to sec
  wait = wait * 1000;
}

void loop() {

  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
  delay(500);                       // wait 
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
  
  checkState();
  
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
          client.println("<head><meta http-equiv=refresh content=15></head><body bgcolor=33363B><center><font face=Ubuntu color=FFFFFF><h1>Status Alarmanlage</h1><h3>Hardware: Wemos D1 Mini</h3>");

          client.println("<br>");

          client.println("<h3>Alarmanlage Status (D1) &rArr; ");

          if (digitalRead(pinAlarmanlageStatus) == LOW)
            {
              client.println("Open"); 
            }
          else
            {
              client.println("Closed"); 
            }

          client.println("</h3>");
          
          client.println("<h3>Alarmanlage Ausgeloest (D2) &rArr; "); 

          if (digitalRead(pinAlarmanlageAusgeloest) == LOW)
            {
              client.println("Open"); 
            }
          else
            {
              client.println("Closed"); 
            }

          client.println("</h3>");

          client.println("<br><h3>Messintervall &rArr; " + String(wait / 1000) + " sec</h3>");
          
          client.println("<br><h3>Alarmanlage Status &rArr; " + securitystatecurrent + "</h3><h4>(Stay=0 / Away=1 / Night=2 / Disarmed=3 / Triggered=4)</h4>");  

          client.println("<br><h3>Request Webhooks:</h3><h4>http://" + hbip + ":" + hbport + "/?accessoryId=" + hbid + "&amp;targetstate=" + securitystatetarget + "</h4>");
          client.println("<br><h3>Request Webhooks:</h3><h4>http://" + hbip + ":" + hbport + "/?accessoryId=" + hbid + "&amp;currentstate=" + securitystatecurrent + "</h4>"); 

          client.println("<br><h3>Sketch: Von Jan mit Liebe gemacht</h3></font></center></body></html>"); 
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
