#include "LoRaWan_APP.h"

//MQTT / WiFi vv
#include "WiFi.h"
#include "PubSubClient.h"

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



//LoRa config vv
#define RF_FREQUENCY 915000000  // Hz

#define TX_OUTPUT_POWER 14  // dBm

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

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi, rxSize;

//state variables
bool lora_idle = true;
float distance = 0;

//tasks

void receiveLora(void *p) {
  while (1) {
    if (lora_idle) {
      lora_idle = false;
      Serial.println("into RX mode");
      Radio.Rx(0);
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


    client.publish(DEVICE_ID, String(distance).c_str());

  }
}

void setup() {


  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected!");

  randomSeed(micros());

  client.setServer(MQTTBROKER, 1883);
  client.setCallback(&mqtt_callback);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);



  txNumber = 0;
  rssi = 0;

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  xTaskCreate(&receiveLora, "receive lora", 4096, NULL, 2, NULL);

  xTaskCreate(&mqttTask, "send data over mqtt", 4096, NULL, 1, NULL);
}



void loop() {
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  rssi = rssi;
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  Radio.Sleep();
  Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n", rxpacket, rssi, rxSize);
  distance = atof(rxpacket);
  lora_idle = true;
  delay(100);
}