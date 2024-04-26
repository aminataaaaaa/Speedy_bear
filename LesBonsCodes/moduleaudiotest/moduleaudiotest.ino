#include "Audio.h"
#include "SD.h"
#include "FS.h"

#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define I2S_DOUT      25 //A1
#define I2S_LRC       26 //A2
#define I2S_BCLK      27 //BCLK


#define BUTTON_INC_VOLUME 14
#define BUTTON_DEC_VOLUME 13

Audio audio;

void setup() {
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    Serial.begin(115200);
    SD.begin(SD_CS);
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21); // Initial volume
    audio.connecttoFS(SD, "/Histoire.mp3");

    pinMode(BUTTON_INC_VOLUME, INPUT_PULLUP);
    pinMode(BUTTON_DEC_VOLUME, INPUT_PULLUP);
}

void loop() {
    audio.loop();

    if (digitalRead(BUTTON_INC_VOLUME) == LOW) {
        increaseVolume();
        delay(200); // Debounce delay
    }

    if (digitalRead(BUTTON_DEC_VOLUME) == LOW) {
        decreaseVolume();
        delay(200); // Debounce delay
    }
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
