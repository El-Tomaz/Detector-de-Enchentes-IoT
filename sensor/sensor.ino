#include <freertos/freertos.h>
#include <freertos/task.h>

#include <Wire.h>
#include "HT_SSD1306Wire.h"

#include "WiFi.h"
#include "PubSubClient.h"

//Conectivity
const char *SSID = "FTTH - BIANCA";
const char *PSWD = "Bighouse05";

const char *MQTTBROKER = "test.mosquitto.org";
const char *DEVICE_ID = "b314-5";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Topic: ");
  Serial.print(topic);
  Serial.print("\tmessagem: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = DEVICE_ID;
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


//HARDWARE MAPPING
#define pTRIG 5
#define pECHO 18

#define pBUZZ 17
#define pLED 2

//OTHER STUFF
#define SOUND_SPEED 0.034
#define MAX_DIST_ALERT 12

//state variables
float distanceCm = 0;

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

static TaskHandle_t blinkHandler = NULL;

void mqttTask(void *p) {
  unsigned long lastTime = millis();
  while (true) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();


    client.publish(DEVICE_ID, String(distanceCm).c_str());
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void readSensor(void *p) {
  while (1) {
    // Clears the trigPin
    digitalWrite(pTRIG, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(pTRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(pTRIG, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    long duration = pulseIn(pECHO, HIGH);

    // Calculate the distance
    distanceCm = duration * SOUND_SPEED / 2;
 //   Serial.println(distanceCm);
    display.clear();
    display.drawStringMaxWidth(0, 0, 128,
                               String(distanceCm));

    display.display();


    if (distanceCm < MAX_DIST_ALERT) {
      xTaskNotifyGive(blinkHandler);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void blink(void *p) {
  while (true) {
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000))) {
      digitalWrite(pLED, HIGH);
      digitalWrite(pBUZZ, HIGH);
      
      vTaskDelay(100 / portTICK_PERIOD_MS);
      
      digitalWrite(pLED, LOW);
      digitalWrite(pBUZZ, LOW);
      
      vTaskDelay((distanceCm - 1) * 100 / portTICK_PERIOD_MS);
    }
  }
}




void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(100);

  display.init();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected!");

  randomSeed(micros());

  client.setServer(MQTTBROKER, 1883);
  client.setCallback(&mqtt_callback);

  pinMode(pBUZZ, OUTPUT);
  pinMode(pLED, OUTPUT);

  pinMode(pTRIG, OUTPUT);
  pinMode(pECHO, INPUT);

  xTaskCreate(&readSensor, "read sensor", 2048, NULL, 3, NULL);
  xTaskCreate(&blink, "alert", 2048, NULL, 2, &blinkHandler);
  xTaskCreate(&mqttTask, "mqtt", 4096, NULL, 4, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
}
