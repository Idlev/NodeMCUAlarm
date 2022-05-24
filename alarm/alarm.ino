
//Home alarm system prototype, NodeMCU/ESP8266
//Movement detection with PIR sensor, sends email alert to user using SMTP
//Sends time data to backend server for storage via url encoded HTTP post request

#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>

const int LED = 2; //Pin D4 on NodeMCU
const int SENSOR = 12; //Pin D6 on NodeMCU
int sensorStatus = 0; //Movement detection flag

//Credentials for Wi-Fi network
const char* WIFI_NAME = "REPLACE_WIFI_NAME";
const char* WIFI_PASSWORD = "REPLACE_WIFI_PASSWORD";

//SMTP host details
const char* SMTP_HOST = "smtp.gmail.com";
const int SMTP_PORT = 465;

//Time zone offset in seconds for local time (UTC+3)
const int utcOffset = 3*60*60;

//NTP/Udp
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"pool.tnp.org",utcOffset);

//Credentials for sending email
const char* SEND_EMAIL = "REPLACE_SENDER_EMAIL_ADDRESS";
const char* SEND_PASSWORD = "REPLACE_SENDER_PASSWORD";

//Email address to recieve alert
const char* RECIEVE_EMAIL = "REPLACE_RECIEVE_EMAIL_ADDRESS";

//Backend server for HTTP post
const char* SERVER_NAME = "REPLACE_SERVER_NAME/IP";

//SMTP object for email sending
SMTPSession smtp;

//Email session object
ESP_Mail_Session session;

//SMTP message object
SMTP_Message message;

//Callback function for eamil status
void smtpCallback(SMTP_Status status);

//Interrupt callback function, store to RAM
void ICACHE_RAM_ATTR handleInterrupt();

void setup() {
  
  Serial.begin(115200); //Serial monitor for debugging, default baudrate
  pinMode(LED,OUTPUT);
  pinMode(SENSOR,INPUT_PULLUP); 
  digitalWrite(LED,LOW);

  //External interrupt for sensor on rising edge
  attachInterrupt(digitalPinToInterrupt(SENSOR),handleInterrupt,RISING);

  //Disable interrupt while connecting to Wi-Fi
  ETS_GPIO_INTR_DISABLE();

  //Connect to Wi-Fi network
  WiFi.mode(WIFI_STA); //Station mode
  Serial.println("Starting Wi-Fi connection...");
  WiFi.begin(WIFI_NAME,WIFI_PASSWORD);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println("Connecting...");
  }

  Serial.println("Wifi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Enable SMTP serial monitor debug
  smtp.debug(1);

  //Set callback function for sending results
  smtp.callback(smtpCallback);

  //Set session data
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = SEND_EMAIL;
  session.login.password = SEND_PASSWORD;
  session.login.user_domain = "";

  //Set message properties
  message.sender.name = "ESP8266";
  message.sender.email = SEND_EMAIL;
  message.subject = "ESP8266 ALERT";
  message.addRecipient("T", RECIEVE_EMAIL);

  //Send raw text
  String emailMessage = "Movement detected!";
  message.text.content = emailMessage.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  //Start time client
  timeClient.begin();

  //Enable interrupt
  ETS_GPIO_INTR_ENABLE();
}

void loop() {

  //PIR sensor interrupt triggers email notification
  if(sensorStatus == 1){
    
    //Disable interrupts
    ETS_GPIO_INTR_DISABLE();
    if(WiFi.status() == WL_CONNECTED){

      //HTTP and WiFi objects
      WiFiClient clnt;
      HTTPClient http;

      //SEND DATETIME TO BACKEND SERVER
      if(!http.begin(clnt,SERVER_NAME)){
        Serial.println("Error, Unable to begin http");
      }else{
  
        //Update time
        timeClient.update();
        time_t eTime = timeClient.getEpochTime();
        struct tm *ptm = gmtime((time_t*)&eTime);
        int dateYear = ptm->tm_year+1900;
        int dateMonth = ptm->tm_mon+1;
        int dateDay = ptm->tm_mday;
        String fTime = timeClient.getFormattedTime();

        //Define content type, url encoded HTTP post request
        http.addHeader("Content-Type","application/x-www-form-urlencoded");
        
        //Format date time "date=YYYY-MM-DD HH:MM:SS"
        String httpData = "date=" +String(dateYear) + "-" + 
        String(dateMonth) + "-" + String(dateDay) + " " + fTime; 
        Serial.print("Data sent: ");
        Serial.println(httpData);
  
        //HTTP post request
        int httpSend = http.POST(httpData); //send
        String payload = http.getString(); //response
  
        Serial.print("HTTP Code: ");
        Serial.println(httpSend);
  
        Serial.print("Response: ");
        Serial.println(payload);

      }

      //SEND USER ALERT NOTIFICATION
      //Connect to server with session data
      if(!smtp.connect(&session))
        Serial.println("Unable to connect server");

      //Attempt sending Email
      if(!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());

    }else{
      //Try reconnect Wi-Fi if connection lost
      Serial.println("Wifi disconnected...");

      Serial.println("Starting Wi-Fi connection...");
      WiFi.begin(WIFI_NAME,WIFI_PASSWORD);

      while(WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.println("Connecting...");
      }

      Serial.println("Wifi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }else{
    //Do nothing when movement not detected
    //Could use sleep modes with interrupts...
  }

  //Reset interrupt flag
  sensorStatus = 0;

  //Enable interrupts
  ETS_GPIO_INTR_ENABLE();

}

//Interrupt routine
//Triggers email notification, blinks LED
void handleInterrupt(){
  digitalWrite(LED,HIGH);
  delayMicroseconds(2000000); //2s delay
  digitalWrite(LED,LOW);

  sensorStatus = 1; //Set interrupt flag
}

//Print email info
void smtpCallback(SMTP_Status status){

  Serial.println(status.info());

}
