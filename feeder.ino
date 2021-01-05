#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "HX711.h"
#include <Servo.h>
#include <Arduino_JSON.h>

#define DVT  D1
#define CLK  D2
#define DVT2  D5
#define CLK2  D6
#define RED  D0
#define GREEN  D8
#define BLUE  D3



float calibration_factor = -500; // CALIBRATION FACTOR
float calibration_factor2 = 500; // CALIBRATION FACTOR
const char* ssid = "6. Kat";
const char* password = "xxxxxxxxxxx";
String url = "http://tranquil-oasis-23659.herokuapp.com";


Servo servoMotor;  // ADDING SERVO
HX711 depo, kap; // ADDING LOAD MODULES

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  Serial.begin(9600);
  delay(10);

  depo.begin(DVT, CLK); // DEPO PINS
  kap.begin(DVT2, CLK2); // KAP PINS
  servoMotor.attach(D4); // SERVO START
  long zero_factor = 89920; //Get a baseline reading
  long zero_factor2 = -105000; //Get a baseline reading
  depo.set_offset(zero_factor); // SET ZERO POINT OF DEPO
  kap.set_offset(zero_factor2); // SET ZERO POINT OF KAP

  Serial.println("Connecting to ");
  Serial.println(ssid);

  servoMotor.write(0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(RED, HIGH);
    delay(250);
    digitalWrite(RED, LOW);
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println(ssid);
  Serial.println("WiFi connected");

}

void loop() {
  depo.set_scale(calibration_factor); //Adjust to this calibration factor
  kap.set_scale(calibration_factor2); //Adjust to this calibration factor

  if (WiFi.status() == WL_CONNECTED) {//Check WiFi connection status
    HTTPClient http;

    String path = url + "/stock";

    http.begin(path.c_str());

    int httpResponseCode = http.GET();

    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      digitalWrite(GREEN, HIGH);
    } else {
      digitalWrite(RED, HIGH);
    }
    String payload = http.getString();
    boolean stop = 1;
    JSONVar data = JSON.parse(payload);


    JSONVar isOk = data["add"];
    JSONVar much = data["willadd"];

    JSONVar control = true;

    if (isOk == control) {
      while (stop) {
        double howmuch = much;

        digitalWrite(BLUE, HIGH);
        if (howmuch < kap.get_units()) {

          stop = 0;
          servoMotor.write(0);

          String path = url + "/refresh";
          http.begin(path);
          http.addHeader("Content-Type", "application/json");
          String httpRequestData = "{\"container\":" + String(kap.get_units()) + ",\"stock\":" + String(depo.get_units()) + ",\"add\":false,\"willadd\":0}";
          int httpResponseCode = http.POST(httpRequestData);
          Serial.println(httpResponseCode);

          // Free resources
          http.end();
          digitalWrite(BLUE, LOW);

        } else {
          servoMotor.write(15);
        }

      }
    } else {
      servoMotor.write(0);
    }

    delay(500);
  }

  Serial.print("depo ");
  Serial.println(depo.get_units());

  Serial.print("kap ");
  Serial.println(kap.get_units());

  HTTPClient http;
  String path = url + "/refresh";
  http.begin(path);
  http.addHeader("Content-Type", "application/json");
  String httpRequestData = "{\"container\":" + String(kap.get_units()) + ",\"stock\":" + String(depo.get_units()) + ",\"add\":false,\"willadd\":0}";
  int httpResponseCode = http.POST(httpRequestData);
  Serial.println(httpResponseCode);

  digitalWrite(GREEN, LOW);
  // Free resources
  http.end();
  delay(3000);    //Send a request every 30 seconds

}
