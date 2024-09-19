#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Wire.h>
#include "heltec.h"
#include "HT_SSD1306Wire.h"

#include "WiFi.h"
#include "PubSubClient.h"


//LoRa
#include "LoRaWan_APP.h"

#define RF_FREQUENCY 915000000  // Hz

#define TX_OUTPUT_POWER 5  // dBm

#define LORA_BANDWIDTH 0         // [0: 125 kHz, \
                                 //  1: 250 kHz, \
                                 //  2: 500 kHz, \
                                 //  3: Reserved]
#define LORA_SPREADING_FACTOR 7  // [SF7..SF12]
#define LORA_CODINGRATE 1        // [1: 4/5, \
                                 //  2: 4/6, \
                                 //  3: 4/7, \
                                 //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8   // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0    // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false


#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 30  // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;

bool lora_idle = true;

static RadioEvents_t RadioEvents;
void OnTxDone(void) {
  Serial.println("LoRa message send");
  lora_idle = true;
}
void OnTxTimeout(void) {
  Radio.Sleep();
  Serial.println("LoRa TX timed out!");
  lora_idle = true;
}


//Conectivity
const char *SSID = "CLARO_2GF0F7FE";
const char *PSWD = "38F0F7FE";

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
#define pTRIG 13
#define pECHO 12

#define pBUZZ 17
#define pLED 2

//OTHER STUFF
#define SOUND_SPEED 0.034
#define MAX_DIST_ALERT 12

//state variables
float distanceCm = 0;

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

static TaskHandle_t blinkHandler = NULL;

void LoRaTask(void *p) {
  while (1) {
    if (lora_idle) {
      delay(1000);

      sprintf(txpacket, "%0.2f", distanceCm);
      Radio.Send((uint8_t *)txpacket, strlen(txpacket));
      lora_idle = false;
    }
    Radio.IrqProcess();
  }
}

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
    display.clear();
    display.drawStringMaxWidth(0, 0, 128,
                               //   Serial.println(distanceCm);
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


  //Configuring LoRa vvvv
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);


  pinMode(pBUZZ, OUTPUT);
  pinMode(pLED, OUTPUT);

  pinMode(pTRIG, OUTPUT);
  pinMode(pECHO, INPUT);



  xTaskCreate(&readSensor, "read sensor", 2048, NULL, 3, NULL);
  xTaskCreate(&blink, "alert", 2048, NULL, 2, &blinkHandler);
  //xTaskCreate(&mqttTask, "mqtt", 4096, NULL, 4, NULL);
  xTaskCreate(&LoRaTask, "lora send", 4096, NULL, 5, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
}
