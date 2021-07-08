/*******************************************************************
    Checks when a list of youtubers are live and prints to the serial

    If you want a version that prints to a display, check here: https://github.com/witnessmenow/D1-Mini-TFT-Shield

    Checks every 5 seconds if one of your channels being watched
    is live. 5 seconds later it will check the next one.

    NOTE 1: This is almost certainly against YouTube ToS, so use at your own risk
    (Although there is nothing in the request that identifies you, they may limit or ban
    your IP address)

    NOTE 2: This could potentially use a decent amount of data as it makes
    lots of requests. (every 5 seconds, all day, that's + 17k requests)

    Compatible Boards:
	  - Any ESP8266 board
	  - Any ESP32 board

    Parts:
    D1 Mini ESP8266 * - http://s.click.aliexpress.com/e/uzFUnIe
    ESP32 Mini Kit (ESP32 D1 Mini) * - https://s.click.aliexpress.com/e/_AYPehO (pick the CP2104 Drive version)

 *  * = Affiliate

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/
#define ARDUINOJSON_DECODE_UNICODE 1
// ----------------------------
// Standard Libraries
// ----------------------------

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <WiFiClientSecure.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <YouTubeLiveStream.h>
// Library for interacting with YouTube Livestreams

// Only available on Github
// https://github.com/witnessmenow/youtube-livestream-arduino


#include <ArduinoJson.h>
// Library used for parsing Json from the API responses

// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson


struct YTChannelDetails
{
  const char *id;
  const char *name;
  bool live;
};


//------- Replace the following! ------

char ssid[] = "SSID";         // your network SSID (name)
char password[] = "password"; // your network password

// Update this to match the number of channels you are following.
#define NUM_CHANNELS 10

// {"channel_ID", "name to appear", "current status"}
//
// Easiest way to get the channel ID is to click on a video,
// then click on their channel name from the video page,
// channel ID will be in the URL then.
//
// 16 (maybe 17) characters is the max that will fit for "name to appear"
//
// Current status should start as false, it will get updated if they are live.

YTChannelDetails channels[NUM_CHANNELS] = {
  {"UCezJOfu7OtqGzd5xrP3q6WA", "Brian Lough", false}, // https://www.youtube.com/channel/UCezJOfu7OtqGzd5xrP3q6WA
  {"UCQmACVkilzq39wH9WW3jmyA", "Lough & Load", false}, // https://www.youtube.com/channel/UCQmACVkilzq39wH9WW3jmyA
  {"UCUW49KGPezggFi0PGyDvcvg", "Zack Freedman", false}, // https://www.youtube.com/channel/UCUW49KGPezggFi0PGyDvcvg
  {"UCp_5PO66faM4dBFbFFBdPSQ", "Bitluni", false}, // https://www.youtube.com/channel/UCp_5PO66faM4dBFbFFBdPSQ
  {"UC8rQKO2XhPnvhnyV1eALa6g", "Bitluni's Trash", false}, // https://www.youtube.com/channel/UC8rQKO2XhPnvhnyV1eALa6g
  {"UCu94OHbBYVUXLYPh4NBu10w", "Unexpected Maker", false}, // https://www.youtube.com/channel/UCu94OHbBYVUXLYPh4NBu10w
  {"UCv7UOhZ2XuPwm9SN5oJsCjA", "Intermit.Tech", false}, // https://www.youtube.com/channel/UCv7UOhZ2XuPwm9SN5oJsCjA
  {"UCllpBTH26_dAl5tYl7vA1TQ", "Defpom", false}, // https://www.youtube.com/channel/UCllpBTH26_dAl5tYl7vA1TQ
  {"UC3yasGCIt1rmKnZ8PukocSw", "Simple Elect", false}, // Simple Electronics - https://www.youtube.com/channel/UC3yasGCIt1rmKnZ8PukocSw
  {"UCpOlOeQjj7EsVnDh3zuCgsA", "Adafruit", false} // https://www.youtube.com/channel/UCpOlOeQjj7EsVnDh3zuCgsA
};

//------- ---------------------- ------

WiFiClientSecure client;
YouTubeLiveStream ytVideo(client, NULL); //"scrapeIsChannelLive" doesn't require a key.

unsigned long delayBetweenRequests = 5000; // Time between requests (5 seconds)
unsigned long requestDueTime;               //time when request due

int channelIndex = 0;
int listChange = false;

void setup() {
  Serial.begin(115200);

  // Set WiFi to 'station' mode and disconnect
  // from the AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Connect to the WiFi network
  Serial.print("\nConnecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);


  //TODO: Use certs
  client.setInsecure();
}

void loop() {

  if (millis() > requestDueTime)
  {
    if (ytVideo.scrapeIsChannelLive(channels[channelIndex].id)) {
      if (!channels[channelIndex].live)
      {
        channels[channelIndex].live = true;
        listChange = true;
      }
      Serial.print(channels[channelIndex].name);
      Serial.println(" is live");
      requestDueTime = millis() + delayBetweenRequests;
    } else {
      if (channels[channelIndex].live)
      {
        channels[channelIndex].live = false;
        listChange = true;
      }
      Serial.print(channels[channelIndex].name);
      Serial.println(" is NOT live");
      requestDueTime = millis() + 5000;
    }
    channelIndex++;
    if (channelIndex >= NUM_CHANNELS)
    {
      channelIndex = 0;
    }
  }

  if (listChange) {
    listChange = false;
    Serial.println("-----------------");
    Serial.println("    Live Now");
    Serial.println("-----------------");
    for (int i = 0; i < NUM_CHANNELS; i++) {
      if (channels[i].live) {
        Serial.println(channels[i].name);
      }
    }
    Serial.println("-----------------");
  }

}