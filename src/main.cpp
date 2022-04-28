// #include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "time.h"

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

const int HTTP_PORT = 80;
const String HTTP_METHOD = "GET";      // or "POST"
const char HOST_NAME[] = "google.com"; // hostname of web server:
const String PATH_NAME = "";

struct color
{
  int r;
  int g;
  int b;
};

color busy = {205, 50, 50};
color freetime = {50, 205, 50};
color part = {255, 191, 0};
color out = {0, 128, 255};

color workday[NUMPIXELS];

int fadeAmount = 15;
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
}

void get()
{
  HTTPClient http;

  String serverPath = "https://www.google.co.uk/";

  // Your Domain name with URL path or IP address with path
  http.begin(serverPath.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
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

void setWorkDay()
{
  workday[0] = freetime;
  workday[1] = freetime;
  workday[2] = freetime;
  workday[3] = freetime;
  workday[4] = freetime;
  workday[5] = out;
  workday[6] = out;
  workday[7] = out;
  workday[8] = freetime;
  workday[9] = freetime;
  workday[10] = freetime;
  workday[11] = freetime;
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

void fadeCurrentTime()
{
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeHour[3];
  strftime(timeHour, 3, "%I", &timeinfo);
  int hour = String(timeHour).toInt() - 1;

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



void setup()
{
  Serial.begin(115200);

  pixels.setBrightness(25);
  pixels.begin();
  setAllPixels(out);

  setWorkDay();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);

  // tft.drawString("LeftButton:", tft.width() / 2, tft.height() / 2 - 16);

  initWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // get();
}

void loop()
{

  for (int i = 0; i < NUMPIXELS; i++)
  {
    color col = workday[i];
    pixels.setPixelColor(i, pixels.Color(col.r, col.g, col.b));
  }

  pixels.setPixelColor(2, pixels.Color(busy.r, busy.g, busy.b));
  pixels.setPixelColor(3, pixels.Color(part.r, part.g, part.b));

  pixels.show();

  fadeCurrentTime();

  tft.setCursor(0, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  // We can now plot text on screen using the "print" class
  tft.println("Hello World!");
}
