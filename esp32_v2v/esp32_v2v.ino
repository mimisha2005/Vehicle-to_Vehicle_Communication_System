#include <WiFi.h>
#include <esp_now.h>

/* ---------- PIN CONFIG ---------- */
#define TRIG_PIN    5
#define ECHO_PIN    18
#define LED_PIN     22
#define BUZZER_PIN  23

/* ---------- LOGIC ---------- */
#define WARNING_DISTANCE 4   // cm
#define DANGER_DISTANCE  2   // cm

/* ---------- PEER (ESP8266) MAC ---------- */
uint8_t peerAddress[] = {0xC4, 0x5B, 0xBE, 0xCB, 0x97, 0x57};

/* ---------- DATA STRUCT ---------- */
typedef struct message {
  bool danger;
} message;

/* ---------- STATE ---------- */
bool collisionState = false;
bool remoteDanger = false;
unsigned long dangerStartTime = 0;

/* ---------- ULTRASONIC ---------- */
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

/* ---------- ESP-NOW RECEIVE ---------- */
void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  message incoming;
  memcpy(&incoming, data, sizeof(incoming));
  if (incoming.danger) {
    remoteDanger = true;
    dangerStartTime = millis();
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("ESP32 Ready");
}

void loop() {
  long distance = getDistance();
  Serial.print("ESP32 Distance: ");
  Serial.println(distance);

  /* SAFE */
  if (distance == -1 || distance > WARNING_DISTANCE) {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    collisionState = false;
  }
  /* WARNING */
  else if (distance > DANGER_DISTANCE) {
    digitalWrite(LED_PIN, HIGH);
    noTone(BUZZER_PIN);
    collisionState = false;
  }
  /* DANGER */
  else {
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000);

    if (!collisionState) {
      message out;
      out.danger = true;
      esp_now_send(peerAddress, (uint8_t *)&out, sizeof(out));
      collisionState = true;
    }
  }

  /* REMOTE ALERT */
  if (remoteDanger) {
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000);

    if (millis() - dangerStartTime > 1500) {
      remoteDanger = false;
      noTone(BUZZER_PIN);
    }
  }

  delay(120);
}
