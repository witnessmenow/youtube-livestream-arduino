/*******************************************************************
    Display messages and Super chats/stickers from a live stream
    on a given channel.

    Parts:
    D1 Mini ESP8266 * - http://s.click.aliexpress.com/e/uzFUnIe

 *  * = Affilate

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

#include <WiFi.h>
#include <WiFiClientSecure.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <YouTubeLiveStream.h>

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

#define LED_PIN 2

//------- ---------------------- ------

WiFiClientSecure client;
ArduinoYoutubeVideoApi ytVideo(client, YT_API_TOKEN);

unsigned long delayBetweenRequests = 30000; // Time between requests (1 minute)
unsigned long requestDueTime;               //time when request due

LiveStreamDetails details;
String liveId;
bool ledState = false;
void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //TODO: Use certs
  client.setInsecure();


  char videoId[20];

  // This is the official way to get the videoID, but it
  // uses too much of your daily quota.
  //char *videoId = ytVideo.getLiveVideoId(CHANNEL_ID);

  if (ytVideo.scrapeIsChannelLive(CHANNEL_ID, videoId, 20))
  {
    Serial.println("Channel is live");
    if (videoId != NULL) {
      Serial.print("Video ID: ");
      Serial.println(videoId);

      delay(100);
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
  } else {
    Serial.println("Channel is NOT live");
  }
}

void printMessage(ChatMessage message) {
  Serial.print(message.displayName);
  if (message.isChatModerator) {
    Serial.print("(mod)");
  }
  Serial.print(": ");
  Serial.println(message.displayMessage);
}

void printSuperThing(ChatMessage message) {
  Serial.print(message.displayName);
  if (message.isChatModerator) {
    Serial.print("(mod)");
  }
  Serial.print(": ");
  Serial.println(message.displayMessage);
  
  Serial.print(message.currency);
  Serial.print(" ");
  long cents = message.amountMicros / 10000;
  long centsOnly = cents % 100;
  
  Serial.print(cents / 100);
  Serial.print(".");
  if(centsOnly < 10){
    Serial.print("0");
  }
  Serial.println(centsOnly);

  Serial.print("Tier: ");
  Serial.println(message.tier);
}

void loop() {
  if (liveId.length() > 0) {
    if (millis() > requestDueTime)
    {
      ChatResponses responses = ytVideo.getChatMessages((char *)liveId.c_str());
      if (!responses.error) {
        for (int i = 0; i < responses.numMessages; i++) {
          if(responses.messages[i].type == yt_message_type_text){
            printMessage(responses.messages[i]);
          } else if (responses.messages[i].type == yt_message_type_superChat || responses.messages[i].type == yt_message_type_superSticker){
            printSuperThing(responses.messages[i]);
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
          } else {
            Serial.print("Unknown Message Type: ");
            Serial.println(responses.messages[i].type);
          }

        }
        Serial.println("done");
        Serial.print("Polling interval: ");
        Serial.println(responses.pollingIntervalMillis);

        requestDueTime = millis() + responses.pollingIntervalMillis + 500;
      } else {
        Serial.println("There was an error");
        requestDueTime = millis() + 5000;
      }
    }
  }

}