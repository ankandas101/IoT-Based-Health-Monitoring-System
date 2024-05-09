#include <UbidotsEsp32Mqtt.h>
#include <PubSubClient.h>
#include <PulseSensorPlayground.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "DHT.h"
#define DHTPIN 26           
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

int getHW827(void);
int Redled_pin = 2;

const char *UBIDOTS_TOKEN = "BBUS-X2xCoaDM0JLFZhLw7rkcKcUvdp5olV";
const char *WIFI_SSID = "ankan";      
const char *WIFI_PASS = "ankan1234";     
const char *DEVICE_LABEL = "esp32";
const char *VARIABLE_LABEL_1 = "dht11"; 
const char *VARIABLE_LABEL_2 = "humidity";
const char *VARIABLE_LABEL_3 = "sensor";
const char *VARIABLE_LABEL_4 = "hw-827";
const char *VARIABLE_LABEL_5 = "bpm";
WebServer server(80);
//----------------
#define VARIABLE_LABEL "sensor" // Assing the variable label
#define DEVICE_LABEL "esp32" // Assig the device label
 

#define SENSOR A0 // Set the A0 as ecg SENSOR
const int VibrationsensorPin = 4; 

char payload[100];
char topic[150];
// Space to store values to send
char str_sensor[10];
 //----------
const int PULSE_SENSOR_PIN = 32;
PulseSensorPlayground pulseSensor;  // Create PulseSensorPlayground object
 //-------------
const int PUBLISH_FREQUENCY = 5000; 
unsigned long timer; 
#define MQTT_CLIENT_NAME "alexn25ton"
Ubidots ubidots(UBIDOTS_TOKEN);

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

//---------
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  Serial.write(payload, length);
  Serial.println(topic);
//---------

}

//webpage 

void handleRoot() {
  char msg[1500];
  snprintf(msg, 1500,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='4'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>\
    <title>Health Monitoring Server</title>\
    <style>\
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\
    h2 { font-size: 3.0rem; }\
    p { font-size: 3.0rem; }\
    .units { font-size: 1.2rem; }\
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}\
    </style>\
  </head>\
  <body>\
      <h2>Health Monitoring Server</h2>\
      <p>\
        <i class='fas fa-thermometer-half' style='color:#ca3517;'></i>\
        <span class='dht-labels'>Temperature</span>\
        <span>%.2f</span>\
        <sup class='units'>&deg;C</sup>\
      </p>\
      <p>\
        <i class='fas fa-tint' style='color:#00add6;'></i>\
        <span class='dht-labels'>Humidity</span>\
        <span>%.2f</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
      <p>\
       </p>\
       <p>\
        <i class='fas fa-heart' style='color:#d4175c;'></i>\
        <span class='bpm-labels'>BPM:</span>\
        <span>%d</span>\
      </p>\
      <p>\
        <i class='fas fa-odnoklassniki' style='color:#d4175c;'></i>\
        <span class='hw827-labels'>Oxygen Labels</span>\
        <span>%d</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
      <p>\
      <span class='motion'>Motion Status:</span>\
      <span>Patient Motion Detected under 10cm.....</span>\
       <span>%c</span>\
      </p>\
        <p>\
        <a href='http://nwuproject.byethost5.com/'>Rum Ecg</a>\
      </p>\
  </body>\
</html>",
           readDHTTemperature(), readDHTHumidity(), calculateBPM(), getHW827(), getVibration()
          );
  server.send(200, "text/html", msg);
}
// end page


void setup()
{
  Serial.begin(115200);    
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
  timer = millis();

  delay(1000);
  checkWiFi();
 // Configure Sensor
  pinMode(SENSOR, INPUT);
  dht.begin();
  pinMode(VibrationsensorPin, INPUT);  

  // Configure PulseSensor
  pulseSensor.analogInput(PULSE_SENSOR_PIN);
  pulseSensor.setThreshold(550);
  pulseSensor.begin() ;// Start of sensor

  // web
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.begin();

  pinMode(Redled_pin,OUTPUT);
  digitalWrite(Redled_pin, LOW);

}

void loop()
{
  server.handleClient();
  checkWiFi();
  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }
  if (millis() - timer) 
  {
    float h = readDHTHumidity();
    float t = readDHTTemperature();
    //ecg sensor 
    float sensor = analogRead(SENSOR); 

    // hw-827

    int getoxigen = getHW827();
    int bpm = calculateBPM();
    const char* motion = getVibration();
    ubidots.add(VARIABLE_LABEL_1, t);
    ubidots.add(VARIABLE_LABEL_2, h);
    ubidots.add(VARIABLE_LABEL_3, sensor);
    ubidots.add(VARIABLE_LABEL_4, getoxigen);
    ubidots.add(VARIABLE_LABEL_5, bpm);
    ubidots.publish(DEVICE_LABEL);
    // print in ide output
    Serial.println("Temperature: " + String(t));
    Serial.println("Humidity: " + String(h));
    Serial.println("Ecg: " + String(sensor));
    Serial.println("BPM: ");
    Serial.println(bpm);
    Serial.print("Oxigen: ");
    Serial.println(getoxigen);
     Serial.println( getVibration() );
          
    Serial.println("-----------------------------------------");
    timer = millis();

  }
  delay(1500); 
  ubidots.loop();
}

const char* getVibration(){
// vib
          if (digitalRead(VibrationsensorPin)){
          return "Patient Motion Detected under 10cm ";
          } 
        else{
              return "No Motion Detected within 10cm";                
            }
        // end vib
}

int getHW827(){
  int oxigen = pulseSensor.getBeatsPerMinute();
  if (pulseSensor.sawStartOfBeat()) 
  {
    return oxigen = oxigen.currentValue;
  }else {
  return -100;
  }
}


float readDHTTemperature() {
  float t = dht.readTemperature();
    return t;
}
float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds
  float h = dht.readHumidity();
    return h;
}

int calculateBPM () 
{  
    pulseValue = analogRead(pulsePin);

  // Detect the pulse
  if (pulseValue > 600) {
    digitalWrite(ledPin, HIGH); // turn on the LED
    delay(100); // wait for a short time
    digitalWrite(ledPin, LOW); // turn off the LED
    bpm = 60000 / pulseValue; // calculate the heart rate in beats per minute
    Serial.print("Heart rate: ");
    Serial.print(bpm);
    Serial.println(" BPM");

    srand(time(NULL));
    return bpm ;
  }
void checkWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(Redled_pin,HIGH);
    Serial.print("WIFI NOT CONNECTED.");
    if (millis() > 10000) ESP.restart();
  }
digitalWrite(Redled_pin,LOW);
}
