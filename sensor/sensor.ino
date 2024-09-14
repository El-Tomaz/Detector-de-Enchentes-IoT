#include <freertos/freertos.h>
#include <freertos/task.h>

#include <Wire.h>
#include "HT_SSD1306Wire.h"


//HARDWARE MAPPING
#define pTRIG 5
#define pECHO 18

#define pBUZZ 17
#define pLED 2

//OTHER STUFF
#define SOUND_SPEED 0.034


//state variables
bool alert = false;
float distanceCm = 0;

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst


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
    Serial.println(distanceCm);
    display.clear();
    display.drawStringMaxWidth(0, 0, 128,
                               String(distanceCm));

    display.display();


    if (distanceCm < 6) {
      alert = true;
    } else {
      alert = false;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void blink(void *p) {
  while (true) {

    if (alert) {
      digitalWrite(pLED, HIGH);
      digitalWrite(pBUZZ, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(pLED, LOW);
      digitalWrite(pBUZZ, LOW);
      vTaskDelay((distanceCm - 1) * 100 / portTICK_PERIOD_MS);
    } else {

      vTaskDelay(10 / portTICK_PERIOD_MS);
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




  pinMode(pBUZZ, OUTPUT);
  pinMode(pLED, OUTPUT);

  pinMode(pTRIG, OUTPUT);
  pinMode(pECHO, INPUT);

  xTaskCreate(&readSensor, "read sensor", 2048, NULL, 3, NULL);

  xTaskCreate(&blink, "alert", 2048, NULL, 2, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
}
