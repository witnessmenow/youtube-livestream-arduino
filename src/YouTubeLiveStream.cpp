/*
Copyright (c) 2020 Brian Lough. All right reserved.

ArduinoSlack - An Arduino library to wrap the Slack API

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

#include "YouTubeLiveStream.h"

YouTubeLiveStream::YouTubeLiveStream(Client &client, const char *apiToken)
{
    this->client = &client;
    this->_apiToken = apiToken;
    nextPageToken[0] = 0;
    initStructs();
}

YouTubeLiveStream::YouTubeLiveStream(Client &client, const char **apiTokenArray, int tokenArrayLength)
{
    this->client = &client;
    this->_apiTokenArray = apiTokenArray;
    this->_tokenArrayLength = tokenArrayLength;
    nextPageToken[0] = 0;
    initStructs();
}

int YouTubeLiveStream::makeGetRequest(const char *command, const char *host, const char *accept, const char *cookie)
{
    client->flush();
    client->setTimeout(YOUTUBE_TIMEOUT);
    if (!client->connect(host, portNumber))
    {
        #ifdef YOUTUBE_SERIAL_OUTPUT
        Serial.println(F("Connection failed"));
        #endif
        return false;
    }

    // give the esp a breather
    yield();

    // Send HTTP request
    client->print(F("GET "));
    client->print(command);
    client->println(F(" HTTP/1.0"));

    //Headers
    client->print(F("Host: "));
    client->println(host);

    if (accept != NULL)
    {
        client->print(F("Accept: "));
        client->println(accept);
    }

    if (cookie != NULL)
    {
        client->print(F("Cookie: "));
        client->println(cookie);
    } 

    client->println(F("Cache-Control: no-cache"));

    //client->println();

    if (client->println() == 0)
    {
        #ifdef YOUTUBE_SERIAL_OUTPUT
        Serial.println(F("Failed to send request"));
        #endif
        return false;
    }

    int statusCode = getHttpStatusCode();
    return statusCode;
}

bool YouTubeLiveStream::getLiveVideoId(const char *channelId, char *videoIdOut, int videoIdOutSize){
    char command[250];

    if(_tokenArrayLength > 0){
        rotateApiKey();
    }

    sprintf(command, searchEndpointAndParams, channelId, _apiToken);
    #ifdef YOUTUBE_DEBUG
    Serial.println(command);
    #endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = 200;

    if (makeGetRequest(command) == 200)
    {
        skipHeaders();

        StaticJsonDocument<64> filter;
        filter["items"][0]["id"]["videoId"] = true;

        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        #ifndef YOUTUBE_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, *client, DeserializationOption::Filter(filter));
        #else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
        #endif
        if (!error)
        {
            strncpy(videoIdOut, doc["items"][0]["id"]["videoId"].as<const char *>(), videoIdOutSize);
            videoIdOut[videoIdOutSize -1] = '\0';
            closeClient();
            return true;

        }
        else
        {
            #ifdef YOUTUBE_SERIAL_OUTPUT
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
            #endif
        }
    }
    closeClient();
    return false;
}

LiveStreamDetails YouTubeLiveStream::getLiveStreamDetails(const char *videoId){
    char command[250];

    if(_tokenArrayLength > 0){
        rotateApiKey();
    }

    sprintf(command, videoLivestreamDetailsEndpoint, videoId, _apiToken);
    #ifdef YOUTUBE_DEBUG
    Serial.println(command);
    #endif

    liveStreamDetails.error = true;

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = 500;
    int statusCode = makeGetRequest(command);
    if (statusCode == 200)
    {
        skipHeaders();
        // Allocate DynamicJsonDocument

        StaticJsonDocument<80> filter;
        JsonObject filter_items_0_liveStreamingDetails = filter["items"][0].createNestedObject("liveStreamingDetails");
        filter_items_0_liveStreamingDetails["concurrentViewers"] = true;
        filter_items_0_liveStreamingDetails["activeLiveChatId"] = true;

        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        #ifndef YOUTUBE_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, *client, DeserializationOption::Filter(filter));
        #else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
        #endif
        if (!error)
        {
            liveStreamDetails.error = false;

            JsonObject liveStreamingDetails = doc["items"][0]["liveStreamingDetails"];

            if (liveStreamingDetails.containsKey("activeLiveChatId")){
                strncpy(liveStreamDetails.activeLiveChatId,  liveStreamingDetails["activeLiveChatId"].as<const char *>(), YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH);
                liveStreamDetails.activeLiveChatId[YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH -1] = '\0';

                liveStreamDetails.isLive = true;
            } else {
                liveStreamDetails.isLive = false; // Probably!?
                liveStreamDetails.activeLiveChatId[0] = '\0';
            }

            if (liveStreamingDetails.containsKey("concurrentViewers")){
                strncpy(liveStreamDetails.concurrentViewers, liveStreamingDetails["concurrentViewers"].as<const char *>(), YOUTUBE_VIEWERS_CHAR_LENGTH);
                liveStreamDetails.concurrentViewers[YOUTUBE_VIEWERS_CHAR_LENGTH -1] = '\0';
            } else {
                liveStreamDetails.concurrentViewers[0] = '\0';
            }

        }
        else
        {
            #ifdef YOUTUBE_SERIAL_OUTPUT
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
            #endif
        }
    } else {
       
        #ifdef YOUTUBE_SERIAL_OUTPUT
        Serial.print("Not 200: ");
        Serial.println(statusCode);
        #endif

        #ifdef YOUTUBE_DEBUG
        while (client->available() )
        {
            char c = 0;
            client->readBytes(&c, 1);
            Serial.print(c);

        }
        #endif

    }
    closeClient();
    return liveStreamDetails;
}

bool YouTubeLiveStream::scrapeIsChannelLive(const char *channelId, char *videoIdOut, int videoIdOutSize){
    char command[100];
    sprintf(command, youTubeChannelUrl, channelId);

    bool channelIsLive = true;

    #ifdef YOUTUBE_DEBUG
    Serial.print("url: ");
    Serial.println(command);
    #endif

    int statusCode = makeGetRequest(command, YOUTUBE_HOST, "*/*", YOUTUBE_ACCEPT_COOKIES_COOKIE);
    if(statusCode == 200) {
        if (!client->find("{\"text\":\" watching\"}"))
        {
            #ifdef YOUTUBE_DEBUG
            Serial.println(F("Channel doesn't seem to be live"));
            #endif

            channelIsLive = false;
        } else if (videoIdOut != NULL){
            if (!client->find("{\"videoId\":\""))
            {
                videoIdOut[0] = '\0';
                #ifdef YOUTUBE_SERIAL_OUTPUT
                Serial.println(F("Could not find videoID"));
                #endif
                channelIsLive = false;
            } else {
                client->readBytesUntil('\"', videoIdOut, videoIdOutSize - 1); // leave room for null
                videoIdOut[videoIdOutSize - 1] = '\0';
            }
        }
    } else {
        #ifdef YOUTUBE_SERIAL_OUTPUT
        Serial.print("Not 200: ");
        Serial.println(statusCode);
        #endif

        #ifdef YOUTUBE_DEBUG
        while (client->available() )
        {
            char c = 0;
            client->readBytes(&c, 1);
            Serial.print(c);

        }
        #endif
        channelIsLive = false;
    }

    closeClient();
    return channelIsLive;

}

ChatResponses YouTubeLiveStream::getChatMessages(processChatMessage chatMessageCallback, const char *liveChatId, bool reverse, const char *part){
    char command[300];

    if(_tokenArrayLength > 0){
        rotateApiKey();
    }

    sprintf(command, liveChatMessagesEndpoint, liveChatId, part, _apiToken);

    if(nextPageToken[0] != 0){
        char nextPageParam[50];
        sprintf(nextPageParam, "&pageToken=%s", nextPageToken);
        strcat(command, nextPageParam);
    }

    #ifdef YOUTUBE_DEBUG
    Serial.println(command);
    #endif

    chatResponses.error = true;
    chatResponses.isStillLive = true; //assume, we'll update if not

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = 30000;
    int statusCode = makeGetRequest(command);
    if (statusCode == 200)
    {
        skipHeaders();
        StaticJsonDocument<272> filter;
        filter["pollingIntervalMillis"] = true;
        filter["offlineAt"] = true;
        filter["nextPageToken"] = true;

        JsonObject filter_pageInfo = filter.createNestedObject("pageInfo");
        filter_pageInfo["totalResults"] = true;
        filter_pageInfo["resultsPerPage"] = true;

        JsonObject filter_items_0 = filter["items"].createNestedObject();

        JsonObject filter_items_0_authorDetails = filter_items_0.createNestedObject("authorDetails");
        filter_items_0_authorDetails["displayName"] = true;
        filter_items_0_authorDetails["isChatModerator"] = true;
        filter_items_0_authorDetails["isChatSponsor"] = true;
        filter_items_0_authorDetails["isVerified"] = true;

        JsonObject filter_items_0_snippet = filter_items_0.createNestedObject("snippet");
        filter_items_0_snippet["displayMessage"] = true;
        filter_items_0_snippet["type"] = true;
        filter_items_0_snippet["superChatDetails"] = true;
        filter_items_0_snippet["superStickerDetails"] = true;

        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        #ifndef YOUTUBE_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, *client, DeserializationOption::Filter(filter));
        #else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
        #endif
        if (!error)
        {
            chatResponses.error = false;
            chatResponses.isStillLive = !doc.containsKey("offlineAt");
            const char* pageToken = doc["nextPageToken"];
            strcpy(nextPageToken, pageToken);
            chatResponses.pollingIntervalMillis = doc["pollingIntervalMillis"].as<long>();
            chatResponses.totalResults = doc["pageInfo"]["totalResults"].as<int>();
            chatResponses.resultsPerPage = doc["pageInfo"]["resultsPerPage"].as<int>();
            JsonArray items = doc["items"];
            //Serial.print("Got Here");
            int numMessages = YOUTUBE_MAX_RESULTS > items.size() ? items.size() : YOUTUBE_MAX_RESULTS;
            int index = 0;
            for(int i = 0; i < numMessages ; i++){

                //Reverse index
                if(reverse){
                    index = items.size() - 1 - i;
                } else {
                    index = i;
                }

#ifdef YOUTUBE_DEBUG
                Serial.print(F("Message: "));
                serializeJson(items[index], Serial);
#endif

                // init message back to blank
                chatMessage.displayMessage = nullptr;
                chatMessage.displayName = nullptr;
                chatMessage.type = yt_message_type_unknown;
                chatMessage.tier = -1;
                chatMessage.amountMicros = -1;
                chatMessage.currency = nullptr;

                // It's possible for users to not request snippet
                if (items[index].containsKey("snippet")) {

                    const char *messageType = items[index]["snippet"]["type"]; 
                    #ifdef YOUTUBE_DEBUG
                    Serial.print("messageType: ");
                    Serial.println(messageType);
                    #endif

                    if (strncmp(messageType, "textMessageEvent", 16) == 0)
                    {
                        chatMessage.type = yt_message_type_text;
                        chatMessage.displayMessage = items[index]["snippet"]["displayMessage"].as<const char *>();

                    }
                    else if (strncmp(messageType, "superChatEvent", 14) == 0)
                    {
                        chatMessage.type = yt_message_type_superChat;

                        JsonObject superChatDetails = items[index]["snippet"]["superChatDetails"];

                        if(superChatDetails.containsKey("userComment")){
                            chatMessage.displayMessage = superChatDetails["userComment"].as<const char *>();
                        }
                        
                        chatMessage.tier = superChatDetails["tier"].as<int>();
                        chatMessage.amountMicros = superChatDetails["amountMicros"].as<long>();

                        chatMessage.currency = superChatDetails["currency"].as<const char *>();
                    } 
                    else if (strncmp(messageType, "superStickerEvent", 17) == 0)
                    {
                        chatMessage.type = yt_message_type_superSticker;

                        JsonObject superStickerDetails = items[index]["snippet"]["superStickerDetails"];

                        if(superStickerDetails.containsKey("userComment")){
                            chatMessage.displayMessage = superStickerDetails["userComment"].as<const char *>();
                        }                      

                        chatMessage.tier = superStickerDetails["tier"].as<int>();
                        chatMessage.amountMicros = superStickerDetails["amountMicros"].as<long>();

                        chatMessage.currency = superStickerDetails["currency"].as<const char *>();
                    }
                    else
                    {
                        chatMessage.type = yt_message_type_unknown;
                    }
                    
                }

                 // It's possible for users to not request authorDetails, it's only needed if you need the name of person who sent the message.
                if (items[index].containsKey("authorDetails")) {
                    chatMessage.displayName = items[index]["authorDetails"]["displayName"].as<const char *>();
                    chatMessage.isChatModerator = items[index]["authorDetails"]["isChatModerator"].as<bool>();
                    chatMessage.isChatOwner = items[index]["authorDetails"]["isChatOwner"].as<bool>();
                    chatMessage.isChatSponsor = items[index]["authorDetails"]["isChatSponsor"].as<bool>();
                    chatMessage.isVerified = items[index]["authorDetails"]["isVerified"].as<bool>();
                } else {
                    #ifdef YOUTUBE_SERIAL_OUTPUT
                    Serial.println("no authorDetails");
                    #endif
                    chatMessage.isChatModerator = false;
                    chatMessage.isChatOwner = false;
                    chatMessage.isChatSponsor = false;
                    chatMessage.isVerified = false;
                }

                if(!chatMessageCallback(chatMessage)){
                    //User has indicated they are finished.
                    break;
                };               
            }

        }
        else
        {
            #ifdef YOUTUBE_SERIAL_OUTPUT
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
            #endif
        }
    } else if(statusCode == 403) {
        if (client->find("\"reason\": \"")){
            char errorMessage[100] = {0};
            client->readBytesUntil('"', errorMessage, sizeof(errorMessage));
            if(strcmp(errorMessage, "liveChatEnded") == 0)
            {
                #ifdef YOUTUBE_SERIAL_OUTPUT
                Serial.print(F("Live Stream is no longer live"));
                #endif
                chatResponses.isStillLive = false;
            }
        }
    }
    closeClient();
    return chatResponses;
}



void YouTubeLiveStream::skipHeaders(bool tossUnexpectedForJSON)
{
    // Skip HTTP headers
    if (!client->find("\r\n\r\n"))
    {
        #ifdef YOUTUBE_SERIAL_OUTPUT
        Serial.println(F("Invalid response"));
        #endif
        return;
    }

    if (tossUnexpectedForJSON)
    {
        // Was getting stray characters between the headers and the body
        // This should toss them away
        while (client->available() && client->peek() != '{')
        {
            char c = 0;
            client->readBytes(&c, 1);
            #ifdef YOUTUBE_DEBUG
            Serial.print(F("Tossing an unexpected character: "));
            Serial.println(c);
            #endif
        }
    }
}

int YouTubeLiveStream::getHttpStatusCode()
{
    char status[32] = {0};
    client->readBytesUntil('\r', status, sizeof(status));
    #ifdef YOUTUBE_DEBUG
    Serial.print(F("Status: "));
    Serial.println(status);
    #endif

    char *token;
    token = strtok(status, " "); // https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm

    #ifdef YOUTUBE_DEBUG
    Serial.print(F("HTTP Version: "));
    Serial.println(token);
    #endif

    if (token != NULL && (strcmp(token, "HTTP/1.0") == 0 || strcmp(token, "HTTP/1.1") == 0))
    {
        token = strtok(NULL, " ");
        if(token != NULL){
            #ifdef YOUTUBE_DEBUG
            Serial.print(F("Status Code: "));
            Serial.println(token);
            #endif
            return atoi(token);
        }
        
    }

    return -1;
}

void YouTubeLiveStream::rotateApiKey()
{
    apiTokenIndex++;
    if(apiTokenIndex >= _tokenArrayLength){
        apiTokenIndex = 0;
    }
    _apiToken = _apiTokenArray[apiTokenIndex];
    
    #ifdef YOUTUBE_DEBUG
    Serial.print(F("Rotating API Key: "));
    Serial.println(_apiToken);
    #endif
}

void YouTubeLiveStream::closeClient()
{
    if (client->connected())
    {
        #ifdef YOUTUBE_DEBUG
        Serial.println(F("Closing client"));
        #endif
        client->stop();
    }
}

void YouTubeLiveStream::initStructs()
{
    liveStreamDetails.concurrentViewers = (char *)malloc(YOUTUBE_VIEWERS_CHAR_LENGTH);
    liveStreamDetails.activeLiveChatId = (char *)malloc(YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH);

}

// Not sure why this would ever be needed, but sure why not.
void YouTubeLiveStream::destroyStructs()
{
    free(liveStreamDetails.concurrentViewers);
    free(liveStreamDetails.activeLiveChatId);
}