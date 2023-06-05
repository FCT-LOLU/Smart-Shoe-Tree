#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <DHT.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

#define API_KEY "AIzaSyAClVANQ0FM1-1yWTTGdz5U1tAU_ki6qmw"
#define FIREBASE_PROJECT_ID "smartshoetree-bedd2"
#define USER_EMAIL "lucie.boucher@telecomnancy.eu"
#define USER_PASSWORD "test12"


const char* ssid = "ZON-AFC0";
const char* password =  "f933af58161c";

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;
String humidityFirebase;
double humidityRateMax= 0.0;
double tempMax=0.0;

#define LIGHT_SENSOR_PIN 36
#define DHT_SENSOR_PIN 21
#define DHT_TYPE DHT11

#define BLUE_PIN 14
#define GREEN_PIN 12
#define RED_PIN 13

const int MOTION_SENSOR_PIN = 32;
int stateMotion=LOW;

DHT dht(DHT_SENSOR_PIN, DHT_TYPE);

String mask = "";
time_t t;

const long utcOffsetInSeconds = 3600; // Décalage horaire en secondes
const char* ntpServer = "pool.ntp.org";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,ntpServer, utcOffsetInSeconds);


// The Firestore payload upload callback function
void fcsUploadCallback(CFS_UploadStatusInfo info)
{
    if (info.status == fb_esp_cfs_upload_status_init)
    {
        Serial.printf("\nUploading data (%d)...\n", info.size);
    }
    else if (info.status == fb_esp_cfs_upload_status_upload)
    {
        Serial.printf("Uploaded %d%s\n", (int)info.progress, "%");
    }
    else if (info.status == fb_esp_cfs_upload_status_complete)
    {
        Serial.println("Upload completed ");
    }
    else if (info.status == fb_esp_cfs_upload_status_process_response)
    {
        Serial.print("Processing the response... ");
    }
    else if (info.status == fb_esp_cfs_upload_status_error)
    {
        Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

void setup() {

  Serial.begin(115200);

  pinMode(2,OUTPUT);

  //Initialize the sensor
  pinMode(MOTION_SENSOR_PIN,INPUT);

  //Initialize the LED
  pinMode(BLUE_PIN,OUTPUT);
  pinMode(GREEN_PIN,OUTPUT);
  pinMode(RED_PIN,OUTPUT);
  t = now();
  setTime(t);

  //Initialize the DHT sensor
  dht.begin();

  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");


  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;


    /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);
  
}

void loop() {
  

  int analogValue = analogRead(LIGHT_SENSOR_PIN);
  stateMotion = digitalRead(MOTION_SENSOR_PIN);
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();
  

  char timestamp[30];
  sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",year(t),month(t),day(t), hour(t), minute(t), second(t));
  Serial.print(timestamp);

  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
  digitalWrite(2, HIGH);
  }else{
    Serial.println("Error in WiFi connection");   
  }
  delay(60000);  //Send a request every 1 minute
  digitalWrite(2, LOW);
  delay(1000);
  if (Firebase.ready() && (millis() - dataMillis > 60000 || dataMillis == 0)){
    dataMillis = millis();

    String documentPath1 = "user/lSYK0UAxhqcYKK6z4zXHfuMK58w1/";
    mask = "";
    double valuehumidity =0.0;
    double valuetemperature =0.0;

    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath1.c_str(), mask.c_str())){

      // get the Humidity Rate Threshold of fbdo.payload().c_str()
      int startIndex = fbdo.payload().indexOf("Humidity Rate Threshold") + 48 ;
      int endIndex = fbdo.payload().indexOf("Humidity Rate Threshold") + 55 ;
      String humidityRateThreshold = fbdo.payload().substring(startIndex, endIndex) ;
      valuehumidity = humidityRateThreshold.toDouble();

      // get the Temperature Threshold of fbdo.payload().c_str()
      int startIndex1 = fbdo.payload().indexOf("Temperature Threshold") + 47 ;
      int endIndex1 = fbdo.payload().indexOf("Temperature Threshold") + 55 ;
      String temperatureThreshold = fbdo.payload().substring(startIndex1, endIndex1) ;
      valuetemperature = temperatureThreshold.toDouble();

      Serial.print("Humidity Rate Threshold: ");
      Serial.println(valuehumidity);
      Serial.print("\n Temperature Threshold: ");
      Serial.println(valuetemperature);
      
    }else{
      Serial.println(fbdo.errorReason());
      
    }

    if(humidity>valuehumidity || temp>valuetemperature){
      Serial.println(valuehumidity);
      Serial.println(humidity);
      Serial.println(valuetemperature);
      Serial.println(temp);
      digitalWrite(RED_PIN,HIGH);
      String documentPathNeedCleaning = "shoe/hZBzDnPYJXmkqQo6oIeu";
      FirebaseJson json;
      json.set("fields/NeedCleaning/booleanValue", "true");
      Serial.print("Update need cleaning value... ");
      if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPathNeedCleaning.c_str(), json.raw(), "NeedCleaning" /* updateMask */))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        else
            Serial.println(fbdo.errorReason());


    }else{
      String documentPathNeedCleaning = "shoe/hZBzDnPYJXmkqQo6oIeu";
      FirebaseJson json;
      json.set("fields/NeedCleaning/booleanValue", "false");
      Serial.print("Update need cleaning value... ");
      if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPathNeedCleaning.c_str(), json.raw(), "NeedCleaning" /* updateMask */))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        else
            Serial.println(fbdo.errorReason());
    }

    if(analogValue>2000 || stateMotion==HIGH){
      digitalWrite(GREEN_PIN,HIGH);
      String documentPathNeedCleaning = "shoe/hZBzDnPYJXmkqQo6oIeu";
      FirebaseJson json;
      json.set("fields/DateLastWorn/timestampValue", timestamp);
      Serial.print("Update date last worn... ");
      
      if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" , documentPathNeedCleaning.c_str(), json.raw(), "DateLastWorn" ))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        else
            Serial.println(fbdo.errorReason());

    }else{
      digitalWrite(GREEN_PIN,LOW);
    }


    FirebaseJson content;

    String documentPath = "shoeData/time"+ String(count);

    //String
    content.set("fields/IDofShoe/stringValue","hZBzDnPYJXmkqQo6oIeu");

    // integer
    content.set("fields/light/integerValue", analogValue);
    content.set("fields/motion/integerValue", stateMotion);
    
    //double
    content.set("fields/humidity/doubleValue", humidity);
    content.set("fields/temperature/doubleValue", temp);

    // timestamp
    content.set("fields/date/timestampValue", timestamp); // RFC3339 UTC "Zulu" format


    count++;

    Serial.print("Create a document... ");

    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw())){
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());

    }else
        Serial.println(fbdo.errorReason());

      
  }

}
