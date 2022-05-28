//
//Home alarm system prototype, NodeMCU/ESP8266
//Movement detection with PIR sensor, sends email alert to user using SMTP
//Light Sleep mode between interrupts
//

#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>

extern "C"{
  #include "gpio.h"
}

extern "C"{
  #include "user_interface.h"
}

const int LED = 2; //Pin D4 on NodeMCU
const int SENSOR = 12; //Pin D6 on NodeMCU
int sensor_status = 0; //Movement detection flag

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

void setup() {

  gpio_init();

  //Serial monitor for debugging, default baudrate
  Serial.begin(115200);
  
  //Set GPIO pins on board, LED as output, PIR as input with pullup resistor
  pinMode(LED,OUTPUT);
  pinMode(SENSOR,INPUT_PULLUP); 
  digitalWrite(LED,LOW);
  digitalWrite(SENSOR,LOW);

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

  //Connect to to WiFi initially, otherwise wakes up from sleep immediately
  connectWifi();

  //Give the system some start-up time (15s)
  delay(15000);
}


void connectWifi(){

  //Station mode
  WiFi.mode(WIFI_STA);

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

}

//Enter Light sleep mode
//Wake up from external interrupt from SENSOR pin
void lightSleep(){
  
  WiFi.mode(WIFI_OFF);
  delay(200);
  Serial.println("Going to Sleep");

  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_enable(GPIO_ID_PIN(SENSOR), GPIO_PIN_INTR_HILEVEL); 
  wifi_fpm_open();
  wifi_fpm_do_sleep(0xFFFFFFF); //sleep for max time  
  delay(200);
  wifi_fpm_close();

  Serial.println("Waking up!"); 

  //PIR stays high for some time, check if woke up to interrupt
  //and not sleep timer timeout
  if(digitalRead(SENSOR) == HIGH)
    sensor_status = 1;
}

//LED blinker indicator
void ledBlinker(){

  digitalWrite(LED,HIGH);
  delay(500);
  digitalWrite(LED,LOW);
  delay(500);
}

void loop() {
    
  if(sensor_status == 1){

    //Connect to wifi
    connectWifi();
    
    //SEND USER ALERT NOTIFICATION
    //Connect to server with session data
    if(!smtp.connect(&session))
      Serial.println("Unable to connect server");
  
    //Attempt sending Email
    if(!MailClient.sendMail(&smtp, &message))
      Serial.println("Error sending Email, " + smtp.errorReason());
    
  }

  //Sensor reading needs to be low when enters sleep-mode
  //Otherwise won't wake up
  if(digitalRead(SENSOR) == HIGH){
    ledBlinker();
    sensor_status = 0;
  }else{ 
    lightSleep();
  }

  //Without this SENSOR pin stays high indefinitely
  pinMode(SENSOR,INPUT_PULLUP);
 
}

//Print email info
void smtpCallback(SMTP_Status status){

  Serial.println(status.info());

}
