#include <HTTPClient.h>
#include <WiFi.h>


const char* ssid = "ZON-AFC0";
const char* password =  "f933af58161c";

void setup() {

  Serial.begin(115200);
  pinMode(2,OUTPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");

}

void loop() {
  

 if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
  
   HTTPClient http;   
   digitalWrite(2,HIGH);
   http.begin("http://192.168.1.7:8090/post");
   http.addHeader("Content-Type", "text/plain");             

   int httpResponseCode = http.POST("POSTING from ESP32");   //Send the actual POST request

   if(httpResponseCode>0){

    Serial.println(httpResponseCode); 
   }else{

    Serial.println("Error on sending POST");

   }

   http.end();  //Free resources

 }else{

    Serial.println("Error in WiFi connection");   

 }

  delay(10000);  //Send a request every 10 seconds

}