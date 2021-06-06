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

#define YOUTUBE_VIDEOS_ENDPOINT "/youtube/v3/videos"
#define YOUTUBE_LIVECHAT_MESSAGES_ENDPOINT "/youtube/v3/liveChat/messages"

#define YOUTUBE_ACCEPT_COOKIES_COOKIE "CONSENT=YES+cb.20210530-19-p0.en-GB+FX+999"

struct LiveStreamDetails
{
    char *concurrentViewers;
    char *activeLiveChatId;
    bool error;
};


struct ChatMessage
{
    char *displayMessage;
    char *displayName;
    bool isMod;
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
    int getHttpStatusCode();
    void skipHeaders(bool tossUnexpectedForJSON = true);
    void closeClient();

    LiveStreamDetails liveStreamDetails;
    ChatResponses chatResponses;
    const char *searchEndpointAndParams = 
        R"(/youtube/v3/search?eventType=live&part=id&channelId=%s&type=video&key=%s&maxResults=1)"
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