// https://gist.github.com/igrr/7f7e7973366fc01d6393
// http://esp8266.ru/forum/threads/ne-mogu-zagruzit-ip-iz-eeprom.704/ хоро описано, как настроить отправку разных данных.
// http://www.hivemq.com/blog/mqtt-client-library-encyclopedia-arduino-pubsubclient/
// https://home-assistant.io/blog/2015/10/11/measure-temperature-with-esp8266-and-report-to-mqtt/
// https://www.sparkfun.com/news/1842 как включить сон
// mosquitto_sub -t esp8266_arduino_out '/devices/arduino/esp8266_arduino_out/#' 
// mqtt-delete-retained удалить все топики

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "DHT.h"

#define DHTPIN D8     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11
#define DELAY_TIME 20000 // время между отправками

DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

const char* ssid = "";
const char* password = "";

const char* topic = "/devices/arduino/controls";
const char* temp_topic = "/devices/arduino/controls/dht_temperature";
const char* hum_topic = "/devices/arduino/controls/dht_humidity";
const char* sensor_name = "DHT";
const char* server = "192.168.1.29";



float humidity, temp_c;  // Values read from sensor
String webString = "";   // String to display
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

// проверка, что данные обновились. 
bool checkBound(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

float lastTemp = 0.0;
float lastHum = 0.0;
float diff = 1.0;

WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor
    previousMillis = currentMillis;

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_c = dht.readTemperature();     // Read temperature as Celsium
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_c)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Generate client name based on MAC address and last 8 bits of microsecond counter
  String clientName;
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);

  Serial.print("Connecting to ");
  Serial.print(server);
  Serial.print(" as ");
  Serial.println(clientName);

  if (client.connect((char*) clientName.c_str())) {
    Serial.println("Connected to MQTT broker");
    Serial.print("Topic is: ");
    Serial.println(topic);

    //if (client.publish(topic, "hello from ESP8266")) {
    ///  Serial.println("Publish ok");
    //}
    //else {
    //  Serial.println("Publish failed");
    //}
  }
  else {
    Serial.println("MQTT connect failed");
    Serial.println("Will reset and try again...");
    abort();
  }
}

void loop() {

  if (client.connected()) {
    gettemperature();
    Serial.println("Sending ... : ");
    Serial.println(diff); // Зависает почему-то после нескоьких раз передачи
    //Serial.print("Tempearture"); Serial.println(temp_c);
    //Serial.print("Humidity"); Serial.println(humidity);

    // Отправляем информацию о температуре в соответствующий топик
    //if (checkBound(temp_c, lastTemp, diff))  {
    lastTemp = temp_c;
    Serial.print("Temperature oC: ");
    Serial.println(String(temp_c).c_str());
    client.publish(temp_topic, String(temp_c).c_str(), true);
    if (client.publish(temp_topic, String(temp_c).c_str() )) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }
    //}
    // Отправляем информацию о влажности в соответствующий топик
    //if (checkBound(humidity, lastHum, diff))  {
    lastHum = humidity;
    Serial.print("Humidity %: ");
    Serial.println(String(humidity).c_str());
    //client.publish(hum_topic, String(humidity).c_str(), true);
    if (client.publish(hum_topic, String(humidity).c_str() )) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }
    //}

  }
  delay(DELAY_TIME);
}
