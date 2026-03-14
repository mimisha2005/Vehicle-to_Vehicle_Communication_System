#include <ESP8266WiFi.h>
#include <espnow.h>

#define TRIG_PIN D5
#define ECHO_PIN D6
#define LED_PIN D7
#define BUZZER_PIN D2

#define SAFE_DISTANCE 4

uint8_t peerAddress[] = {0x10, 0x06, 0x1C, 0x68, 0x43, 0x48};

typedef struct message {
  bool collisionAlert;
} message;

message outgoingData;
bool collisionState = false;

long getDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if(duration == 0) return -1;

  return duration * 0.034 / 2;
}

void onReceive(uint8_t *mac, uint8_t *incomingData, uint8_t len) {

  message incoming;
  memcpy(&incoming, incomingData, sizeof(incoming));

  if(incoming.collisionAlert){
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1500);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void setup() {

  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onReceive);
  esp_now_add_peer(peerAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  Serial.println("ESP8266 Ready");
}

void loop() {

  long distance = getDistance();

  Serial.print("ESP8266 Distance: ");
  Serial.println(distance);

  if(distance > 0 && distance <= SAFE_DISTANCE){

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    if(!collisionState){
      outgoingData.collisionAlert = true;
      esp_now_send(peerAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));
      collisionState = true;
    }
  }
  else{
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    collisionState = false;
  }

  delay(150);
}
