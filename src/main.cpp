#include <Arduino.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

#define RX 10
#define TX 11

SoftwareSerial SerialESP8266(10, 11);
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

// Wifi settings
String ssid = "SSID";
String password = "SENHA";
bool connection_error = false;

// Api Settings
String location = "0";
String server = "alagometro.herokuapp.com";
String secret = "SEGREDO";

// App settings
String level = "0";
int iterations = 0;
bool debug_mode = true;
int molhado = 550;
bool request_ok = false;
int loop_counter = 0;

// LCD Helpers
int Li          = 16;
int Lii         = 0;
int Ri          = -1;
int Rii         = -1;

void setup()
{
  Serial.begin(9600);
  SerialESP8266.begin(9600);
  lcd.begin(16, 2);

  lcd.print("Iniciando");
  initialize();
  connect();

  if(connection_error) {
    lcd.setCursor(0, 0);
    lcd.print("      ERRO      ");
    lcd.setCursor(0, 1);
    lcd.print("  Erro na Rede  ");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("   Alagometro   ");
  }
}

void loop()
{
  if(connection_error) return;

  String sensorLevel = sensor_reading();
  int request_counter = 0;

  loop_counter++;

  if(loop_counter == 4) {
    loop_counter = 0;

    if(!request_ok && request_counter < 3) {
      request(sensorLevel);
      request_counter++;
    }

    // Updates the level in API if is different than the previous one
    if (sensorLevel != level)
    {
      Serial.println("");
      Serial.println("Identificando mudança de nível: de " + level + " para " + sensorLevel);
      iterations = 0;
      request_ok = false;
      request(sensorLevel);
      level = sensorLevel;

      if(request_ok) request_counter = 0;
    }
  }

  if(sensorLevel == "10") {
    lcd.setCursor(0, 0);
    lcd.print("     ERRO     ");
  }

  lcd.setCursor(0, 1);
  lcd.print(Scroll_LCD_Left(nivel_texto(sensorLevel)));

   // Updates each minute, used as keep-alive indicator
   if(iterations == 240) {
     iterations = 0;
     request(level);
   }

  iterations++;
  delay(250);
}

void initialize()
{
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
}

String nivel_texto(String nivel){
    if(nivel == "0")
      return "Seguro atravessar";

    if(nivel == "1")
      return "Cuidado ao atravessar";

    if(nivel == "2")
      return "Perigo ao atravessar";

    if(nivel == "3")
      return "Nao atravesse!";

    if(nivel == "10")
      return "Inconsistência na Leitura";

    return nivel;
}

String sensor_reading()
{
  int s1 = analogRead(A3);
  int s2 = analogRead(A4);
  int s3 = analogRead(A5);

  if(debug_mode) {
    Serial.println(s1);
    Serial.println(s2);
    Serial.println(s3);
    Serial.println("---");
  }

  if (s3 >= molhado && s2 >= molhado && s1 >= molhado)
    return "3";

  if (s2 >= molhado && s1 >= molhado && s3 < molhado)
    return "2";

  if (s1 >= molhado && s2 < molhado && s3 < molhado)
    return "1";

  if(s1 < molhado && s2 < molhado && s3 < molhado)
    return "0";

  return "10";
}

String store_and_print(String body)
{
  while (SerialESP8266.available() > 0)
  {
    char c = SerialESP8266.read();
    body.concat(c);

    if (body.length() > 50)
      body = "";

    Serial.write(c);
  }

  return body;
}

void request(String reqLevel)
{
  String body = "";
  boolean request_end = false;

  lcd.setCursor(0, 0);
  lcd.print("   Alagometro   ");
  lcd.setCursor(0, 1);
  lcd.print(" Atualizando... ");

  SerialESP8266.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");

  // Opens connection to the server
  if (!SerialESP8266.find("OK"))
    return Serial.println("Unable to find the server....");

  // Request Header
  String httpRequest = "GET /update/" + secret + "/" + location + "/" + reqLevel;
  httpRequest = httpRequest + " HTTP/1.1\r\n";
  httpRequest = httpRequest + "Host: " + server + "\r\n\r\n";
  httpRequest = httpRequest + "Host: localhost\r\n\r\n";

  // Sends the request length
  SerialESP8266.print("AT+CIPSEND=");
  SerialESP8266.println(httpRequest.length());

  // waiting for ">" for sending HTTP request
  if (!SerialESP8266.find(">"))
    return;

  Serial.println("Sending HTTP request. . .");
  SerialESP8266.println(httpRequest);

  if (!SerialESP8266.find("SEND OK"))
    return Serial.println("error on HTTP request.....");

  Serial.println("HTTP request sent...:");

  long start_time = millis();

  while (request_end == false)
  {
    body = store_and_print(body);

    if(body.length() > 0)
      Serial.println(body);

    // terminate if "body" length is greater than 3500
    if (body.length() > 3500)
    {
      SerialESP8266.println("AT+CIPCLOSE");
      request_end = true;

      Serial.println("The request exceeded the maximum length...");
      if (SerialESP8266.find("OK"))
        Serial.println("Connection terminated...");
    }

    // Terminate if connection time exceeded the maximum
    if ((millis() - start_time) > 2000)
    {
      SerialESP8266.println("AT+CIPCLOSE");
      request_end = true;

      Serial.println("Connection time exceeded...");
      if (SerialESP8266.find("OK"))
        Serial.println("Connection terminated");
    }

    if (body.indexOf("CLOSED") > 0 || body.indexOf("200 OK") > 0 || body.indexOf("last_update") > 0)
    {
      Serial.println("String OK, connection terminated");
      request_end = true;
    }
  }
  request_ok = true;
}


void connect()
{
  Serial.println("================");
  Serial.println("Conexão ao Wifi:");
  Serial.println("================");

  lcd.setCursor(0, 1);
  Serial.println("Tentando conectar");
  lcd.write('.');
  delay(5000);
  SerialESP8266.setTimeout(5000);

  // checking the ESP8266 response
  SerialESP8266.println("AT");

  if (SerialESP8266.find("OK")) {
    Serial.println("AT OK");
    lcd.write('.');
  } else {
    Serial.println("Erro no ESP8266");
    lcd.write('x');
    connection_error = true;
    return;
  }

  // ESP8266 in STA mode.
  SerialESP8266.println("AT+CWMODE=1");

  if (SerialESP8266.find("OK"))
    Serial.println("ESP8266 on STATION mode...");

  // Connecting to wifi
  SerialESP8266.println("AT+CWJAP=\"" + ssid + "\",\"" + password + "\"");
  Serial.println("Conectando a " + ssid);
  SerialESP8266.setTimeout(10000); //waiting for connection

  if (SerialESP8266.find("OK")) {
    Serial.println("WIFI OK");
    lcd.write('.');
  } else {
    Serial.println("Não é possível conectar");
    lcd.write('x');
    connection_error = true;
    return;
  }

  SerialESP8266.setTimeout(2000);
  lcd.write('!');

  // Disable multiple connections
  SerialESP8266.println("AT+CIPMUX=0");
  if (SerialESP8266.find("OK"))
    Serial.println("Múltiplas conexões estão desabilitadas");
}

String Scroll_LCD_Left(String StrDisplay){
  String result;
  String StrProcess = "                " + StrDisplay + "                ";
  result = StrProcess.substring(Li,Lii);
  Li++;
  Lii++;
  if (Li>StrProcess.length()){
    Li=16;
    Lii=0;
  }
  return result;
}

void Clear_Scroll_LCD_Left(){
  Li=16;
  Lii=0;
}
