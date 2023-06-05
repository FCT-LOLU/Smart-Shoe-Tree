#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <DHT.h>
#include <TimeLib.h>

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

  setTime(0, 0, 0, 6, 5, 2023); 
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
  
  time_t now = time(nullptr);
  char timestamp[25];
  sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",year(now), month(now), day(now), hour(now), minute(now), second(now));
  
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
  digitalWrite(2, HIGH);
  }else{
    Serial.println("Error in WiFi connection");   
  }
  delay(1000);  //Send a request every 1 second
  digitalWrite(2, LOW);
  delay(1000);
  if (Firebase.ready() && (millis() - dataMillis > 60000 || dataMillis == 0)){
    dataMillis = millis();

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

        // get the Humidity Rate Threshold of fbdo.payload().c_str()
        double humidityRateThreshold = fbdo.payload().substring(fbdo.payload().indexOf("Humidity Rate Threshold") + 23, fbdo.payload().indexOf("Humidity Rate Threshold") + 27).toDouble();
        Serial.print("Humidity Rate Threshold: ");
        Serial.println(humidityRateThreshold); // 40.8
    }else
        Serial.println(fbdo.errorReason());

    documentPath = "user/lSYK0UAxhqcYKK6z4zXHfuMK58w1";
    mask = "";
    
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask.c_str())){
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      
    }else{
      Serial.println(fbdo.errorReason());
      
    }
      
  }

}

