/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "bat_get_media.h"

#include <sstream>
#include <cmath>

#include "bat_get_media.h"
#include "bat_helper.h"
#include "ledger_impl.h"
#include "rapidjson_bat_helper.h"
#include "static_values.h"

using namespace std::placeholders;

namespace braveledger_bat_get_media {

void onVisitSavedDummy(ledger::Result result,
  std::unique_ptr<ledger::PublisherInfo> media_publisher_info) {
// OnMediaPublisherInfoUpdated will always be called by LedgerImpl so do nothing
}

BatGetMedia::BatGetMedia(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

BatGetMedia::~BatGetMedia() {}

std::string BatGetMedia::GetLinkType(const std::string& url, const std::string& first_party_url,
  const std::string& referrer) {
  std::string type = "";

  if (url.find("https://m.youtube.com/api/stats/watchtime?") != std::string::npos
      || url.find("https://www.youtube.com/api/stats/watchtime?") != std::string::npos) {
    type = YOUTUBE_MEDIA_TYPE;
  }  else if (
    (
      (first_party_url.find("https://www.twitch.tv/") == 0 ||
        first_party_url.find("https://m.twitch.tv/") == 0) ||
      (referrer.find("https://player.twitch.tv/") == 0)
    ) &&
    (
      url.find(".ttvnw.net/v1/segment/") != std::string::npos ||
      url.find("https://ttvnw.net/v1/segment/") != std::string::npos
    )
  ) {
    type = TWITCH_MEDIA_TYPE;
  }

  return type;
}

void BatGetMedia::processMedia(const std::map<std::string, std::string>& parts, const std::string& type,
    const ledger::VisitData& visit_data) {
  if (parts.size() == 0) {
    return;
  }
  std::string mediaId = braveledger_bat_helper::getMediaId(parts, type);
  if (mediaId.empty()) {
    return;
  }
  std::string media_key = braveledger_bat_helper::getMediaKey(mediaId, type);
  uint64_t duration = 0;
  ledger::TwitchEventInfo twitchEventInfo;
  if (type == YOUTUBE_MEDIA_TYPE) {
    duration = braveledger_bat_helper::getMediaDuration(parts, media_key, type);
  } else if (type == TWITCH_MEDIA_TYPE) {
    std::map<std::string, std::string>::const_iterator iter = parts.find("event");
    if (iter != parts.end()) {
      twitchEventInfo.event_ = iter->second;
    }
    iter = parts.find("time");
    if (iter != parts.end()) {
      twitchEventInfo.time_ = iter->second;
    }
  }

  getPublisherFromMediaProps(mediaId, media_key, type, duration, twitchEventInfo, visit_data);
}

void BatGetMedia::getPublisherFromMediaProps(const std::string& mediaId, const std::string& media_key,
   const std::string& providerName, const uint64_t& duration,
   const ledger::TwitchEventInfo& twitchEventInfo, const ledger::VisitData& visit_data) {
  ledger_->GetMediaPublisherInfo(media_key, std::bind(&BatGetMedia::getPublisherInfoDataCallback, this,
                    mediaId, media_key, providerName, duration, twitchEventInfo, visit_data, _1, _2));
}

void BatGetMedia::getPublisherInfoDataCallback(const std::string& mediaId, const std::string& media_key, const std::string& providerName,
      const uint64_t& duration, const ledger::TwitchEventInfo& twitchEventInfo, const ledger::VisitData& visit_data,
      ledger::Result result, std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK && result != ledger::Result::NOT_FOUND) {
    // TODO error handling
    return;
  }

  if (!publisher_info.get()) {
    std::string mediaURL = getMediaURL(mediaId, providerName);
    if (providerName == YOUTUBE_MEDIA_TYPE) {
      auto request = ledger_->LoadURL((std::string)YOUTUBE_PROVIDER_URL + "?format=json&url=" + ledger_->URIEncode(mediaURL),
        std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
      handler_.AddRequestHandler(std::move(request),
          std::bind(&BatGetMedia::getPublisherFromMediaPropsCallback,
          this,
          duration,
          media_key,
          providerName,
          mediaURL,
          visit_data,
          _1,
          _2,
          _3));
    } else if (providerName == TWITCH_MEDIA_TYPE) {
      if (mediaId.empty()) {
        return;
      }

      ledger::TwitchEventInfo oldEvent;
      std::map<std::string, ledger::TwitchEventInfo>::const_iterator iter = twitchEvents.find(media_key);
      if (iter != twitchEvents.end()) {
        oldEvent = iter->second;
      }

      ledger::TwitchEventInfo newEvent(twitchEventInfo);
      newEvent.status_ = getTwitchStatus(oldEvent, newEvent);

      uint64_t realDuration = getTwitchDuration(oldEvent, newEvent);
      twitchEvents[media_key] = newEvent;

      if (realDuration == 0) {
        return;
      }

      std::string twitchMediaID = mediaId;
      std::string mediaUrl = getMediaURL(twitchMediaID, providerName);

      ledger::VisitData updated_visit_data(visit_data);
      updated_visit_data.favicon_url = "";
      updated_visit_data.provider = TWITCH_PROVIDER_NAME;

      if (mediaId.find("_vod_") != std::string::npos) {
        // VOD
        std::vector<std::string> media_props = braveledger_bat_helper::split(mediaId, MEDIA_DELIMITER);
        if (media_props.empty()) {
          return;
        }

        twitchMediaID = media_props[0];
        mediaUrl = getMediaURL(twitchMediaID, providerName);
        std::string oembed_url = (std::string)TWITCH_VOD_URL + media_props[media_props.size() - 1];
        updated_visit_data.name = twitchMediaID;
        updated_visit_data.url = mediaUrl + "/videos";
        auto request = ledger_->LoadURL((std::string)TWITCH_PROVIDER_URL + "?json&url=" + ledger_->URIEncode(oembed_url),
                                        std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
        handler_.AddRequestHandler(std::move(request),
                                   std::bind(&BatGetMedia::getPublisherFromMediaPropsCallback,
                                             this,
                                             realDuration,
                                             media_key,
                                             providerName,
                                             mediaUrl,
                                             updated_visit_data,
                                             _1,
                                             _2,
                                             _3));
        return;
      }

      // Live stream
      std::string id = providerName + "#author:" + twitchMediaID;
      updated_visit_data.name = twitchMediaID;
      updated_visit_data.url = mediaUrl + "/videos";

      ledger_->SaveMediaVisit(id, updated_visit_data, realDuration);
      ledger_->SetMediaPublisherInfo(media_key, id);
    }
  } else {
    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.name = publisher_info->name;
    updated_visit_data.url = publisher_info->url;
    if (providerName == YOUTUBE_MEDIA_TYPE) {
      updated_visit_data.provider = YOUTUBE_MEDIA_TYPE;
      updated_visit_data.favicon_url = publisher_info->favicon_url;
      std::string id = publisher_info->id;
      ledger_->SaveMediaVisit(id, updated_visit_data, duration);
    } else if (providerName == TWITCH_MEDIA_TYPE) {
      updated_visit_data.provider = TWITCH_PROVIDER_NAME;
      updated_visit_data.favicon_url = publisher_info->favicon_url;

      ledger::TwitchEventInfo oldEvent;
      std::map<std::string, ledger::TwitchEventInfo>::const_iterator iter = twitchEvents.find(media_key);
      if (iter != twitchEvents.end()) {
        oldEvent = iter->second;
      }

      ledger::TwitchEventInfo newEvent(twitchEventInfo);
      newEvent.status_ = getTwitchStatus(oldEvent, newEvent);

      uint64_t realDuration = getTwitchDuration(oldEvent, newEvent);
      twitchEvents[media_key] = newEvent;

      std::string id = publisher_info->id;
      ledger_->SaveMediaVisit(id, updated_visit_data, realDuration);
    }
  }
}

std::string BatGetMedia::getTwitchStatus(const ledger::TwitchEventInfo& oldEventInfo, const ledger::TwitchEventInfo& newEventInfo) {
  std::string status = "playing";

  if (
    (
      newEventInfo.event_ == "video_pause" &&
      oldEventInfo.event_ != "video_pause"
    ) ||  // User clicked pause (we need to exclude seeking while paused)
    (
      newEventInfo.event_ == "video_pause" &&
      oldEventInfo.event_ == "video_pause" &&
      oldEventInfo.status_ == "playing"
    ) ||  // User clicked pause as soon as he clicked play
    (
      newEventInfo.event_ == "player_click_vod_seek" &&
      oldEventInfo.status_ == "paused"
    )  // Seeking a video while it is paused
  ) {
    status = "paused";
  }

  // User pauses a video, then seeks it and plays it again
  if (newEventInfo.event_ == "video_pause" &&
      oldEventInfo.event_ == "player_click_vod_seek" &&
      oldEventInfo.status_ == "paused") {
    status = "playing";
  }

  //LOG(ERROR) << "!!!video status == " << status;

  return status;
}

uint64_t BatGetMedia::getTwitchDuration(const ledger::TwitchEventInfo& oldEventInfo, const ledger::TwitchEventInfo& newEventInfo) {
  // Remove duplicated events
  if (oldEventInfo.event_ == newEventInfo.event_ &&
      oldEventInfo.time_ == newEventInfo.time_) {
    return 0;
  }

  if (newEventInfo.event_ == "video-play") {  // Start event
    return TWITCH_MINIMUM_SECONDS;
  }

  // TODO: check if converted properly
  double time = 0;
  std::stringstream tempTime(newEventInfo.time_);
  double currentTime = 0;
  tempTime >> currentTime;
  std::stringstream tempOld(oldEventInfo.time_);
  double oldTime = 0;
  tempOld >> oldTime;

  if (oldEventInfo.event_ == "video-play") {
    time = currentTime - oldTime - TWITCH_MINIMUM_SECONDS;
  } else if (newEventInfo.event_ == "minute-watched" ||  // Minute watched
      newEventInfo.event_ == "buffer-empty" ||  // Run out of buffer
      newEventInfo.event_ == "video_error" ||  // Video has some problems
      newEventInfo.event_ == "video_end" ||  // Video ended
      (newEventInfo.event_ == "player_click_vod_seek" && oldEventInfo.status_ == "paused") ||  // Vod seek
      (
        newEventInfo.event_ == "video_pause" &&
        (
          (
            oldEventInfo.event_ != "video_pause" &&
            oldEventInfo.event_ != "player_click_vod_seek"
          ) ||
          oldEventInfo.status_ == "playing"
        )
      )  // User paused video
    ) {
    time = currentTime - oldTime;
  }

  if (time < 0) {
    return 0;
  }

  if (oldEventInfo.status_.empty()) { // if autoplay is off and play is pressed
    return 0;
  }

  if (time > TWITCH_MAXIMUM_SECONDS_CHUNK) {
    time = TWITCH_MAXIMUM_SECONDS_CHUNK;
  }

  return (uint64_t)std::round(time);
}

void BatGetMedia::getPublisherFromMediaPropsCallback(const uint64_t& duration, const std::string& media_key,
    const std::string& providerName, const std::string& mediaURL, const ledger::VisitData& visit_data,
    bool success, const std::string& response, const std::map<std::string, std::string>& headers) {
  if (!success) {
    // TODO add error handler
    return;
  }

  if (providerName == YOUTUBE_MEDIA_TYPE) {
    std::string publisherURL;
    braveledger_bat_helper::getJSONValue("author_url", response, publisherURL);
    std::string publisherName;
    braveledger_bat_helper::getJSONValue("author_name", response, publisherName);

    auto request = ledger_->LoadURL(publisherURL,
        std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
    handler_.AddRequestHandler(std::move(request),
        std::bind(&BatGetMedia::getPublisherInfoCallback,
                  this,
                  duration,
                  media_key,
                  providerName,
                  mediaURL,
                  publisherURL,
                  publisherName,
                  visit_data,
                  _1,
                  _2,
                  _3));
    return;
  }

  if (providerName == TWITCH_MEDIA_TYPE) {
    std::string fav_icon;
    braveledger_bat_helper::getJSONValue("author_thumbnail_url", response, fav_icon);
    std::string author_name;
    braveledger_bat_helper::getJSONValue("author_name", response, author_name);

    std::string twitchMediaID = visit_data.name;
    std::string id = providerName + "#author:" + twitchMediaID;

    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.favicon_url = "https://" + ledger_->GenerateGUID() + ".invalid";
    updated_visit_data.name = author_name;

    if (fav_icon.length() > 0) {
      ledger_->FetchFavIcon(fav_icon, updated_visit_data.favicon_url);
    }

    ledger_->SaveMediaVisit(id, updated_visit_data, duration);
    ledger_->SetMediaPublisherInfo(media_key, id);
  }
}

void BatGetMedia::getPublisherInfoCallback(const uint64_t& duration, const std::string& media_key,
    const std::string& providerName, const std::string& mediaURL, const std::string& publisherURL,
    const std::string& publisherName, const ledger::VisitData& visit_data, bool success, const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (success &&  providerName == YOUTUBE_MEDIA_TYPE) {

    std::string favIconURL = parseFavIconUrl(response);
    std::string channelId = parseChannelId(response);

    std::string publisher_id = providerName + "#channel:";
    if (channelId.empty()) {
      return;
    }
    publisher_id += channelId;

    if (!favIconURL.empty()) {
      std::string favicon_key = "https://" + ledger_->GenerateGUID() + ".invalid";
      ledger_->FetchFavIcon(favIconURL, favicon_key);
      favIconURL = favicon_key;
    }

    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.favicon_url = favIconURL;
    updated_visit_data.provider = providerName;
    updated_visit_data.name = publisherName;
    updated_visit_data.url = publisherURL + "/videos";

    ledger_->SaveMediaVisit(publisher_id, updated_visit_data, duration);
    ledger_->SetMediaPublisherInfo(media_key, publisher_id);
  }
}

std::string BatGetMedia::getMediaURL(const std::string& mediaId, const std::string& providerName) {
  std::string res;

  DCHECK(!mediaId.empty());
  if (YOUTUBE_MEDIA_TYPE == providerName) {
    res = "https://www.youtube.com/watch?v=" + mediaId;
  } else if (TWITCH_MEDIA_TYPE == providerName) {
    res = "https://www.twitch.tv/" + mediaId;
  }

  return res;
}

std::string BatGetMedia::getPublisherUrl(const std::string& publisher_key,
  const std::string& providerName) {
  std::string res;
  DCHECK(!publisher_key.empty());
  if (YOUTUBE_MEDIA_TYPE == providerName) {
    res = "https://www.youtube.com/channel/" + publisher_key;
  } else if (TWITCH_MEDIA_TYPE == providerName) {
    res = "https://www.twitch.tv/" + publisher_key;
  }

  return res;
}

void BatGetMedia::getMediaActivityFromUrl(
  uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  bool altPath) {
  if (providerType == YOUTUBE_MEDIA_TYPE) {
    processYoutubeMediaPanel(windowId, visit_data, providerType, altPath);
  } else if (providerType == TWITCH_MEDIA_TYPE) {
    processTwitchMediaPanel(windowId, visit_data, providerType);
  } else { // treat as regular page
    ledger_->GetPublisherActivityFromUrl(windowId, visit_data);
  }
}

void BatGetMedia::processYoutubeMediaPanel(uint64_t windowId,
  const ledger::VisitData& visit_data, const std::string& providerType,
  bool altPath) {
  if (visit_data.path.find("/watch") != std::string::npos) {
    processYoutubeWatchPath(windowId, visit_data, providerType);
  } else if (visit_data.path.find("/channel") != std::string::npos) {
    processYoutubeChannelPath(windowId, visit_data, providerType, altPath);
  } else if (visit_data.path.find("/user") != std::string::npos) {
    processYoutubeUserPath(windowId, visit_data, providerType);
  } else {
    processYoutubeAlternatePath(windowId, visit_data, providerType, altPath);
  }
}

void BatGetMedia::processTwitchMediaPanel(uint64_t windowId,
  const ledger::VisitData& visit_data, const std::string& providerType) {
  std::string media_key;
}

void BatGetMedia::onCheckTwitchResponse(
  bool success, const std::string& response,
  const std::map<std::string, std::string>& headers) {
    LOG(ERROR) << "=============TWITCH RESPONSE: " << response;
}

void BatGetMedia::processYoutubeWatchPath(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType) {
  std::string media_key = getYoutubeMediaKeyFromUrl(providerType, visit_data);
  if (!media_key.empty()) {
    ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&BatGetMedia::onMediaPublisherActivity,
      this, _1, _2, windowId, visit_data,
      providerType, media_key));
  }
}

void BatGetMedia::processYoutubeChannelPath(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  bool altPath) {
  std::string publisher_key = "youtube#channel:";
  std::string key = getYoutubePublisherKeyFromUrl(providerType, visit_data);
  if (!key.empty()) {
    publisher_key += key;
    fetchPublisherDataFromDB(windowId, visit_data,
      providerType, publisher_key, altPath);
  } else {
    // get headliner video media key and see if we have it
    fetchPublisherDataFromUrl(windowId, visit_data, providerType, altPath);
  }
}

void BatGetMedia::processYoutubeUserPath(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType) {
  fetchPublisherDataFromUserUrl(windowId, visit_data, providerType);
}

void BatGetMedia::processYoutubeAlternatePath(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  bool altPath) {
  fetchPublisherDataFromUrl(windowId, visit_data, providerType, altPath);
}

void BatGetMedia::fetchPublisherDataFromDB(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  const std::string& publisher_key,
  bool altPath) {
  if (altPath) {
    auto filter = ledger_->CreatePublisherFilter(
      publisher_key, ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      visit_data.local_month, visit_data.local_year,
      ledger::PUBLISHER_EXCLUDE::ALL, false, 0);
    ledger_->GetPublisherInfo(filter,
      std::bind(&BatGetMedia::onFetchPublisherFromDBResponse,
      this, _1, _2, windowId, visit_data, providerType, publisher_key, altPath));
  } else {
    ledger_->GetMediaPublisherInfo(publisher_key,
      std::bind(&BatGetMedia::onFetchPublisherFromDBResponse,
      this, _1, _2, windowId, visit_data, providerType, publisher_key, altPath));
  }
}

void BatGetMedia::onFetchPublisherFromDBResponse(
  ledger::Result result,
  std::unique_ptr<ledger::PublisherInfo> info,
  uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  const std::string& publisher_key,
  bool altPath) {
  if (result == ledger::Result::NOT_FOUND) {
    fetchPublisherDataFromUrl(windowId, visit_data, providerType, altPath);
  } else {
    ledger_->OnPublisherActivity(result, std::move(info), windowId);
  }
}

void BatGetMedia::fetchPublisherDataFromUrl(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  bool altPath) {
  auto request = ledger_->LoadURL(visit_data.url,
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request),
    std::bind(&BatGetMedia::onGetChannelHeadlineVideo,
    this, windowId, visit_data, providerType, altPath, _1,
    _2, _3));
    //
}

void BatGetMedia::fetchPublisherDataFromUserUrl(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType) {
  auto request = ledger_->LoadURL(visit_data.url,
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request),
    std::bind(&BatGetMedia::onGetExternalIdFromUserPage,
    this, windowId, visit_data, providerType, _1,
    _2, _3));
}

void BatGetMedia::onGetExternalIdFromUserPage(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  bool success, const std::string& response,
  const std::map<std::string, std::string>& headers) {
  processYoutubeAsPublisherType(response, windowId, visit_data, providerType);
}

void BatGetMedia::onGetChannelHeadlineVideo(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  bool altPath, bool success, const std::string& response,
  const std::map<std::string, std::string>& headers) {
  if (visit_data.path.find("/channel") != std::string::npos ||
    visit_data.path.find("/user") != std::string::npos) {
    if (channelHasAuthor(response) || altPath) {
      processYoutubeAsMediaType(response, windowId, visit_data, providerType);
      return;
    }
  } else {
    processYoutubeAsPublisherType(response, windowId, visit_data,
      providerType, altPath);
      return;
  }
  ledger_->GetPublisherActivityFromUrl(windowId, visit_data);
}


void BatGetMedia::processYoutubeAsPublisherType(
  const std::string& data,
  uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  bool altPath) {
  std::string channelId = parsePublisherLink(data);
  if (!channelId.empty()) {
    std::string newPath = "/channel";
    if (visit_data.path.find("/user") == std::string::npos) {
      altPath = true;
    }
    std::string publisherUrl = getPublisherUrl(channelId, providerType);
    getMediaActivityFromUrl(windowId, visit_data, providerType, altPath);
  } else {
    ledger_->GetPublisherActivityFromUrl(windowId, visit_data);
  }
}

void BatGetMedia::processYoutubeAsMediaType(
  const std::string& data,
  uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType) {
  std::string media_key = parseHeadlineMediaKey(data);
  ledger::VisitData new_visit_data(visit_data);
  if (!media_key.empty()) {
    new_visit_data.url = getMediaURL(media_key, providerType);
    new_visit_data.path = "/watch";
    processYoutubeWatchPath(windowId, new_visit_data, providerType);
  } else {
    ledger_->GetPublisherActivityFromUrl(
      windowId, visit_data);
  }
}

void BatGetMedia::onMediaPublisherActivity(ledger::Result result,
  std::unique_ptr<ledger::PublisherInfo> info,
  uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  const std::string& media_key) {

  if (ledger::Result::LEDGER_OK != result &&
    ledger::Result::NOT_FOUND != result) {
    return; // TODO(jsadler) handle error
  }
  if (ledger::Result::NOT_FOUND == result) {
    createMediaVisit(
      windowId, visit_data, providerType, media_key);
  } else {
    ledger_->OnPublisherActivity(result, std::move(info), windowId);
  }
}

void BatGetMedia::createMediaVisit(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  const std::string& media_key) {
    fetchYoutubeEmbedInfo(windowId, visit_data, providerType, media_key);
  }

void BatGetMedia::fetchYoutubeEmbedInfo(
  uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  const std::string& media_key) {
  auto request = ledger_->LoadURL((std::string)YOUTUBE_PROVIDER_URL +
    "?format=json&url=" + ledger_->URIEncode(visit_data.url),
    std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
  handler_.AddRequestHandler(std::move(request),
    std::bind(&BatGetMedia::onGetMediaPublisher,
    this, windowId, visit_data, providerType,
    media_key, _1, _2, _3));
}

void BatGetMedia::onGetMediaPublisher(
  uint64_t windowId, const ledger::VisitData& visit_data,
  const std::string& providerType,
  const std::string& media_key, bool success,
  const std::string& response,
  const std::map<std::string, std::string>& headers) {
  if (providerType == YOUTUBE_MEDIA_TYPE) {
    std::string publisherURL;
    if (!success) {
      return; // TODO(jsadler) handle error
    }
    braveledger_bat_helper::getJSONValue(
      "author_url", response, publisherURL);
    std::string publisherName;
    braveledger_bat_helper::getJSONValue(
      "author_name", response, publisherName);

    auto request = ledger_->LoadURL(publisherURL,
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET, &handler_);
    handler_.AddRequestHandler(std::move(request),
      std::bind(&BatGetMedia::onGetChannelInfo, this, windowId, visit_data,
      providerType, publisherURL, publisherName,
      media_key, _1, _2, _3));
  }
}

void BatGetMedia::onGetChannelInfo(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  const std::string publisherURL,
  const std::string& publisherName, const std::string& media_key, bool success,
  const std::string& response,
  const std::map<std::string, std::string>& headers) {
  if (success) {

    std::string favIconURL = parseFavIconUrl(response);
    std::string publisher_name = publisherName;
    if (publisherName.empty()) {
      publisher_name = parsePublisherName(response);
    }

    std::string pubUrl = publisherURL + "/videos";
    std::string publisher_id = parsePublisherId(publisherURL, providerType);
    saveMediaVisit(windowId, visit_data, publisher_name, pubUrl,
      providerType, favIconURL, publisher_id, media_key,
      std::bind(&BatGetMedia::getMediaActivity, this, windowId,
      visit_data, providerType, _1));
  }
}

void BatGetMedia::getMediaActivity(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType,
  ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    return; // TODO(jsadler) handle error
  }
  getMediaActivityFromUrl(windowId, visit_data, providerType);
}

void BatGetMedia::saveMediaVisit(uint64_t windowId,
  const ledger::VisitData& visit_data, const std::string& providerType,
  const std::string publisherName, const std::string& publisherURL,
  const std::string& favIconURL, const std::string& publisher_id,
  const std::string& media_key, MediaPublisherInfoCallback callback) {
  ledger::VisitData media_visit_data(YOUTUBE_TLD, YOUTUBE_TLD,
    visit_data.path, -1, visit_data.local_month, visit_data.local_year, publisherName,
    publisherURL, providerType, favIconURL);
  ledger_->SaveMediaVisit(publisher_id, media_visit_data, 0);
  ledger_->SetMediaPublisherInfo(media_key, publisher_id);
  callback(ledger::Result::LEDGER_OK);
}


std::string BatGetMedia::parsePublisherId(const std::string& publisherURL,
  const std::string& provider_name) {
  size_t pos = publisherURL.rfind("/");
  std::string publisher_id = provider_name + "#channel:";
  if (pos != std::string::npos && pos < publisherURL.length() - 1) {
    publisher_id += publisherURL.substr(pos + 1);
  }
  return publisher_id;
}

std::string BatGetMedia::parseHeadlineMediaKey(const std::string& data) {
  return extractData(data, "\"videoId\":\"", "\"");
}

std::string BatGetMedia::parsePublisherLink(const std::string& data) {
  std::string externalId = extractData(data,
    "externalId\":\"", "\"");
  return externalId;
}

std::string BatGetMedia::parsePublisherName(const std::string& data) {
  return extractData(data, "\"author\":\"", "\"");
}

std::string BatGetMedia::parseFavIconUrl(const std::string& data) {
  std::string favicon_url = extractData(data,
    "\"avatar\":{\"thumbnails\":[{\"url\":\"", "\"");
  if (favicon_url.length() > 0) {
    std::string favicon_key = "https://" + ledger_->GenerateGUID() +
      ".invalid";
    ledger_->FetchFavIcon(favicon_url, favicon_key);
    favicon_url = favicon_key;
  }
  return favicon_url;
}

std::string BatGetMedia::parseChannelId(const std::string& data) {
  return extractData(data, "\"ucid\":\"", "\"");
}

bool BatGetMedia::channelHasAuthor(const std::string& data) {
  return !extractData(data, "\"author\":\"", "\"").empty();
}

std::string BatGetMedia::getYoutubeMediaKeyFromUrl(
  const std::string& provider_type,
  const ledger::VisitData& visit_data) {
  std::vector<std::string> m_url =
    braveledger_bat_helper::split(visit_data.url, '=');
  if (m_url.size() > 1) {
    return provider_type + "_" + m_url[1];
  } else {
    // TODO(jsadler) handle error
    return std::string();
  }
  return std::string();
}

std::string BatGetMedia::getYoutubePublisherKeyFromUrl(
  const std::string& provider_type,
  const ledger::VisitData& visit_data) {
  return extractData(visit_data.url, "/channel/", "/");
}

void BatGetMedia::getTwitchMediaKey(uint64_t windowId,
const ledger::VisitData& visit_data,
const std::string& provider_type) {
  LOG(ERROR) << "======+++++++TWITCH PATH: " << visit_data.path;
  if (provider_type == TWITCH_MEDIA_TYPE) {
    if (visit_data.path.find("/videos") != std::string::npos) {
      //vod
    } else {

    }
  }
}

std::string BatGetMedia::extractData(const std::string& data,
  const std::string& matchAfter, const std::string& matchUntil) {
  std::string match;
  if (data.find(matchAfter) != std::string::npos) {
    size_t startPos = data.find(matchAfter) + matchAfter.size();
    if (startPos != std::string::npos) {
      size_t endPos = data.find(matchUntil, startPos + matchAfter.size());
      if (endPos != std::string::npos && endPos > startPos) {
        match = data.substr(startPos, endPos - startPos);
      } else {
        match = data.substr(startPos, endPos);
      }
    }
  }
  return match;
}

std::string BatGetMedia::getPublisherKeyFromUrl(const std::string& data) {
  return extractData(data, "/channel/", "/");
}

}  // namespace braveledger_bat_get_media
