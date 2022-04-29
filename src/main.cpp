#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "time.h"
#include <ArduinoJson.h>

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke library, pins defined in User_Setup.h

#define TFT_GREY 0x5AEB // New colour

#define PIN 22
#define NUMPIXELS 12

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const char *ssid = "iPhone";
const char *password = "hellohello";

struct color
{
  int r;
  int g;
  int b;
  String name;
};

color busy = {255, 0, 0};
color freetime = {0, 255, 0};
color part = {255, 191, 0};
color out = {0, 0, 255};

color workday[NUMPIXELS];

int fadeAmount = 50;
int fadeDelay = 100;

void setAllPixels(color color)
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(color.r, color.g, color.b));
  }
  pixels.show();
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Connecting to WiFi ..", tft.width() / 2 - 50, tft.height() / 2);

  int progress = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');

    setAllPixels(out);

    pixels.setPixelColor(progress, pixels.Color(freetime.r, freetime.g, freetime.b));

    pixels.show();

    delay(1000);

    progress++;
    if (progress > 11)
      progress = 0;
  }
  Serial.println("Connected");
  Serial.println(WiFi.localIP());
  tft.fillScreen(TFT_BLACK);
}

void ReadWorkDay(String json)
{
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, json);

  for (int i = 0; i < doc["events"].size(); i++)
  {
    color statusColor;

    int status = doc["events"][i]["status"];
    int hour = doc["events"][i]["hour"];

    switch (status)
    {
    case 2:
      statusColor = freetime;
      if (hour == 5 || hour == 6 || hour == 7)
        statusColor = out;
      break;
    case 3:
      statusColor = part;
      break;
    case 4:
      statusColor = busy;
      break;
    default:
      statusColor = out;
      break;
    }

    String sensor = doc["events"][i]["meetingNames"];
    Serial.println(sensor);
    statusColor.name = sensor;

    workday[hour] = statusColor;
  }
}

void get()
{
  HTTPClient http;

  String serverPath = "https://theallknowingeye.azurewebsites.net/api/TheAllKnowingEye?code=XT1B9KLLC2hPERu8HFljr4x5L2Rv1g-9t-fDjYB7fvK5AzFuwsVRtQ==&Key=Payroc1!&name=Rory.Hamilton@Payroc.com";

  // Your Domain name with URL path or IP address with path
  http.begin(serverPath.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();

    ReadWorkDay(payload);

    Serial.println(payload);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}

// Returns the Red component of a 32-bit color
uint8_t Red(uint32_t color)
{
  return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color)
{
  return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color)
{
  return color & 0xFF;
}

void fadeCurrentTime(int hour)
{
  uint32_t colorInt = pixels.getPixelColor(hour);

  color current = {Red(colorInt), Green(colorInt), Blue(colorInt)};

  int r = current.r;
  int g = current.g;
  int b = current.b;

  for (int fade = 1; fade < fadeAmount; fade++)
  {
    r -= fade;
    g -= fade;
    b -= fade;
    pixels.setPixelColor(hour, pixels.Color(
                                   constrain(r, 0, current.r),
                                   constrain(g, 0, current.g),
                                   constrain(b, 0, current.b)));
    pixels.show();
    if (r <= 0 && g <= 0 && b <= 0)
      break;
    delay(fadeDelay);
  }

  for (int fade = 1; fade < fadeAmount; fade++)
  {
    r += fade;
    g += fade;
    b += fade;
    pixels.setPixelColor(hour, pixels.Color(
                                   constrain(r, 0, current.r),
                                   constrain(g, 0, current.g),
                                   constrain(b, 0, current.b)));
    pixels.show();
    delay(fadeDelay);
  }
}

void updateScreen(int hour)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_DARKGREEN);
  tft.setCursor(0, 0, 2);
  tft.println(workday[hour].name);
}

void setup()
{
  Serial.begin(115200);

  pixels.setBrightness(25);
  pixels.begin();
  setAllPixels(out);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  initWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  setAllPixels(out);

  get();
}

void loop()
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    color col = workday[i];
    pixels.setPixelColor(i, pixels.Color(col.r, col.g, col.b));
  }

  pixels.show();

  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeHour[3];
  strftime(timeHour, 3, "%I", &timeinfo);
  int hour = String(timeHour).toInt() - 1;

  updateScreen(hour);

  fadeCurrentTime(hour);

  delay(1000);
}
