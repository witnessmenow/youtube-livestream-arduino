/*******************************************************************
    
    This is getLiveStreamMessagesExample but using certs to validate the HTTPs
    endpoints.

    We have a complication with this though, this library connects to youtube.com 
    and the youtube api, which are on different servers.
  
    We will default to API cert and swap in the youtube cert when needed, then swap back.

    NOTE: setInsecure is faster, but doesn't verify the server is who you expect.
    
    Display messages and Super chats/stickers from a live stream
    on a given channel.

    This technically works on an ESP8266, but it does not have enough
    memory to handle all the messages, just use an ESP32 for this use
    case.

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


#define ARDUINOJSON_DECODE_UNICODE 1 // Tell ArduinoJson to decide unicode, needs to be before the #include!

#include <ArduinoJson.h>
// Library used for parsing Json from the API responses

// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

//------- Replace the following! ------

char ssid[] = "SSID";         // your network SSID (name)
char password[] = "password"; // your network password

// You need 1 API key per roughly 2 hours of chat you plan to monitor 
// So you can pass in just one:

#define YT_API_TOKEN "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD"

// Or you can pass in an array of keys, 2 keys gives 4 hours, 3 == 6 etc (See Github readme for info)

//#define NUM_API_KEYS 2
//const char *keys[NUM_API_KEYS] = {"AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD", "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCEEEEEEEEEE"};


//#define CHANNEL_ID "UCezJOfu7OtqGzd5xrP3q6WA" //Brian Lough
#define CHANNEL_ID "UCSJ4gkVC6NrvII8umztf0Ow" //Lo-fi beats (basically always live)

#define LED_PIN LED_BUILTIN

//------- ---------------------- ------

WiFiClientSecure client;

#ifdef NUM_API_KEYS
YouTubeLiveStream ytVideo(client, keys, NUM_API_KEYS);
#else
YouTubeLiveStream ytVideo(client, YT_API_TOKEN);
#endif


unsigned long requestDueTime;               //time when request due
unsigned long delayBetweenRequests = 5000; // Time between requests (5 seconds)

LiveStreamDetails details;
char liveChatId[YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH];
char videoId[YOUTUBE_VIDEO_ID_LENGTH];
bool haveVideoId = false;
bool haveLiveChatId = false;
bool ledState = false;

char lastMessageReceived[YOUTUBE_MSG_CHAR_LENGTH];

void setup() {
  liveChatId[0] = '\0';
  videoId[0] = '\0';
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


// Handle HTTPS Server verification
// setInsecture is faster, but doesn't verify who you are talking to
//client.setInsecure();
#if defined(ESP8266)
  client.setFingerprint(YOUTUBE_API_FINGERPRINT) // These will change somewhat regularly 
  // More details in this video: https://www.youtube.com/watch?v=HUjFMVOpXBM
#elif defined(ESP32)
  //client.setInsecure(); 

  client.setCACert(youtubeApiCert); // This is the cert for the API 
#endif

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

bool processMessage(ChatMessage chatMessage, int index, int numMessages) {

  if (index == 0) {
    Serial.print("Total Number of Messages: ");
    Serial.println(numMessages);
  }

  // Use the chat members details in this method
  // or if you want to store them make sure
  // you copy (using something like strcpy) them

  switch (chatMessage.type)
  {
    case yt_message_type_text:
      printMessage(chatMessage);

      //Possible to act on a message
      if ( strcmp(chatMessage.displayMessage, "!led") == 0 )
      {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
      }

      // You can copy message details if you need them for something
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


// This gets the video ID of a live stream on a given channel
void getVideoId() {
  // This is the official way to get the videoID, but it
  // uses too much of your daily quota.
  //haveVideoId = ytVideo.getLiveVideoId(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH);


  // Need to verify a different server before making the request. 
  // We don't need to do this if using "client.setInsecure();" in all places
  // FYI - I really feel client.setInsecure would be ok here...
  //client.setInsecure(); 
#if defined(ESP8266)
  client.setFingerprint(YOUTUBE_FINGERPRINT);
#elif defined(ESP32)
  client.setCACert(youtubeCert); // This is the cert for youtube.com 
#endif

  haveVideoId = ytVideo.scrapeIsChannelLive(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH);
  if (haveVideoId) {
    Serial.println("Channel is live");
    Serial.print("Video ID: ");
    Serial.println(videoId);
  } else {
    Serial.println("Channel does not seem to be live");
  }

  // getting ready to make requests to the API again.
#if defined(ESP8266)
  client.setFingerprint(YOUTUBE_API_FINGERPRINT);
#elif defined(ESP32)
  client.setCACert(youtubeApiCert); // This is the cert for the API 
#endif
}

// This gets the Live Chat ID of a live stream on a given Video ID.
// This is needed to get chat messages.
void getLiveChatId() {
  haveLiveChatId = false;

  details = ytVideo.getLiveStreamDetails(videoId);
  if (!details.error) {
    if (details.isLive) {
      Serial.print("Chat Id: ");
      Serial.println(details.activeLiveChatId);
      strncpy(liveChatId, details.activeLiveChatId, sizeof(liveChatId));
      liveChatId[sizeof(liveChatId) - 1] = '\0';
      haveLiveChatId = true;
    } else {
      Serial.println("Video does not seem to be live");
      haveVideoId = false;
    }
  } else {
    Serial.println("Error getting Live Stream Details");
  }
}

void loop() {
  if (millis() > requestDueTime)
  {
    if (!haveVideoId) {
      //Don't have a video ID, so we'll try get one.
      getVideoId();
    }

    if (haveVideoId && !haveLiveChatId) {
      // We have a video ID, but not a liveChatId
      getLiveChatId();
    }

    if (haveLiveChatId) {
      // The processMessage Callback will be called for every message found.
      ChatResponses responses = ytVideo.getChatMessages(processMessage, liveChatId);
      if (!responses.error) {
        Serial.println("done");
        Serial.print("Polling interval: ");
        Serial.println(responses.pollingIntervalMillis);

        requestDueTime = millis() + responses.pollingIntervalMillis + 500;
      } else if (!responses.isStillLive) {
        //Stream is not live any more.
        haveLiveChatId = false;
        haveVideoId = false;
        requestDueTime = millis() + delayBetweenRequests;
      } else {
        Serial.println("There was an error getting Messages");
        requestDueTime = millis() + delayBetweenRequests;
      }
    } else {
      requestDueTime = millis() + delayBetweenRequests;
    }
  }
}