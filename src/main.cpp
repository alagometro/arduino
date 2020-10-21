#include <Arduino.h>
#include <SoftwareSerial.h>

SoftwareSerial SerialESP8266(10, 11); // RX, TX

// Wifi settings
String ssid = "";
String password = "";

// Api Settings
String location = "1";
String level = "1";
String server = "alagometro.herokuapp.com";
String secret = "";

String cadena = ""; //to store HTTP request

void setup()
{
  SerialESP8266.begin(9600);
  Serial.begin(9600);
  delay(10000);
  SerialESP8266.setTimeout(5000);
  //checking the ESP8266 response
  SerialESP8266.println("AT");
  if (SerialESP8266.find("OK"))
    Serial.println("AT OK");
  else
    Serial.println("Error on ESP8266");
  //ESP8266 in STA mode.
  SerialESP8266.println("AT+CWMODE=1");
  if (SerialESP8266.find("OK"))
    Serial.println("ESP8266 on STATION mode...");
  //Connecting to wifi
  SerialESP8266.println("AT+CWJAP=\""+ssid+"\",\""+password+"\"");
  Serial.println("Connnecting...");
  SerialESP8266.setTimeout(10000); //waiting for connection
  if (SerialESP8266.find("OK"))
    Serial.println("WIFI OK");
  else
    Serial.println("Unable to connect...");
  SerialESP8266.setTimeout(2000);
  //Disable multiple connections
  SerialESP8266.println("AT+CIPMUX=0");
  if (SerialESP8266.find("OK"))
    Serial.println("Multiple connections disabled");
}

void loop()
{
  SerialESP8266.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");
  //connecting to server
  if (SerialESP8266.find("OK"))
  {
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println("Server connection successful...");
    //Armamos el encabezado de la peticion http
    String peticionHTTP = "GET /update/"+secret+"/"+location+"/"+level;
    peticionHTTP = peticionHTTP + " HTTP/1.1\r\n";
    peticionHTTP = peticionHTTP + "Host: alagometro.herokuapp.com\r\n\r\n";
    peticionHTTP = peticionHTTP + "Host: localhost\r\n\r\n";
    //Sending the length of the HTTP request
    SerialESP8266.print("AT+CIPSEND=");
    SerialESP8266.println(peticionHTTP.length());
    //waiting for ">" for sending HTTP request
    if (SerialESP8266.find(">"))
    {
      //we can send the HTTP request when > is displayed
      Serial.println("Sending HTTP request. . .");
      SerialESP8266.println(peticionHTTP);
      if (SerialESP8266.find("SEND OK"))
      {
        Serial.println("HTTP request sent...:");
        Serial.println();
        Serial.println("On stand by...");
        boolean fin_respuesta = false;
        long tiempo_inicio = millis();
        cadena = "";
        while (fin_respuesta == false)
        {
          while (SerialESP8266.available() > 0)
          {
            char c = SerialESP8266.read();
            Serial.write(c);
            cadena.concat(c); //store the request string on "cadena"
            if (cadena.length() > 50)
              cadena = "";
          }
          //terminate if "cadena" length is greater than  3500
          if (cadena.length() > 3500)
          {
            Serial.println("The request exceeded the maximum length...");
            SerialESP8266.println("AT+CIPCLOSE");
            if (SerialESP8266.find("OK"))
              Serial.println("Connection terminated...");
            fin_respuesta = true;
          }
          if ((millis() - tiempo_inicio) > 10000)
          {
            //Terminate if connection time exceeded the maximum
            Serial.println("Connection time exceeded...");
            SerialESP8266.println("AT+CIPCLOSE");
            if (SerialESP8266.find("OK"))
              Serial.println("Connection terminated");
            fin_respuesta = true;
          }
          if (cadena.indexOf("CLOSED") > 0)
          {
            Serial.println();
            Serial.println("String OK, connection terminated");
            fin_respuesta = true;
          }
        }
      }
      else
      {
        Serial.println("error on HTTP request.....");
      }
    }
  }
  else
  {
    Serial.println("Unable to find the server....");
  }
  delay(1000); //1 second delay before new loop
}
