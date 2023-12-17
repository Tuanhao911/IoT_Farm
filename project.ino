#include <WiFi.h>
#include <DHT.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <AdafruitIO.h>
#include <AdafruitIO_WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#define WLAN_SSID       "..."
#define WLAN_PASS       "16112002"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "MTPQ_BKU"
#define AIO_KEY         "aio_TKoC97qg47bA6zFuYjQv5aZekhwb"

WiFiClient client;

//real time
const char *ntpServer = "pool.ntp.org";
int currHour = 0;
// Giờ khu vực của bạn (UTC offset)
const long utcOffsetInSeconds = 7 * 3600;  // Ví dụ: UTC+7

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds);


#define DHTPIN 16
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int Sensor_pin = 36;
int Sensor_value;
int SensorSoil_temp;
bool LEDMan = false;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish Humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish SoilMoisture = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/soil-moisture");

float temp; //to store the temperature value
float hum; // to store the humidity value
float soil; //to store the Soil Moisture value

#define LED_PIN 14
#define LED_PIN2 12
#define PUMP_PIN 5

AdafruitIO_WiFi io(AIO_USERNAME, AIO_KEY, WLAN_SSID, WLAN_PASS);
AdafruitIO_Feed *led = io.feed("Led");
AdafruitIO_Feed *led2 = io.feed("Led2");
AdafruitIO_Feed *pump = io.feed("pump1");

void setup() {
  Serial.begin(115200);
  pinMode(DHTPIN, OUTPUT);
  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  //real time
  timeClient.begin();

  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());

  connect();
  
  led->get();
  led->onMessage(handleMessage);
  
  led2->get();
  led2->onMessage(handleMessage2);

  pump->get();
  pump->onMessage(handleMessagePump);
}

void connect() {
  Serial.print(F("Connecting to Adafruit IO... "));
  int8_t ret;
  while ((ret = mqtt.connect()) != 0) {
    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(10000);
  }
  Serial.println(F("Adafruit IO Connected!"));
}

void loop() {
  if(!mqtt.ping(3)) {
    if(!mqtt.connected())
      connect();
  }

  //real time
  timeClient.update();
  currHour = hour();
  // Lấy thông tin thời gian
  Serial.print("Current time: ");
  Serial.println(timeClient.getFormattedTime());
  // Bật đèn từ 18h đến 0h và từ 0h đến 6h
  if (currHour == 18) {
    LEDMan = true;  // Bật đèn
  } else if (currHour == 5) {
    LEDMan = false; // Tắt đèn
  }

  temp = dht.readTemperature();
  hum = dht.readHumidity();

  Sensor_value = (100 - ((analogRead(Sensor_pin) / 1023.00) * 100));
  if (Sensor_value == 100) Sensor_value = 0;
  if (Sensor_value != 0) SensorSoil_temp= Sensor_value; 
  Sensor_value = SensorSoil_temp;

  Serial.print("temperature = ");
  Serial.println(temp);
  Serial.print("humidity = ");
  Serial.println(hum);

  Serial.print("Soil Moisture is  = ");
  Serial.println(Sensor_value);
  Serial.println("%");

  delay(30000);

  if (!SoilMoisture.publish(Sensor_value)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent soil!"));
  }

  if (!Temperature.publish(temp)) {
    Serial.println(F("Failed"));
  }
  
  if (!Humidity.publish(hum)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }

  io.run();
}

void handleMessage(AdafruitIO_Data *data) {
  Serial.print("received <- ");
 
  if(data->toPinLevel() == HIGH || currHour){
    Serial.println("HIGH");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("LOW");
    digitalWrite(LED_PIN, LOW);
  }
}

void handleMessage2(AdafruitIO_Data *data) {
  Serial.print("received 2222 <- ");
 
  if(data->toPinLevel() == HIGH){
    Serial.println("HIGH");
    digitalWrite(LED_PIN2, HIGH);
  } else {
    Serial.println("LOW");
    digitalWrite(LED_PIN2, LOW);
  }
}

void handleMessagePump(AdafruitIO_Data *data) {
  Serial.print("received pump <- ");
 
  if(data->toPinLevel() == HIGH){
    Serial.println("HIGH");
    digitalWrite(PUMP_PIN, HIGH);
  } else {
    Serial.println("LOW");
    digitalWrite(PUMP_PIN, LOW);
  }
}