/*******************************************************************
    Get the amount of people who are watching a live stream
    for a given channel.

    Compatible Boards:
	  - Any ESP32 board

    Parts:
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

#include <YouTubeLiveStreamCert.h> // Comes with above, For HTTPS certs if you need them


#include <ArduinoJson.h>
// Library used for parsing Json from the API responses

// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

//------- Replace the following! ------

char ssid[] = "SSID";         // your network SSID (name)
char password[] = "password"; // your network password

#define YT_API_TOKEN "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD"

//#define CHANNEL_ID "UC8rQKO2XhPnvhnyV1eALa6g" //Bitluni's trash
#define CHANNEL_ID "UCSJ4gkVC6NrvII8umztf0Ow" //Lo-fi beats (basically always live)

#define LED_PIN LED_BUILTIN

//------- ---------------------- ------

WiFiClientSecure client;
YouTubeLiveStream ytVideo(client, YT_API_TOKEN);

unsigned long requestDueTime;               //time when request due
unsigned long delayBetweenRequests = 10000; // Time between requests (10 seconds)
// This should last all day as getLiveChatId uses 1 credit and checking every 10 seconds
// should only use 8640 of your 10K per day.

LiveStreamDetails details;
char videoId[YOUTUBE_VIDEO_ID_LENGTH];

long currentViewers;
bool haveVideoId = false;

void setup() {
  videoId[0] = '\0';
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


  // NOTE: See "usingHTTPSCerts" example for how to verify the server you are talking to.
  client.setInsecure();

}

void getVideoId() {
  // This is the official way to get the videoID, but it
  // uses too much of your daily quota.
  //haveVideoId = ytVideo.getLiveVideoId(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH);

  haveVideoId = ytVideo.scrapeIsChannelLive(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH);
  if (haveVideoId) {
    Serial.println("Channel is live");
    Serial.print("Video ID: ");
    Serial.print(videoId);
  } else {
    Serial.println("Channel does not seem to be live");
  }
}

void loop() {

  if (millis() > requestDueTime)
  {
    if (!haveVideoId) {
      //Don't have a video ID, so we'll try get one.
      getVideoId();
    }

    if (haveVideoId) {
      details = ytVideo.getLiveStreamDetails(videoId);
      if (!details.error) {
        if (details.isLive) {
          Serial.print("Concurrent Viewers from API: ");
          Serial.println(details.concurrentViewers); //concurrentViewers is a char array, as it comes back from the API as a string.

          currentViewers = atol(details.concurrentViewers); //convert to long if you want
          Serial.print("Concurrent Viewers long: ");
          Serial.println(currentViewers);
        } else {
          Serial.println("Video does not seem to be live");
          haveVideoId = false;
        }

      } else {
        Serial.println("Error getting Live Stream Details");
      }
    }

    requestDueTime = millis() + delayBetweenRequests;
  }
}