//
//Home alarm system, NodeMCU/ESP8266
//Movement detection via PIR sensor, sends email alert with SMTP
//

#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>

const int LED = 2; //Pin D4 on NodeMCU
const int SENSOR = 12; //Pin D6 on NodeMCU
int sensorStatus = 0; //Movement detection flag

//Credentials for Wi-Fi network
const char* WIFI_NAME = "REPLACE_WIFI_NAME";
const char* WIFI_PASSWORD = "REPLACE_WIFI_PASSWORD";

//SMTP host details
const char* SMTP_HOST = "smtp.gmail.com";
const int SMTP_PORT = 465;

//Credentials for sending email
const char* SEND_EMAIL = "REPLACE_SENDER_EMAIL_ADDRESS";
const char* SEND_PASSWORD = "REPLACE_SENDER_PASSWORD";

//Email address to recieve alert
const char* RECIEVE_EMAIL = "REPLACE_RECIEVE_EMAIL_ADDRESS";

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
  message.sender.name = "ESP";
  message.sender.email = SEND_EMAIL;
  message.subject = "ESP8266 ALERT";
  message.addRecipient("Tuomas", RECIEVE_EMAIL);

  //Send raw text
  String textMsg = "Movement detected!";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

}

void loop() {

  //PIR sensor interrupt triggers email notification
  if(sensorStatus == 1){
    
    //Disable interrupts
    ETS_GPIO_INTR_DISABLE();
    if(WiFi.status() == WL_CONNECTED){
 
      //Connect to server with session data
      if(!smtp.connect(&session))
        Serial.println("Unable to connect server");

      //Attempt sending Email
      if(!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());

    }else{
      //Try reconnect Wi-Fi
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

  //Print current status
  Serial.println(status.info());

  //Print email results
  if(status.success()){
    ESP_MAIL_PRINTF("Message success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message failed: %d\n", status.failedCount());

  }
}
