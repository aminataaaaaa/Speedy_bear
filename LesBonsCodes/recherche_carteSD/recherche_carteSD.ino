#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <ArduinoJson.h>

#define BUZZER_PIN 12 // Définir la broche utilisée pour le buzzer

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

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT); // Définir la broche du buzzer en mode sortie

  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }

  File file = SD.open("/trajet_robot.txt");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  float latitude = 47.8442964;
  float longitude = 1.9456307; 

  Serial.print("Searching for position (");
  Serial.print(latitude);
  Serial.print(", ");
  Serial.print(longitude);
  Serial.println(")");

  for (JsonVariant feature : doc["features"].as<JsonArray>()) {
    float lat = roundTo(feature["geometry"]["coordinates"][0][1], 4); // Arrondi à 4 chiffres après la virgule
    float lon = roundTo(feature["geometry"]["coordinates"][0][0], 4); // Arrondi à 4 chiffres après la virgule
    if (roundTo(latitude, 4) == lat && roundTo(longitude, 4) == lon) {
      Serial.print("Found position at (");
      Serial.print(lat);
      Serial.print(", ");
      Serial.print(lon);
      Serial.print("), Max Speed: ");
      String maxSpeedString = feature["properties"]["maxspeed"].as<String>();
      Serial.println(maxSpeedString);
      
      // Convertir la vitesse limite en float
      float maxSpeed = maxSpeedString.toFloat();

      // Récupérer la vitesse fournie (simulée ici)
      float providedSpeed = 20.0; // Exemple de vitesse fournie (en km/h)

      // Comparer la vitesse fournie avec la vitesse limite
      if (providedSpeed > maxSpeed) {
        Serial.println("Provided speed exceeds speed limit!");
        playTone(1000, 10000); // Activer le buzzer
      } else {
        Serial.println("Provided speed is within speed limit.");
        digitalWrite(BUZZER_PIN, LOW); // Désactiver le buzzer
      }

      break;
    }
  }

  file.close();
}

void loop() {}
