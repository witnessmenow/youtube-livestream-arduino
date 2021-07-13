# youtube-livestream-arduino

An arduino library for interacting with YouTube live streams.

## Help support what I do!

I have put a lot of effort into creating Arduino libraries that I hope people can make use of. [If you enjoy my work, please consider becoming a Github sponsor!](https://github.com/sponsors/witnessmenow/)

## Library Features:

The Library supports the following features:

- Checking if a channel is live - [Example](/examples/checkWhoIsLive/checkWhoIsLive.ino)
- Check how many viewers a stream has - [Example](/examples/getLiveViewerCount/getLiveViewerCount.ino)
- Retrieve live stream messages (Realistically ESP32 only) - [Example](examples/getLiveStreamMessages/getLiveStreamMessages.ino)
- Retrieve super-chats and super-stickers (Realistically ESP32 only) - [Example](examples/getLiveStreamMessages/getLiveStreamMessages.ino)

## Setup Instructions

### Installation

Download zip from Github and install to the Arduino IDE using that.

#### Dependencies

- V6 of Arduino JSON - can be installed through the Arduino Library manager.

### API Keys

All calls other than `scrapeIsChannelLive` require a valid API Key from Google. These are free, but have a quota of 10k per day. Different requests use up different amounts of the quota (details listed in the **API Endpoints Details** table below)

#### Instructions to get an API key

- Go to [the Google developer dashboard](https://console.developers.google.com) and create a new project.
- Go to the [the Google API library](https://console.developers.google.com/apis/library), find the ["YouTube Data API v3"](https://console.developers.google.com/apis/library/youtube.googleapis.com), and "Enable" it for your project.
- In the "API & Services" menu, [go to "Credentials"](https://console.developers.google.com/apis/credentials), click "Create Credentials" and create a new API key.
- (Optional) You can limit this API key to only work with the YouTube API
  - Click on the API key
  - Under "API restrictions" select "Restrict Key"
  - Check the "YouTube Data API v3"
- Make sure the following URL works for you in your browser (change the key at the end!): `https://www.googleapis.com/youtube/v3/channels?part=statistics&id=UCezJOfu7OtqGzd5xrP3q6WA&key=PutYourNewlyGeneratedKeyHere`

Note: For the above URLs, you may need to select the correct project, there is a drop down on the top left.

#### Multiple API Keys

An API key can actively monitor live chat constantly for just over **2 hours**, seeing as this isn't that long, this library supports using multiple API keys to spread out the quota (2 keys should get over 4 hours, 3 keys -> 6 hours etc). To create multiple keys, repeat the steps above, including creating a new project each time.

To use multiple API keys, use the following code. Add as many API keys as needed to the array, just make sure `NUM_API_KEYS` matches. The Library will automatically rotate the keys each request.

```
//#define NUM_API_KEYS 2
//const char *keys[NUM_API_KEYS] = {"AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD", "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCEEEEEEEEEE"};

YouTubeLiveStream ytVideo(client, keys, NUM_API_KEYS);
```

## Library Usage

See examples for more context on how to use these methods.

### Initializing the library

Note: API key is not needed for `scrapeIsChannelLive` so you can pass in `NULL` for the API key if you only want to use that

```
#define YT_API_TOKEN "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD"

// Or you can pass in an array of keys, 2 keys gives 4 hours, 3 == 6 etc (See Github readme for info)

//#define NUM_API_KEYS 2
//const char *keys[NUM_API_KEYS] = {"AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD", "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCEEEEEEEEEE"};

WiFiClientSecure client; // Any client that supports HTTPS

#ifdef NUM_API_KEYS
YouTubeLiveStream ytVideo(client, keys, NUM_API_KEYS);
#else
YouTubeLiveStream ytVideo(client, YT_API_TOKEN);
#endif
```

### Check if a channel is Live / get video id

```
bool scrapeIsChannelLive(const char *channelId, char *videoIdOut = NULL, int videoIdOutSize = 0);
```

This scrapes the channel's YouTube page to check if the channel is live. `videoIdOut` will be updated with the video ID of the stream if the channel is live, but is optional.

#### Example

```
#define CHANNEL_ID "UCSJ4gkVC6NrvII8umztf0Ow" //Lo-fi beats (basically always live)
char videoId[YOUTUBE_VIDEO_ID_LENGTH];
bool haveVideoId = false;

void getVideoId() {
  // This is the official way to get the videoID, but it
  // uses too much of your daily quota.
  //haveVideoId = ytVideo.getLiveVideoId(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH);

  haveVideoId = ytVideo.scrapeIsChannelLive(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH);
  if (haveVideoId) {
    Serial.println("Channel is live");
    Serial.print("Video ID: ");
    Serial.println(videoId);
  } else {
    Serial.println("Channel does not seem to be live");
  }
}
```

### Get Live Stream Details

```
LiveStreamDetails getLiveStreamDetails(const char *videoId);

struct LiveStreamDetails
{
    char *concurrentViewers;
    char *activeLiveChatId;
    bool isLive;
    bool error;
};

```

This returns details about the live Stream

#### Example

```
#define CHANNEL_ID "UCSJ4gkVC6NrvII8umztf0Ow" //Lo-fi beats (basically always live)
char videoId[YOUTUBE_VIDEO_ID_LENGTH]; // Update using "scrapeChannelIsLive"
char liveChatId[YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH];
bool haveLiveChatId = false;
LiveStreamDetails details;

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
      liveChatId[sizeof(liveChatId) - 1] = '\0'; // Ensure we have a '\0' terminator
      haveLiveChatId = true;
    } else {
      Serial.println("Video does not seem to be live");
      haveVideoId = false;
    }
  } else {
    Serial.println("Error getting Live Stream Details");
  }
}
```

### Get Chat Messages (including super chats/stickers)

```
ChatResponses getChatMessages(processChatMessage chatMessageCallback, const char *liveChatId, bool reverse = false, const char *part = "id,snippet,authorDetails");

// Gets returned from getChatMessages
struct ChatResponses
{
    int totalResults;
    int resultsPerPage;
    long pollingIntervalMillis;
    int numMessages;
    bool isStillLive;
    bool error;
};

// Is returned as a parameter of the callback message
struct ChatMessage
{
    YoutubeMessageType type;
    const char *displayMessage;
    const char *displayName;
    int tier; // Only applies to super chat/sticker
    long amountMicros; // Only applies to super chat/sticker
    const char *currency; // Only applies to super chat/sticker
    bool isChatModerator;
    bool isChatOwner;
    bool isChatSponsor;
    bool isVerified;
};

enum YoutubeMessageType
{
    yt_message_type_unknown,
    yt_message_type_text,
    yt_message_type_superChat,
    yt_message_type_superSticker
};

```

This gets all the chat messages and super chats/stickers. Takes a callback as the first parameter. By default it will return the messages from oldest to newest, but can be reversed using the `reverse` flag.

#### Callback example

```
bool processMessage(ChatMessage chatMessage, int index, int numMessages) {
  switch (chatMessage.type)
  {
    case yt_message_type_text:
      //Do something with message
      break;
    case yt_message_type_superChat:
    case yt_message_type_superSticker:
      // Do something with super chat or sticker
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
  // Do stuff ....
  ChatResponses responses = ytVideo.getChatMessages(processMessage, liveChatId, true); //true reverses the messages, so most recent comes first.
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
}
```

## Additional Information

### API Endpoints Details

| Endpoint Name    | Description                                                 | Documentation                                                               | Quota (10k per day)     |
| ---------------- | ----------------------------------------------------------- | --------------------------------------------------------------------------- | ----------------------- |
| liveChatMessages | Gets live stream messages and super-chats                   | [Link](https://developers.google.com/youtube/v3/live/docs/liveChatMessages) | 5 (I think, not listed) |
| Videos           | Uses video ID to get "activeLiveChatId" (also viewer count) | [Link](https://developers.google.com/youtube/v3/docs/videos/list)           | 1                       |
| Search           | Can be used to get live stream video ID (but don't)         | [Link](https://developers.google.com/youtube/v3/docs/search/list)           | 100 (!!!!?)             |

### Scraping Endpoints

Ideally you would never need to scrape a webpage as it can be pretty fragile if any changes are made. But the "search" endpoint quota cost is just not practical so I guess needs must

| Page              | Description                                                 | How?                                                                                  |
| ----------------- | ----------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| Main Channel page | Checking if the channel is live and extracting the video ID | Basically searching for the existence of the "55 watching" element on the live-stream |
