#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <DHT.h>
#include <SPI.h>

// Pin definitions
#define TFT_CS   10
#define TFT_DC    8
#define TFT_RST   9
#define DHT_PIN   2
#define DHT_TYPE  DHT11

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Track previous values to avoid unnecessary redraws
float lastTemp    = -999;
float lastHum     = -999;
float lastHeatIdx = -999;
int   lastComfort = -1;   // 0=Comfortable 1=Warm 2=Uncomfortable 3=Dangerous

// Heat index formula (Rothfusz, Celsius)
// Only accurate above 27C and 40% RH — below that returns raw temp
float computeHeatIndex(float t, float h) {
  if (t < 27.0 || h < 40.0) return t;

  float hi = -8.78469475556
    + 1.61139411    * t
    + 2.338548839   * h
    - 0.14611605    * t * h
    - 0.01230809050 * t * t
    - 0.01642482777 * h * h
    + 0.00221732631 * t * t * h
    + 0.00072546    * t * h * h
    - 0.00000358582 * t * t * h * h;
  return hi;
}

// Comfort level: returns 0-3 based on heat index
int comfortLevel(float hi) {
  if (hi < 27.0) return 0;  // Comfortable
  if (hi < 32.0) return 1;  // Warm
  if (hi < 39.0) return 2;  // Uncomfortable
  return 3;                  // Dangerous
}

// Colour per comfort level
uint16_t comfortColor(int level) {
  switch (level) {
    case 0: return ST77XX_GREEN;
    case 1: return ST77XX_YELLOW;
    case 2: return 0xFD00;    // Orange in RGB565
    default: return ST77XX_RED;
  }
}

// Label per comfort level
const char* comfortLabel(int level) {
  switch (level) {
    case 0: return "Comfortable";
    case 1: return "Warm";
    case 2: return "Uncomfortable";
    default: return "Dangerous!";
  }
}

// Draw a small white section header
void drawLabel(int x, int y, const char* text) {
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(x, y);
  tft.print(text);
}

void setup() {
  tft.initR(INITR_BLACKTAB);  // try INITR_GREENTAB if colours look wrong
  tft.setRotation(2);          // portrait: 128x160
  tft.fillScreen(ST77XX_BLACK);
  dht.begin();

  // Static labels drawn once
  drawLabel(10, 5,   "Temperature");
  drawLabel(10, 58,  "Humidity");
  drawLabel(10, 111, "Feels like");
}

void loop() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  // Handle sensor failure
  if (isnan(temp) || isnan(hum)) {
    tft.fillRect(10, 20, 108, 20, ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(10, 20);
    tft.print("Sensor error");
    delay(2000);
    return;
  }

  float hi    = computeHeatIndex(temp, hum);
  int comfort = comfortLevel(hi);

  // Temperature
  if (temp != lastTemp) {
    tft.fillRect(10, 20, 108, 30, ST77XX_BLACK);
    tft.setTextColor(ST77XX_ORANGE);
    tft.setTextSize(2);
    tft.setCursor(10, 22);
    tft.print(temp, 1);
    tft.print(" C");
    lastTemp = temp;
  }

  // Humidity
  if (hum != lastHum) {
    tft.fillRect(10, 73, 108, 30, ST77XX_BLACK);
    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(2);
    tft.setCursor(10, 75);
    tft.print(hum, 1);
    tft.print(" %");
    lastHum = hum;
  }

  // Heat index
  if (hi != lastHeatIdx) {
    tft.fillRect(10, 125, 108, 20, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 126);
    tft.print(hi, 1);
    tft.print(" C");
    lastHeatIdx = hi;
  }

  // Comfort label
  if (comfort != lastComfort) {
    tft.fillRect(10, 148, 118, 10, ST77XX_BLACK);
    tft.setTextColor(comfortColor(comfort));
    tft.setTextSize(1);
    tft.setCursor(10, 149);
    tft.print(comfortLabel(comfort));
    lastComfort = comfort;
  }

  delay(2000);
}
