/*
Copyright (c) 2020 Brian Lough. All right reserved.

ARduinoYoutubeVideoApi - An Arduino library to wrap the Youtube API for video stuff

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

#ifndef ArduinoYoutubeVideoApi_h
#define ArduinoYoutubeVideoApi_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Client.h>

#define YOUTUBE_API_HOST "www.googleapis.com"
#define YOUTUBE_HOST "www.youtube.com"
// Fingerprint correct as of June 11th 2020
//#define SLACK_FINGERPRINT "C1 0D 53 49 D2 3E E5 2B A2 61 D5 9E 6F 99 0D 3D FD 8B B2 B3"
#define YOUTUBE_TIMEOUT 2000

#define YOUTUBE_MAX_RESULTS 5

#define YOUTUBE_MSG_CHAR_LENGTH 100 //Increase if MSG are being cut off
#define YOUTUBE_NAME_CHAR_LENGTH 50
#define YOUTUBE_VIEWERS_CHAR_LENGTH 20
#define YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH 80
#define YOUTUBE_LIVE_CHAT_CURRENCY_LENGTH 4

#define YOUTUBE_VIDEOS_ENDPOINT "/youtube/v3/videos"
#define YOUTUBE_LIVECHAT_MESSAGES_ENDPOINT "/youtube/v3/liveChat/messages"

#define YOUTUBE_ACCEPT_COOKIES_COOKIE "CONSENT=YES+cb.20210530-19-p0.en-GB+FX+999"

/*
            "snippet": {
                "type": "superChatEvent",
                "liveChatId": "Cg0KCzlUVEM2VmpWVXVrKicKGFVDOHJRS08yWGhQbnZobnlWMWVBTGE2ZxILOVRUQzZWalZVdWs",
                "authorChannelId": "UCezJOfu7OtqGzd5xrP3q6WA",
                "publishedAt": "2021-06-09T15:40:52.765983+00:00",
                "hasDisplayContent": true,
                "displayMessage": "€2.00 from Brian Lough: \"test\"",
                "superChatDetails": {
                    "amountMicros": "2000000",
                    "currency": "EUR",
                    "amountDisplayString": "€2.00",
                    "userComment": "test",
                    "tier": 2
                }
            },
            "authorDetails": {
                "channelId": "UCezJOfu7OtqGzd5xrP3q6WA",
                "channelUrl": "http://www.youtube.com/channel/UCezJOfu7OtqGzd5xrP3q6WA",
                "displayName": "Brian Lough",
                "profileImageUrl": "https://yt3.ggpht.com/ytc/AAUvwnhN1ReWNGKAXND2kwS8Yk3Z4Vs8ea-wMXd_1mzUag=s88-c-k-c0x00ffffff-no-rj",
                "isVerified": false,
                "isChatOwner": false,
                "isChatSponsor": false,
                "isChatModerator": true
            }

*/

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

class ArduinoYoutubeVideoApi
{
  public:
    ArduinoYoutubeVideoApi(Client &client, const char *apiToken);
    ArduinoYoutubeVideoApi(Client &client, const char **apiTokenArray, int tokenArrayLength);
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