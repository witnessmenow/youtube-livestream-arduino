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

    LiveStreamDetails details;
    details.error = true;

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(4) + 440;
    if (makeGetRequest(command) == 200)
    {
        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        DeserializationError error = deserializeJson(doc, *client);
        if (!error)
        {
            details.error = false;

            char* videoId = (char *) doc["items"][0]["id"]["videoId"].as<char *>();
            JsonObject liveStreamingDetails = doc["items"][0]["liveStreamingDetails"];
            details.concurrentViewers  = (char *) liveStreamingDetails["concurrentViewers"].as<char *>();
            details.activeLiveChatId = (char *) liveStreamingDetails["activeLiveChatId"].as<char *>();

        }
        else
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
    }
    closeClient();
    return details;
}

ChatResponses ArduinoYoutubeVideoApi::getChatMessages(char *liveChatId, char *part){
    char command[250];
    sprintf(command, liveChatMessagesEndpoint, liveChatId, part, YOUTUBE_MAX_RESULTS, _apiToken);
    if (_debug)
    {
        Serial.println(command);
    }
    ChatResponses responses;
    responses.error = true;

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = 15000;
    if (makeGetRequest(command) == 200)
    {
        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        DeserializationError error = deserializeJson(doc, *client);
        if (!error)
        {
            responses.error = false;
            const char* pageToken = doc["nextPageToken"];
            strcpy(nextPageToken, pageToken);
            responses.pollingIntervalMillis = doc["pollingIntervalMillis"].as<long>();
            responses.totalResults = doc["pageInfo"]["totalResults"].as<int>();
            responses.resultsPerPage = doc["pageInfo"]["resultsPerPage"].as<int>();
            JsonArray items = doc["items"];
            for(int i = 0; i < items.size(); i++){
                
                responses.messages[i].displayMessage = (char *) items[i]["snippet"]["displayMessage"].as<char *>();
                responses.messages[i].displayName = (char *) items[i]["authorDetails"]["displayName"].as<char *>();
                responses.messages[i].isMod = items[i]["authorDetails"]["isChatModerator"].as<bool>();
            }

        }
        else
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
    }
    closeClient();
    return responses;
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