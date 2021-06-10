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
    client->println(F(" HTTP/1.1"));

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

    client->println();

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

char* YouTubeLiveStream::getLiveVideoId(const char *channelId){
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
        #elif
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
        #endif
        if (!error)
        {
            
            char* videoId = (char *) doc["items"][0]["id"]["videoId"].as<const char *>();
            closeClient();
            return videoId;

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
    return NULL;
}

LiveStreamDetails YouTubeLiveStream::getLiveChatId(const char *videoId){
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
        #elif
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
        #endif
        if (!error)
        {
            liveStreamDetails.error = false;

            JsonObject liveStreamingDetails = doc["items"][0]["liveStreamingDetails"];

            strncpy(liveStreamDetails.concurrentViewers, liveStreamingDetails["concurrentViewers"].as<const char *>(), YOUTUBE_VIEWERS_CHAR_LENGTH);
            liveStreamDetails.concurrentViewers[YOUTUBE_VIEWERS_CHAR_LENGTH -1] = '\0';

            strncpy(liveStreamDetails.activeLiveChatId,  liveStreamingDetails["activeLiveChatId"].as<const char *>(), YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH);
            liveStreamDetails.activeLiveChatId[YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH -1] = '\0';

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
                #ifdef YOUTUBE_SERIAL_OUTPUT
                Serial.println(F("Could not find videoID"));
                #endif
                channelIsLive = false;
            } else {
                client->readBytesUntil('\"', videoIdOut, videoIdOutSize);
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

ChatResponses YouTubeLiveStream::getChatMessages(const char *liveChatId, const char *part){
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

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = 30000;
    if (makeGetRequest(command) == 200)
    {
        skipHeaders();
        StaticJsonDocument<272> filter;
        filter["pollingIntervalMillis"] = true;
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
        #elif
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
        #endif
        if (!error)
        {
            chatResponses.error = false;
            const char* pageToken = doc["nextPageToken"];
            strcpy(nextPageToken, pageToken);
            chatResponses.pollingIntervalMillis = doc["pollingIntervalMillis"].as<long>();
            chatResponses.totalResults = doc["pageInfo"]["totalResults"].as<int>();
            chatResponses.resultsPerPage = doc["pageInfo"]["resultsPerPage"].as<int>();
            JsonArray items = doc["items"];
            //Serial.print("Got Here");
            int numMessages = YOUTUBE_MAX_RESULTS > items.size() ? items.size() : YOUTUBE_MAX_RESULTS;
            for(int i = 0; i < numMessages ; i++){
                //numMessages ++;
                int reverseIndex = items.size() - 1 - i;
                #ifdef YOUTUBE_DEBUG
                Serial.print(F("Message: "));
                serializeJson(items[reverseIndex], Serial);
                #endif
                ChatMessage *message = &chatResponses.messages[i];

                // init message back to blank
                message->displayMessage[0] = '\0';
                message->type = yt_message_type_unknown;
                message->tier = -1;
                message->amountMicros = -1;
                message->currency[0] = '\0';

                // It's possible for users to not request snippet
                if (items[reverseIndex].containsKey("snippet")) {

                    const char *messageType = items[reverseIndex]["snippet"]["type"]; 
                    #ifdef YOUTUBE_DEBUG
                    Serial.print("messageType: ");
                    Serial.println(messageType);
                    #endif

                    if (strncmp(messageType, "textMessageEvent", 16) == 0)
                    {
                        message->type = yt_message_type_text;
                        strncpy(message->displayMessage,  items[reverseIndex]["snippet"]["displayMessage"].as<const char *>(), YOUTUBE_MSG_CHAR_LENGTH);
                        message->displayMessage[YOUTUBE_MSG_CHAR_LENGTH -1] = '\0';

                    }
                    else if (strncmp(messageType, "superChatEvent", 14) == 0)
                    {
                        message->type = yt_message_type_superChat;

                        JsonObject superChatDetails = items[reverseIndex]["snippet"]["superChatDetails"];

                        if(superChatDetails.containsKey("userComment")){
                            strncpy(message->displayMessage,  superChatDetails["userComment"].as<const char *>(), YOUTUBE_MSG_CHAR_LENGTH);
                            message->displayMessage[YOUTUBE_MSG_CHAR_LENGTH -1] = '\0';
                        }
                        
                        message->tier = superChatDetails["tier"].as<int>();
                        message->amountMicros = superChatDetails["amountMicros"].as<long>();
                        strncpy(message->currency,  superChatDetails["currency"].as<const char *>(), YOUTUBE_LIVE_CHAT_CURRENCY_LENGTH);
                        message->currency[YOUTUBE_LIVE_CHAT_CURRENCY_LENGTH -1] = '\0';
                    } 
                    else if (strncmp(messageType, "superStickerEvent", 17) == 0)
                    {
                        chatResponses.messages[i].type = yt_message_type_superSticker;

                        JsonObject superStickerDetails = items[reverseIndex]["snippet"]["superStickerDetails"];

                        if(superStickerDetails.containsKey("userComment")){
                            strncpy(message->displayMessage,  superStickerDetails["userComment"].as<const char *>(), YOUTUBE_MSG_CHAR_LENGTH);
                            message->displayMessage[YOUTUBE_MSG_CHAR_LENGTH -1] = '\0';
                        }                      

                        message->tier = superStickerDetails["tier"].as<int>();
                        message->amountMicros = superStickerDetails["amountMicros"].as<long>();
                        strncpy(message->currency,  superStickerDetails["currency"].as<const char *>(), YOUTUBE_LIVE_CHAT_CURRENCY_LENGTH);
                        message->currency[YOUTUBE_LIVE_CHAT_CURRENCY_LENGTH -1] = '\0';
                    }
                    else
                    {
                        chatResponses.messages[i].type = yt_message_type_unknown;
                    }
                    
                }

                 // It's possible for users to not request authorDetails, it's only needed if you need the name of person who sent the message.
                if (items[reverseIndex].containsKey("authorDetails")) {
                    strncpy(message->displayName, items[reverseIndex]["authorDetails"]["displayName"].as<const char *>(), YOUTUBE_NAME_CHAR_LENGTH);
                    message->displayName[YOUTUBE_NAME_CHAR_LENGTH -1] = '\0';
                    message->isChatModerator = items[reverseIndex]["authorDetails"]["isChatModerator"].as<bool>();
                    message->isChatOwner = items[reverseIndex]["authorDetails"]["isChatOwner"].as<bool>();
                    message->isChatSponsor = items[reverseIndex]["authorDetails"]["isChatSponsor"].as<bool>();
                    message->isVerified = items[reverseIndex]["authorDetails"]["isVerified"].as<bool>();
                } else {
                    #ifdef YOUTUBE_SERIAL_OUTPUT
                    Serial.println("no authorDetails");
                    #endif
                    message->displayName[0] = '\0';
                    message->isChatModerator = false;
                    message->isChatOwner = false;
                    message->isChatSponsor = false;
                    message->isVerified = false;
                }               
            }

            chatResponses.numMessages = numMessages;

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
    // Check HTTP status
    if(client->find("HTTP/1.1")){
        int statusCode = client->parseInt();
        return statusCode;
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
    for(int i = 0; i < YOUTUBE_MAX_RESULTS; i++){
        chatResponses.messages[i].displayMessage = (char *)malloc(YOUTUBE_MSG_CHAR_LENGTH);
        chatResponses.messages[i].displayName = (char *)malloc(YOUTUBE_NAME_CHAR_LENGTH);
        chatResponses.messages[i].currency = (char *)malloc(YOUTUBE_LIVE_CHAT_CURRENCY_LENGTH);
    }

    liveStreamDetails.concurrentViewers = (char *)malloc(YOUTUBE_VIEWERS_CHAR_LENGTH);
    liveStreamDetails.activeLiveChatId = (char *)malloc(YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH);

}

// Not sure why this would ever be needed, but sure why not.
void YouTubeLiveStream::destroyStructs()
{
    for(int i = 0; i < YOUTUBE_MAX_RESULTS; i++){
        free(chatResponses.messages[i].displayMessage);
        free(chatResponses.messages[i].displayName);
        free(chatResponses.messages[i].currency);
    }

    free(liveStreamDetails.concurrentViewers);
    free(liveStreamDetails.activeLiveChatId);
}