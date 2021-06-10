/*
Copyright (c) 2020 Brian Lough. All right reserved.

YouTubeLiveStream - An Arduino library to wrap the Youtube API for video stuff

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef YouTubeLiveStream_h
#define YouTubeLiveStream_h


// I find setting these types of flags unreliable from the Arduino IDE
// so uncomment this if its not working for you.
// NOTE: Do not use this option on live-streams, it will reveal your
// private tokens!
#define YOUTUBE_DEBUG 1

// Comment out if you want to disable any serial output from this library (also comment out DEBUG and PRINT_JSON_PARSE)
#define YOUTUBE_SERIAL_OUTPUT 1 

// Prints the JSON received to serial (only use for debugging as it will be slow)
// #define YOUTUBE_PRINT_JSON_PARSE 1

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Client.h>

#ifdef YOUTUBE_PRINT_JSON_PARSE
#include <StreamUtils.h>
#endif

#define YOUTUBE_API_HOST "www.googleapis.com"
#define YOUTUBE_HOST "www.youtube.com"

// Fingerprint correct as of June 10th 2021
#define YOUTUBE_API_FINGERPRINT "CE 0A 82 83 34 79 AA 42 C7 3B 4A 0E FA 1E 98 31 B8 CF 3F FB"
#define YOUTUBE_FINGERPRINT "57 FE CC B1 D0 EA 5D B5 1B 1A 76 B0 7D 03 26 A4 8D 1F 90 83"

#define YOUTUBE_TIMEOUT 2000

#define YOUTUBE_MAX_RESULTS 10

#define YOUTUBE_MSG_CHAR_LENGTH 100 //Increase if MSG are being cut off
#define YOUTUBE_NAME_CHAR_LENGTH 50
#define YOUTUBE_VIEWERS_CHAR_LENGTH 20
#define YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH 80
#define YOUTUBE_LIVE_CHAT_CURRENCY_LENGTH 4

#define YOUTUBE_VIDEOS_ENDPOINT "/youtube/v3/videos"
#define YOUTUBE_LIVECHAT_MESSAGES_ENDPOINT "/youtube/v3/liveChat/messages"

// Required when scraping or it will bring you to a accept cookie landing page
#define YOUTUBE_ACCEPT_COOKIES_COOKIE "CONSENT=YES+cb.20210530-19-p0.en-GB+FX+999"

enum YoutubeMessageType
{
    yt_message_type_unknown,
    yt_message_type_text,
    yt_message_type_superChat,
    yt_message_type_superSticker
};

struct LiveStreamDetails
{
    char *concurrentViewers;
    char *activeLiveChatId;
    bool error;
};


struct ChatMessage
{
    YoutubeMessageType type;
    char *displayMessage;
    char *displayName;
    int tier;
    long amountMicros;
    char *currency;
    bool isChatModerator;
    bool isChatOwner;
    bool isChatSponsor;
    bool isVerified;
};

struct ChatResponses
{
    ChatMessage messages[YOUTUBE_MAX_RESULTS];
    int totalResults;
    int resultsPerPage;
    long pollingIntervalMillis;
    int numMessages;
    bool error;
};

class YouTubeLiveStream
{
  public:
    YouTubeLiveStream(Client &client, const char *apiToken);
    YouTubeLiveStream(Client &client, const char **apiTokenArray, int tokenArrayLength);
    int makeGetRequest(const char *command, const char *host = YOUTUBE_API_HOST, const char *accept = "application/json", const char *cookie = NULL);
    char* getLiveVideoId(const char *channelId);
    bool scrapeIsChannelLive(const char *channelId, char *videoIdOut = NULL, int videoIdOutSize = 0);
    LiveStreamDetails getLiveChatId(const char *videoId);
    ChatResponses getChatMessages(const char *liveChatId, const char *part = "id,snippet,authorDetails");
    int portNumber = 443;
    bool _debug = true;
    Client *client;
    char nextPageToken[50];
    void initStructs();
    void destroyStructs();

  private:
    const char *_apiToken;
    const char **_apiTokenArray;
    int _tokenArrayLength = 0;
    int apiTokenIndex;
    int getHttpStatusCode();
    void rotateApiKey();
    void skipHeaders(bool tossUnexpectedForJSON = true);
    void closeClient();

    LiveStreamDetails liveStreamDetails;
    ChatResponses chatResponses;
    const char *searchEndpointAndParams = 
        R"(/youtube/v3/search?eventType=live&part=id&channelId=%s&type=video&key=%s&maxResults=1&isMine=true)"
    ;

    const char *videoLivestreamDetailsEndpoint = 
        R"(/youtube/v3/videos?part=liveStreamingDetails&id=%s&key=%s)"
    ;

    const char *liveChatMessagesEndpoint = 
        R"(/youtube/v3/liveChat/messages?liveChatId=%s&part=%s&key=%s)"
    ;

    const char *youTubeChannelUrl = 
        R"(/channel/%s)"
    ;

};

#endif