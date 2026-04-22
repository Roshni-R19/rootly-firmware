#include "Particle.h"
#include "../lib/neopixel/src/neopixel.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ST7735.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

#define PIXEL_COUNT 12
#define PIXEL_PIN   SPI1
#define PIXEL_TYPE  WS2812
Adafruit_NeoPixel ring(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

#define TFT_CS  S3
#define TFT_DC  A1
#define TFT_RST A2
Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_GREEN   0x07E0
#define COLOR_YELLOW  0xFFE0
#define COLOR_RED     0xF800
#define COLOR_BLUE    0x059F
#define COLOR_ORANGE  0xFD20
#define COLOR_SOFTBG  0x2126

enum PlantState {
    STATE_HAPPY = 0,
    STATE_SAD = 1,
    STATE_THIRSTY = 2,
    STATE_HOT = 3
};

struct StateStyle {
    const char *label;
    uint16_t accent;
    uint8_t ringR;
    uint8_t ringG;
    uint8_t ringB;
};

const StateStyle STYLES[] = {
    {"HAPPY", COLOR_GREEN, 0, 255, 0},
    {"SAD", COLOR_ORANGE, 255, 170, 0},
    {"THIRSTY", COLOR_BLUE, 0, 170, 255},
    {"HOT", COLOR_RED, 255, 0, 0},
};

int plantState = STATE_HAPPY;

static const uint8_t FACE_HAPPY[] = {
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x03,0x80,0x03,0x80,
    0x07,0xC0,0x07,0xC0,
    0x03,0x80,0x03,0x80,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x70,0x0E,0x00,
    0x00,0x1C,0x38,0x00,
    0x00,0x07,0xE0,0x00,
    0x00,0x01,0x80,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
};

static const uint8_t FACE_SAD[] = {
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x03,0x80,0x03,0x80,
    0x07,0xC0,0x07,0xC0,
    0x03,0x80,0x03,0x80,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x01,0x80,0x00,
    0x00,0x07,0xE0,0x00,
    0x00,0x1C,0x38,0x00,
    0x00,0x70,0x0E,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
};

static const uint8_t FACE_THIRSTY[] = {
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x01,0xE0,0x01,0xE0,
    0x03,0x10,0x03,0x10,
    0x00,0xE0,0x00,0xE0,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x7F,0xFE,0x00,
    0x00,0x40,0x02,0x00,
    0x00,0x3F,0xFC,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
};

static const uint8_t FACE_HOT[] = {
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x03,0x30,0x0C,0xC0,
    0x01,0xE0,0x07,0x80,
    0x03,0x30,0x0C,0xC0,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x0F,0xF0,0x00,
    0x00,0x0F,0xF0,0x00,
    0x00,0x03,0xC0,0x00,
    0x00,0x03,0xC0,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
};

void setRingColor(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < ring.numPixels(); i++) {
        ring.setPixelColor(i, r, g, b);
    }
    ring.setBrightness(60);
    ring.show();
}

void fillRoundedPanel(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color) {
    display.fillRoundRect(x, y, w, h, radius, color);
}

void drawScaledBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, int16_t scale, uint16_t color) {
    int bytesPerRow = (w + 7) / 8;

    for (int16_t row = 0; row < h; row++) {
        for (int16_t col = 0; col < w; col++) {
            int byteIndex = row * bytesPerRow + (col / 8);
            uint8_t bitMask = 0x80 >> (col % 8);
            if (bitmap[byteIndex] & bitMask) {
                display.fillRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

const uint8_t *bitmapForState(int state) {
    switch (state) {
    case STATE_HAPPY:
        return FACE_HAPPY;
    case STATE_SAD:
        return FACE_SAD;
    case STATE_THIRSTY:
        return FACE_THIRSTY;
    case STATE_HOT:
    default:
        return FACE_HOT;
    }
}

void drawFacePanel(int state) {
    const StateStyle &style = STYLES[state];
    const int scale = 3;

    display.fillScreen(COLOR_SOFTBG);
    fillRoundedPanel(10, 10, 108, 74, 12, COLOR_BLACK);
    drawScaledBitmap(16, 20, bitmapForState(state), 32, 16, scale, COLOR_WHITE);

    display.fillCircle(26, 58, 4, COLOR_ORANGE);
    display.fillCircle(102, 58, 4, COLOR_ORANGE);

    if (state == STATE_THIRSTY) {
        display.fillTriangle(61, 61, 69, 61, 65, 76, COLOR_BLUE);
    }
    if (state == STATE_HOT) {
        display.fillCircle(102, 18, 4, COLOR_WHITE);
        display.fillTriangle(102, 22, 98, 31, 106, 31, COLOR_WHITE);
    }

    int16_t x1, y1;
    uint16_t w, h;
    display.setTextSize(2);
    display.getTextBounds(style.label, 0, 0, &x1, &y1, &w, &h);
    display.setTextColor(style.accent);
    display.setCursor((128 - w) / 2, 98);
    display.print(style.label);
}

void applyState(int state) {
    const StateStyle &style = STYLES[state];
    plantState = state;
    setRingColor(style.ringR, style.ringG, style.ringB);
    drawFacePanel(state);
    Log.info("State set to %s", style.label);
}

int parseState(String command) {
    command.trim();
    command.toLowerCase();

    if (command == "0" || command == "happy" || command == "wet" || command == "healthy" || command == "good") {
        return STATE_HAPPY;
    }
    if (command == "1" || command == "sad" || command == "low" || command == "droopy") {
        return STATE_SAD;
    }
    if (command == "2" || command == "thirsty" || command == "dry" || command == "water") {
        return STATE_THIRSTY;
    }
    if (command == "3" || command == "hot" || command == "temp" || command == "warm") {
        return STATE_HOT;
    }
    return -1;
}

int setPlantMood(String command) {
    int nextState = parseState(command);
    if (nextState < 0) {
        Log.warn("Unknown command: %s", command.c_str());
        return -1;
    }

    applyState(nextState);
    return plantState;
}

void showBootScreen() {
    display.fillScreen(COLOR_SOFTBG);
    fillRoundedPanel(12, 18, 104, 48, 10, COLOR_BLACK);
    display.setTextColor(COLOR_WHITE);
    display.setTextSize(1);
    display.setCursor(24, 32);
    display.print("Cloud demo ready");
    display.setCursor(16, 48);
    display.print("Use: plantMood");
    display.setTextColor(COLOR_GREEN);
    display.setCursor(28, 86);
    display.print("happy   sad");
    display.setTextColor(COLOR_BLUE);
    display.setCursor(12, 100);
    display.print("thirsty   hot");
}

void setup() {
    waitFor(Serial.isConnected, 5000);
    Log.info("Booting Plant Care Companion cloud demo");

    ring.begin();
    setRingColor(0, 0, 40);

    display.initR(INITR_GREENTAB);
    display.setRotation(1);
    showBootScreen();

    Particle.function("plantMood", setPlantMood);
    Particle.variable("plantState", plantState);

    delay(1200);
    applyState(STATE_HAPPY);
}

void loop() {
}

/*
// Soil sensor version kept here for reference.
// This code path was tested and backed up separately in:
// src/plant_project_sensor_debug_backup.txt
//
// #include "Adafruit_Seesaw.h"
// Adafruit_Seesaw soilSensor;
// bool soilSensorFound = false;
// bool soilReadError = false;
//
// Wire.setSpeed(CLOCK_SPEED_100KHZ);
// Wire.begin();
// if (!soilSensor.begin(0x36)) {
//     Log.error("Soil sensor not found");
// }
// uint16_t moisture = soilSensor.touchRead(0);
// float tempC = soilSensor.getTemp();
*/
