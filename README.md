# arduino-youtube-video-api

An arduino library to read messages from the YouTube Live stream API

WIP, expect changes!

## API Endpoints Details

| Endpoint Name    | Description                                                 | Documentation                                                               | Quota Usage             |
| ---------------- | ----------------------------------------------------------- | --------------------------------------------------------------------------- | ----------------------- |
| liveChatMessages | Gets live stream messages and super-chats                   | [Link](https://developers.google.com/youtube/v3/live/docs/liveChatMessages) | 5 (I think, not listed) |
| Videos           | Uses video ID to get "activeLiveChatId" (also viewer count) | [Link](https://developers.google.com/youtube/v3/docs/videos/list)           | 1                       |
| Search           | Can be used to get live stream video ID (but don't)         | [Link](https://developers.google.com/youtube/v3/docs/search/list)           | 100 (!!!!?)             |

## Scraping Endpoints

Ideally you would never need to scrape a webpage as it can be pretty fragile if any changes are made. But the "search" endpoint quota cost is just not practical so I guess needs must

| Page              | Description                                                 | How?                                                                                  |
| ----------------- | ----------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| Main Channel page | Checking if the channel is live and extracting the video ID | Basically searching for the existence of the "55 watching" element on the live-stream |
