/*******************************************************************
    Sets a custom status on your slack account. It will toggle between
    two every 30 seconds

    You will need a bearer token, see readme for more details

    You will also need to be on version 2.5 or higher of the ESP8266 core

    Parts:
    D1 Mini ESP8266 * - http://s.click.aliexpress.com/e/uzFUnIe

 *  * = Affilate

    If you find what I do usefuland would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// ----------------------------
// Standard Libraries
// ----------------------------

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <ArduinoYoutubeVideoApi.h>


#include <ArduinoJson.h>
// Library used for parsing Json from the API responses

// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

//------- Replace the following! ------

char ssid[] = "SSID";         // your network SSID (name)
char password[] = "password"; // your network password

#define YT_API_TOKEN "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD"

#define CHANNEL_ID "UCezJOfu7OtqGzd5xrP3q6WA"


//------- ---------------------- ------

WiFiClientSecure client;
ArduinoYoutubeVideoApi ytVideo(client, YT_API_TOKEN);

unsigned long delayBetweenRequests = 30000; // Time between requests (1 minute)
unsigned long requestDueTime;               //time when request due

LiveStreamDetails details;
String liveId;
void setup() {

  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  //client.setFingerprint(SLACK_FINGERPRINT);
  client.setInsecure();
  // If you want to enable some extra debugging
  ytVideo._debug = true;

  char *videoId = ytVideo.getLiveVideoId(CHANNEL_ID);
  if (videoId != NULL) {
    Serial.print("Channel is live now with Video ID: ");
    Serial.println(videoId);

    details = ytVideo.getLiveChatId(videoId);
    if (!details.error) {
      Serial.print("concurrent Viewers: ");
      Serial.println(details.concurrentViewers);
      Serial.print("Chat Id: ");
      Serial.println(details.activeLiveChatId);
      liveId = String(details.activeLiveChatId);
    } else {
      Serial.println("Error getting Live Stream Details");
    }
  } else {
    Serial.println("Channel does not appear to be live");
  }
}

void printMessage(ChatMessage message) {
  Serial.print(message.displayName);
  if (message.isMod) {
    Serial.print("(mod)");
  }
  Serial.print(": ");
  Serial.println(message.displayMessage);
}

void loop() {
  if (liveId.length() > 0) {
    if (millis() > requestDueTime)
    {
      //Serial.println(details.activeLiveChatId);
      Serial.println(liveId);
      //ChatResponses responses = ytVideo.getChatMessages(details.activeLiveChatId);
      ChatResponses responses = ytVideo.getChatMessages((char *)liveId.c_str());
      if (!responses.error) {
        for (int i = 0; i < responses.resultsPerPage; i++) {
          printMessage(responses.messages[i]);
        }
        Serial.println("done");
        requestDueTime = millis() + responses.pollingIntervalMillis + 500;
      } else {
        Serial.println("There was an error");
        requestDueTime = millis() + 5000;
      }
    }
  }

}