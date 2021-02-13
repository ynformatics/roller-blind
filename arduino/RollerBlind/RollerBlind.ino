#include <ArduinoOTA.h>

#include <PubSubClient.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include "Blind.h"

#define AIN1 D6
#define AIN2 D5
#define PWMA D0
#define MAG1 D2

#define BIN1 D8
#define BIN2 D3
#define PWMB D4
#define MAG2 D1

#define STBY D7

String command = "";         // a string to hold incoming data
boolean commandComplete = false;  // is the command complete

WiFiClient espClient;
PubSubClient client(espClient);
char sendBuffer[256];

Blind blind1( AIN1, AIN2, PWMA, STBY, MAG1);
//Blind blind2( BIN1, BIN2, PWMB, STBY, MAG2);


void setup() 
{
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(MAG1), ISR1, CHANGE); 
//  attachInterrupt(digitalPinToInterrupt(MAG2), ISR2, CHANGE); 

 WiFi.begin("<$wifi-ssid$>", "<$wifi-password$>");
 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    //Serial.println("Connecting to WiFi..");
  }
  //Serial.println("Connected to the WiFi network");
 
  client.setServer("192.168.1.73", 1883);
  client.setCallback(callback);
 
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client" )) 
    {
      DBG("connected");   
    } 
    else 
    { 
      Serial.print("failed with state: ");
      Serial.println(client.state());
      delay(500); 
    }
  }

  DBG("roller blinds IP: %s", WiFi.localIP().toString().c_str());
 
  client.subscribe("blinds/move"); 

    ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  
}

void loop() {
   ArduinoOTA.handle();
  
   if(!client.loop())
   {
      ESP.restart();
   }
  
    doSerialEvent();
    
    if (commandComplete) 
    {
       ProcessCommand();
       command = "";
       commandComplete = false;
    }

    blind1.loop();
}

void ProcessCommand()
{
   Serial.println( command);
  
   if(command == "f")
   {
     blind1.motor->drive(1023);
   }
   else if(command == "b")
   {
     blind1.motor->drive(-1023);
   }
   else if(command == "s")
   {
     blind1.motor->brake();
   }
   else if(command == "y")
   {
     blind1.motor->standby();
   }
   else if(command.startsWith("m"))
   {   
     float revs  = command.substring(1).toFloat();      
     blind1.move(revs);
   } 
}

void callback(char* topic, byte* payload, unsigned int length) 
{
 char buf[length+1];
 
  for (int i = 0; i < length; i++) 
  {
    buf[i] = (char)payload[i];
  }
  buf[length] = '\0';

  if(strcmp(topic, "blinds/move") == 0)
  { 
    float pos = atof(buf);
    blind1.move(pos);

//    sprintf(sendBuffer, "%s", "moved");
//    client.publish("blinds/status", sendBuffer);
  }
 
  else
  {
    client.publish("blinds/debug", "unimplemented topic");
  }
  
}

void DBG( char* fmt, ...)
{
   va_list args;          // args variable to hold the list of parameters
   va_start(args, fmt);  // mandatory call to initilase args

   vsnprintf(sendBuffer, sizeof(sendBuffer),fmt, args);
   client.publish("blinds/debug", sendBuffer);
//   Serial.println(sendBuffer);
}
   
void ISR1()
{
  blind1.ISR();
}
//void ISR2()
//{
//  blind2.ISR();
//}

void doSerialEvent() 
{
  while (Serial.available()) 
  {
    char inChar = (char)Serial.read(); 
   
    if (inChar == '\n') 
    {
      commandComplete = true;     
    } 
    else if (inChar != '\r') 
      command += inChar;
  }
}

