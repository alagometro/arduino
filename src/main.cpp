#include <Arduino.h>
#include <SoftwareSerial.h>

#define RX 10
#define TX 11

SoftwareSerial SerialESP8266(RX, TX); // RX, TX

// Wifi settings
String ssid = "";
String password = "";

// Api Settings
String location = "1";
String server = "alagometro.herokuapp.com";
String secret = "";

// App settings
boolean debug = false;
String level = "0";

void setup() {
  SerialESP8266.begin(9600);
  Serial.begin(9600);

  connect();
}

void loop() {
  String sensorLevel = sensor_reading();

  if(sensorLevel != level) {
    level = sensorLevel;
    request(level);
  }

  delay(1000);
}

String sensor_reading() {
  return "";
}

String store_and_print(String body) {
  while (SerialESP8266.available() > 0) {
    char c = SerialESP8266.read();
    body.concat(c);

    if (body.length() > 50)
      body = "";

    if (logger) Serial.write(c);
  }

  return body;
}

void logger(String log) {
  if(debug) logger(log);
}

void request(String reqLevel) {
  String body = "";
  boolean request_end = false;

  SerialESP8266.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");

  // Opens connection to the server
  if (!SerialESP8266.find("OK"))
    return logger("Unable to find the server....");

  // Request Header
  String httpRequest = "GET /update/" + secret + "/" + location+"/" + reqLevel;
  httpRequest = httpRequest + " HTTP/1.1\r\n";
  httpRequest = httpRequest + "Host: " + server + "\r\n\r\n";
  httpRequest = httpRequest + "Host: localhost\r\n\r\n";

  // Sends the request length
  SerialESP8266.print("AT+CIPSEND=");
  SerialESP8266.println(httpRequest.length());

  // waiting for ">" for sending HTTP request
  if (SerialESP8266.find(">"))
    return;

  logger("Sending HTTP request. . .");
  SerialESP8266.println(httpRequest);

  if (!SerialESP8266.find("SEND OK"))
    return logger("error on HTTP request.....");

  logger("HTTP request sent...:");

  long start_time = millis();

  while (request_end == false) {
    body = store_and_print(body);

    // terminate if "body" length is greater than 3500
    if (body.length() > 3500) {
      SerialESP8266.println("AT+CIPCLOSE");
      request_end = true;

      logger("The request exceeded the maximum length...");
      if (SerialESP8266.find("OK"))
        logger("Connection terminated...");
    }

    // Terminate if connection time exceeded the maximum
    if ((millis() - start_time) > 10000) {
      SerialESP8266.println("AT+CIPCLOSE");
      request_end = true;

      logger("Connection time exceeded...");
      if (SerialESP8266.find("OK"))
        logger("Connection terminated");
    }

    if (body.indexOf("CLOSED") > 0) {
      logger("String OK, connection terminated");
      request_end = true;
    }
  }
}

void connect() {
  SerialESP8266.setTimeout(5000);

  // checking the ESP8266 response
  SerialESP8266.println("AT");

  if (SerialESP8266.find("OK"))
    logger("AT OK");
  else
    logger("Error on ESP8266");

  // ESP8266 in STA mode.
  SerialESP8266.println("AT+CWMODE=1");

  if (SerialESP8266.find("OK"))
    logger("ESP8266 on STATION mode...");

  // Connecting to wifi
  SerialESP8266.println("AT+CWJAP=\"" + ssid + "\",\"" + password + "\"");
  logger("Connnecting...");
  SerialESP8266.setTimeout(10000); //waiting for connection

  if (SerialESP8266.find("OK"))
    logger("WIFI OK");
  else
    logger("Unable to connect...");

  SerialESP8266.setTimeout(2000);

  // Disable multiple connections
  SerialESP8266.println("AT+CIPMUX=0");
  if (SerialESP8266.find("OK"))
    logger("Multiple connections disabled");
}
