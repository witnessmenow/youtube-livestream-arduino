/*******************************************************************
    Display messages and Super chats/stickers from a live stream
    on a given channel.

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

LiveStreamDetails details;
char liveId[YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH];
bool ledState = false;

char lastMessageReceived[YOUTUBE_MSG_CHAR_LENGTH];

void setup() {
  liveId[0] = '\0';
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

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


  char videoId[YOUTUBE_VIDEO_ID_LENGTH + 1];

  // This is the official way to get the videoID, but it
  // uses too much of your daily quota.
  //char *videoId = ytVideo.getLiveVideoId(CHANNEL_ID);

  if (ytVideo.scrapeIsChannelLive(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH))
  {
    Serial.println("Channel is live");
    if (videoId != NULL) {
      videoId[YOUTUBE_VIDEO_ID_LENGTH] = '\0';
      Serial.print("Video ID: ");
      Serial.println(videoId);

      delay(100);
      details = ytVideo.getLiveChatId(videoId);
      if (!details.error) {
        Serial.print("concurrent Viewers: ");
        Serial.println(details.concurrentViewers);
        Serial.print("Chat Id: ");
        Serial.println(details.activeLiveChatId);

        strncpy(liveId, details.activeLiveChatId, sizeof(liveId));
        liveId[sizeof(liveId) - 1] = '\0';

        //liveId = String(details.activeLiveChatId);
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

  if (message.isChatSponsor) {
    Serial.print("(sponsor)");
  }

  Serial.print(": ");
  Serial.println(message.displayMessage);

  if ( strcmp(message.displayMessage, "!led") == 0 )
  {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
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
  if (centsOnly < 10) {
    Serial.print("0");
  }
  Serial.println(centsOnly);

  Serial.print("Tier: ");
  Serial.println(message.tier);
}

bool processMessage(ChatMessage chatMessage) {
  // Use the chat members details in this method
  // or if you want to store them make sure
  // you copy (using something like strcpy) them

  switch (chatMessage.type)
  {
    case yt_message_type_text:
      printMessage(chatMessage);

      strncpy(lastMessageReceived, chatMessage.displayMessage, sizeof(lastMessageReceived)); //DO NOT use lastMessageReceived = chatMessage.displayMessage, it won't work as you expect!
      lastMessageReceived[sizeof(lastMessageReceived) - 1] = '\0';
      break;
    case yt_message_type_superChat:
    case yt_message_type_superSticker:
      printSuperThing(chatMessage);
      break;
    default:
      Serial.print("Unknown Message Type: ");
      Serial.println(chatMessage.type);
  }

  // return false from this method if you want to
  // stop parsing more messages.
  return true;
}

void loop() {
  if (liveId[0] != '\0') {
    if (millis() > requestDueTime)
    {
      Serial.print("Live Chat Id: ");
      Serial.println(liveId);
      Serial.print("Pointer: ");
      int ptr = (int) &liveId[0];
      Serial.println(ptr);
      ChatResponses responses = ytVideo.getChatMessages(processMessage, liveId);
      if (!responses.error) {
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