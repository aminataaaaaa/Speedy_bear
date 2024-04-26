#include <Adafruit_GPS.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "Audio.h"

#define RXD2 16
#define TXD2 17
#define GPSSerial Serial2
#define GPSECHO false
#define BUZZER_PIN 12
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26
#define BUTTON_INC_VOLUME 14
#define BUTTON_DEC_VOLUME 13

uint32_t timer = millis();
Adafruit_GPS GPS(&GPSSerial);
Audio audio;

void setup() {
  Serial.begin(115200);
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // Mise à jour du GPS toutes les 1seconde
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(5000);
  GPSSerial.println(PMTK_Q_RELEASE);
  pinMode(BUZZER_PIN, OUTPUT); // Définir la broche du buzzer en mode sortie

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SD.begin(SD_CS);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21); // Initial volume
  audio.connecttoFS(SD, "/Histoire.mp3");
  pinMode(BUTTON_INC_VOLUME, INPUT_PULLUP);
  pinMode(BUTTON_DEC_VOLUME, INPUT_PULLUP);
}

void loop() {
  char c = GPS.read();
  if (GPSECHO && c) {
    Serial.print(c);
  }

  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) {
      return;
    }
  }

  if (millis() - timer > 2000) {
    timer = millis();

    if (GPS.fix) {
      float latitude = GPS.latitudeDegrees;
      float longitude = GPS.longitudeDegrees;

      Serial.print("\nLocation: ");
      Serial.print(latitude, 4);
      Serial.print(", ");
      Serial.println(longitude, 4);

      File file = SD.open("/trajet1.txt");
      if (file) {
        size_t size = file.size();
        std::unique_ptr<char[]> buf(new char[size]);
        file.readBytes(buf.get(), size);

        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, buf.get());
        if (!error) {
          Serial.print("Searching for position (");
          Serial.print(latitude, 4);
          Serial.print(", ");
          Serial.print(longitude, 4);
          Serial.println(")");

          for (JsonVariant feature : doc["features"].as<JsonArray>()) {
            float lat = roundTo(feature["geometry"]["coordinates"][0][1], 3);
            float lon = roundTo(feature["geometry"]["coordinates"][0][0], 3);
            if (roundTo(latitude, 3) == lat && roundTo(longitude, 3) == lon) {
              Serial.print("Found position at (");
              Serial.print(lat);
              Serial.print(", ");
              Serial.print(lon);
              Serial.print("), Max Speed: ");
              String maxSpeedString = feature["properties"]["maxspeed"].as<String>();
              Serial.println(maxSpeedString);
              
              // Convertir la vitesse limite en float
              float maxSpeed = maxSpeedString.toFloat();

              // Comparer la vitesse limite avec la vitesse GPS
              float currentSpeed = GPS.speed;
              if (currentSpeed > maxSpeed) {
                Serial.println("Current speed exceeds speed limit!");
                digitalWrite(BUZZER_PIN, LOW); // Désactiver le buzzer
                audio.connecttoFS(SD, "/Histoire.mp3"); //revoir avec audio.play()
              } else {
                Serial.println("Current speed is within speed limit.");
                digitalWrite(BUZZER_PIN, HIGH); // Activer le buzzer
                audio.stopSong(); // Arrêter la lecture audio
              }

              break;
            }
          }
        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.f_str());
        }
        file.close();
      } else {
        Serial.println("Failed to open file for reading");
      }
    } else {
      Serial.println("No GPS Fix");
    }
  }

  audio.loop(); // Gérer la lecture audio

  if (digitalRead(BUTTON_INC_VOLUME) == LOW) {
    increaseVolume();
    delay(200); // Debounce delay
  }

  if (digitalRead(BUTTON_DEC_VOLUME) == LOW) {
    decreaseVolume();
    delay(200); // Debounce delay
  }
}

// Fonction pour arrondir une valeur à un nombre spécifié de décimales
float roundTo(float value, int decimalPlaces) {
  float multiplier = pow(10.0, decimalPlaces);
  return round(value * multiplier) / multiplier;
}

// Fonction pour générer une tonalité
void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency); // Activer le buzzer à la fréquence spécifiée
  delay(duration); // Jouer la tonalité pendant la durée spécifiée
  noTone(BUZZER_PIN); // Désactiver le buzzer
  delay(1000); // Attendre 1 seconde entre les tonalités
}

void increaseVolume() {
  int currentVolume = audio.getVolume();
  if (currentVolume < 21) { // Volume maximum
    audio.setVolume(currentVolume + 1);
    Serial.println("Volume increased");
  }
}

void decreaseVolume() {
  int currentVolume = audio.getVolume();
  if (currentVolume > 0) { // Volume minimum
    audio.setVolume(currentVolume - 1);
    Serial.println("Volume decreased");
  }
}
