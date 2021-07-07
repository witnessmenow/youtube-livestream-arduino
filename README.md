# youtube-livestream-arduino

An arduino library for interacting with YouTube live streams.

**Work in progress library - expect changes!**

## Help support what I do!

I have put a lot of effort into creating Arduino libraries that I hope people can make use of. [If you enjoy my work, please consider becoming a Github sponsor!](https://github.com/sponsors/witnessmenow/)

## Library Features:

The Library supports the following features:

- Checking if a channel is live
- Check how many viewers a stream has
- Retrieve live stream messages (Realistically ESP32 only)
- Retrieve super-chats and super-stickers (Realistically ESP32 only)

### What needs to be added:

- Example for checking the stream status of multiple channels
- Improve examples to stop requesting messages when streams end

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

Check out the [useMultipleTokens](examples/ESP32/useMultipleTokens/useMultipleTokens.ino) example to see how to use multiple keys.

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
