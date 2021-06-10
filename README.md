# youtube-livestream-arduino

An arduino library for interacting with YouTube live streams.

**Work in progress library - expect changes!**

## Help support what I do!

I have put a lot of effort into creating Arduino libraries that I hope people can make use of. [If you enjoy my work, please consider becoming a Github sponsor!](https://github.com/sponsors/witnessmenow/)

## Library Features:

The Library supports the following features:

- Checking if a channel is live
- Check how many viewers a stream has
- Retrieve live stream messages
- Retrieve super-chats and super-stickers (Untested, no examples)

### What needs to be added:

- Documentation on getting a key and why you might want multiple keys
- Example for checking the stream status of multiple channels
- Improve examples to stop requesting messages when streams end
- Clean up library to remove debug prints etc

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
