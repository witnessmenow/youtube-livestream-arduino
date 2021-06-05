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

#include "ArduinoYoutubeVideoApi.h"

ArduinoYoutubeVideoApi::ArduinoYoutubeVideoApi(Client &client, char *apiToken)
{
    this->client = &client;
    this->_apiToken = apiToken;
    nextPageToken[0] = 0;
    initStructs();
}

int ArduinoYoutubeVideoApi::makeGetRequest(char *command)
{
    client->flush();
    client->setTimeout(YOUTUBE_TIMEOUT);
    if (!client->connect(YOUTUBE_VIDEO_HOST, portNumber))
    {
        Serial.println(F("Connection failed"));
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
    client->println(YOUTUBE_VIDEO_HOST);

    client->println(F("Accept: application/json"));

    client->println(F("Cache-Control: no-cache"));

    client->println();

    if (client->println() == 0)
    {
        Serial.println(F("Failed to send request"));
        return false;
    }

    int statusCode = getHttpStatusCode();
    skipHeaders();
    return statusCode;
}

char* ArduinoYoutubeVideoApi::getLiveVideoId(char *channelId){
    char command[250];
    sprintf(command, searchEndpointAndParams, channelId, _apiToken);
    if (_debug)
    {
        Serial.println(command);
    }

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = JSON_ARRAY_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 250;

    if (makeGetRequest(command) == 200)
    {
        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        DeserializationError error = deserializeJson(doc, *client);
        if (!error)
        {
            
            char* videoId = (char *) doc["items"][0]["id"]["videoId"].as<char *>();
            closeClient();
            return videoId;

        }
        else
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
    }
    closeClient();
    return NULL;
}

LiveStreamDetails ArduinoYoutubeVideoApi::getLiveChatId(char *videoId){
    char command[250];
    sprintf(command, videoLivestreamDetailsEndpoint, videoId, _apiToken);
    if (_debug)
    {
        Serial.println(command);
    }

    liveStreamDetails.error = true;

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(4) + 440;
    int statusCode = makeGetRequest(command);
    if (statusCode == 200)
    {
        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        DeserializationError error = deserializeJson(doc, *client);
        serializeJson(doc, Serial);
        if (!error)
        {
            liveStreamDetails.error = false;

            char* videoId = (char *) doc["items"][0]["id"]["videoId"].as<char *>();
            JsonObject liveStreamingDetails = doc["items"][0]["liveStreamingDetails"];


            strncpy(liveStreamDetails.concurrentViewers, liveStreamingDetails["concurrentViewers"].as<char *>(), YOUTUBE_VIEWERS_CHAR_LENGTH);
            liveStreamDetails.concurrentViewers[YOUTUBE_VIEWERS_CHAR_LENGTH -1] = '\0';

            strncpy(liveStreamDetails.activeLiveChatId,  liveStreamingDetails["activeLiveChatId"].as<char *>(), YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH);
            liveStreamDetails.activeLiveChatId[YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH -1] = '\0';

        }
        else
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
    } else {
        Serial.print("Not 200: ");
        Serial.println(statusCode);

        while (client->available() )
        {
            char c = 0;
            client->readBytes(&c, 1);
            Serial.print(c);

        }

    }
    closeClient();
    return liveStreamDetails;
}

ChatResponses ArduinoYoutubeVideoApi::getChatMessages(char *liveChatId, char *part){
    char command[300];
    sprintf(command, liveChatMessagesEndpoint, liveChatId, part, _apiToken);

    if(nextPageToken[0] != 0){
        char nextPageParam[50];
        sprintf(nextPageParam, "&pageToken=%s", nextPageToken);
        strcat(command, nextPageParam);
    }

    if (_debug)
    {
        Serial.println(command);
    }

    chatResponses.error = true;

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = 20000;
    if (makeGetRequest(command) == 200)
    {
        StaticJsonDocument<192> filter;
        filter["nextPageToken"] = true;
        filter["pollingIntervalMillis"] = true;

        JsonObject filter_pageInfo = filter.createNestedObject("pageInfo");
        filter_pageInfo["totalResults"] = true;
        filter_pageInfo["resultsPerPage"] = true;

        JsonObject filter_items_0 = filter["items"].createNestedObject();
        filter_items_0["snippet"]["displayMessage"] = true;

        JsonObject filter_items_0_authorDetails = filter_items_0.createNestedObject("authorDetails");
        filter_items_0_authorDetails["displayName"] = true;
        filter_items_0_authorDetails["isChatModerator"] = true;

        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        DeserializationError error = deserializeJson(doc, *client, DeserializationOption::Filter(filter));
        if (!error)
        {
            chatResponses.error = false;
            const char* pageToken = doc["nextPageToken"];
            strcpy(nextPageToken, pageToken);
            chatResponses.pollingIntervalMillis = doc["pollingIntervalMillis"].as<long>();
            chatResponses.totalResults = doc["pageInfo"]["totalResults"].as<int>();
            chatResponses.resultsPerPage = doc["pageInfo"]["resultsPerPage"].as<int>();
            JsonArray items = doc["items"];
            Serial.print("Got Here");
            int numMessages = YOUTUBE_MAX_RESULTS > items.size() ? items.size() : YOUTUBE_MAX_RESULTS;
            for(int i = 0; i < numMessages ; i++){
                //numMessages ++;
                int reverseIndex = items.size() - 1 - i;
                serializeJson(items[reverseIndex], Serial);
                
                strncpy(chatResponses.messages[i].displayMessage,  items[reverseIndex]["snippet"]["displayMessage"].as<char *>(), YOUTUBE_MSG_CHAR_LENGTH);
                chatResponses.messages[i].displayMessage[YOUTUBE_MSG_CHAR_LENGTH -1] = '\0';

                strncpy(chatResponses.messages[i].displayName, items[reverseIndex]["authorDetails"]["displayName"].as<char *>(), YOUTUBE_NAME_CHAR_LENGTH);
                chatResponses.messages[i].displayName[YOUTUBE_NAME_CHAR_LENGTH -1] = '\0';
                chatResponses.messages[i].isMod = items[reverseIndex]["authorDetails"]["isChatModerator"].as<bool>();
            }

            chatResponses.numMessages = numMessages;

        }
        else
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
    }
    closeClient();
    return chatResponses;
}



void ArduinoYoutubeVideoApi::skipHeaders()
{
    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client->find(endOfHeaders))
    {
        Serial.println(F("Invalid response"));
        return;
    }

    // Was getting stray characters between the headers and the body
    // This should toss them away
    while (client->available() && client->peek() != '{')
    {
        char c = 0;
        client->readBytes(&c, 1);
        if (_debug)
        {
            Serial.print("Tossing an unexpected character: ");
            Serial.println(c);
        }
    }
}

int ArduinoYoutubeVideoApi::getHttpStatusCode()
{
    // Check HTTP status
    if(client->find("HTTP/1.1")){
        int statusCode = client->parseInt();
        return statusCode;
    } 

    return -1;
}

void ArduinoYoutubeVideoApi::closeClient()
{
    if (client->connected())
    {
        if (_debug)
        {
            Serial.println(F("Closing client"));
        }
        client->stop();
    }
}

void ArduinoYoutubeVideoApi::initStructs()
{
    for(int i = 0; i < YOUTUBE_MAX_RESULTS; i++){
        chatResponses.messages[i].displayMessage = (char *)malloc(YOUTUBE_MSG_CHAR_LENGTH);
        chatResponses.messages[i].displayName = (char *)malloc(YOUTUBE_NAME_CHAR_LENGTH);
    }

    liveStreamDetails.concurrentViewers = (char *)malloc(YOUTUBE_VIEWERS_CHAR_LENGTH);
    liveStreamDetails.activeLiveChatId = (char *)malloc(YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH);

}

// Not sure why this would ever be needed, but sure why not.
void ArduinoYoutubeVideoApi::destroyStructs()
{
    for(int i = 0; i < YOUTUBE_MAX_RESULTS; i++){
        free(chatResponses.messages[i].displayMessage);
        free(chatResponses.messages[i].displayName);
    }

    free(liveStreamDetails.concurrentViewers);
    free(liveStreamDetails.activeLiveChatId);
}