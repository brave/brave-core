/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <sstream>
#include <cmath>
#include <vector>
#include <memory>
#include <utility>

#include "bat/ledger/internal/bat_get_media.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/static_values.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_bat_get_media {

BatGetMedia::BatGetMedia(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

BatGetMedia::~BatGetMedia() {}

std::string BatGetMedia::GetLinkType(const std::string& url,
                                     const std::string& first_party_url,
                                     const std::string& referrer) {
  std::string type;

  const std::string mobile_api = "https://m.youtube.com/api/stats/watchtime?";
  const std::string desktop_api =
      "https://www.youtube.com/api/stats/watchtime?";

  bool is_valid_twitch_path =
      braveledger_bat_helper::HasSameDomainAndPath(
                                  url, "ttvnw.net", "/v1/segment/");

  if (url.find(mobile_api) != std::string::npos ||
      url.find(desktop_api) != std::string::npos) {
    type = YOUTUBE_MEDIA_TYPE;
  }  else if (
    (
      (first_party_url.find("https://www.twitch.tv/") == 0 ||
        first_party_url.find("https://m.twitch.tv/") == 0) ||
      (referrer.find("https://player.twitch.tv/") == 0)) &&
      is_valid_twitch_path
  ) {
    type = TWITCH_MEDIA_TYPE;
  }

  return type;
}

void BatGetMedia::processMedia(const std::map<std::string, std::string>& parts,
                               const std::string& type,
                               const ledger::VisitData& visit_data) {
  if (parts.size() == 0) {
    return;
  }

  std::string mediaId = braveledger_bat_helper::getMediaId(parts, type);
  BLOG(ledger_, ledger::LogLevel::LOG_DEBUG) << "Media Id: " << mediaId;
  if (mediaId.empty()) {
    return;
  }
  std::string media_key = braveledger_bat_helper::getMediaKey(mediaId, type);
  BLOG(ledger_, ledger::LogLevel::LOG_DEBUG) << "Media key: " << media_key;
  uint64_t duration = 0;
  ledger::TwitchEventInfo twitchEventInfo;
  if (type == YOUTUBE_MEDIA_TYPE) {
    duration = braveledger_bat_helper::getMediaDuration(parts, media_key, type);
  } else if (type == TWITCH_MEDIA_TYPE) {
    std::map<std::string, std::string>::const_iterator iter =
        parts.find("event");
    if (iter != parts.end()) {
      twitchEventInfo.event_ = iter->second;
    }
    iter = parts.find("time");
    if (iter != parts.end()) {
      twitchEventInfo.time_ = iter->second;
    }
  }
  BLOG(ledger_, ledger::LogLevel::LOG_DEBUG) << "Media duration: " << duration;

  ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&BatGetMedia::getPublisherInfoDataCallback,
                this,
                mediaId,
                media_key,
                type,
                duration,
                twitchEventInfo,
                visit_data,
                0,
                _1,
                _2));
}

void BatGetMedia::getPublisherInfoDataCallback(const std::string& mediaId,
    const std::string& media_key,
    const std::string& providerName,
    const uint64_t& duration,
    const ledger::TwitchEventInfo& twitchEventInfo,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Failed to get publisher info";
    // TODO(anyone) error handling
    return;
  }

  if (!publisher_info && !publisher_info.get()) {
    std::string mediaURL = getMediaURL(mediaId, providerName);
    if (providerName == YOUTUBE_MEDIA_TYPE) {
      auto callback = std::bind(
          &BatGetMedia::getPublisherFromMediaPropsCallback,
          this,
          duration,
          media_key,
          providerName,
          mediaURL,
          visit_data,
          window_id,
          _1,
          _2,
          _3);
      ledger_->LoadURL(
          (std::string)YOUTUBE_PROVIDER_URL + "?format=json&url=" +
          ledger_->URIEncode(mediaURL),
        std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, callback);
    } else if (providerName == TWITCH_MEDIA_TYPE) {
      if (mediaId.empty()) {
        return;
      }

      ledger::TwitchEventInfo oldEvent;
      std::map<std::string, ledger::TwitchEventInfo>::const_iterator iter =
          twitchEvents.find(media_key);
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
      updated_visit_data.provider = TWITCH_MEDIA_TYPE;

      if (mediaId.find("_vod_") != std::string::npos) {
        // VOD
        std::vector<std::string> media_props =
            braveledger_bat_helper::split(mediaId, MEDIA_DELIMITER);
        if (media_props.empty()) {
          return;
        }

        twitchMediaID = media_props[0];
        mediaUrl = getMediaURL(twitchMediaID, providerName);
        std::string oembed_url =
            (std::string)TWITCH_VOD_URL + media_props[media_props.size() - 1];
        updated_visit_data.name = twitchMediaID;
        updated_visit_data.url = mediaUrl + "/videos";

        auto callback = std::bind(
            &BatGetMedia::getPublisherFromMediaPropsCallback,
            this,
            realDuration,
            media_key,
            providerName,
            mediaUrl,
            updated_visit_data,
            window_id,
            _1,
            _2,
            _3);
        ledger_->LoadURL(
            (std::string)TWITCH_PROVIDER_URL + "?json&url=" +
            ledger_->URIEncode(oembed_url),
            std::vector<std::string>(), "", "",
            ledger::URL_METHOD::GET, callback);
        return;
      }

      // Live stream
      std::string id = providerName + "#author:" + twitchMediaID;
      updated_visit_data.name = twitchMediaID;
      updated_visit_data.url = mediaUrl + "/videos";

      ledger_->SaveMediaVisit(id, updated_visit_data, realDuration, window_id);
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
      ledger_->SaveMediaVisit(id, updated_visit_data, duration, window_id);
    } else if (providerName == TWITCH_MEDIA_TYPE) {
      updated_visit_data.provider = TWITCH_MEDIA_TYPE;
      updated_visit_data.favicon_url = publisher_info->favicon_url;

      ledger::TwitchEventInfo oldEvent;
      std::map<std::string, ledger::TwitchEventInfo>::const_iterator iter =
          twitchEvents.find(media_key);
      if (iter != twitchEvents.end()) {
        oldEvent = iter->second;
      }

      ledger::TwitchEventInfo newEvent(twitchEventInfo);
      newEvent.status_ = getTwitchStatus(oldEvent, newEvent);

      uint64_t realDuration = getTwitchDuration(oldEvent, newEvent);
      twitchEvents[media_key] = newEvent;

      std::string id = publisher_info->id;
      ledger_->SaveMediaVisit(id, updated_visit_data, realDuration, window_id);
    }
  }
}

std::string BatGetMedia::getTwitchStatus(
    const ledger::TwitchEventInfo& oldEventInfo,
    const ledger::TwitchEventInfo& newEventInfo) {
  std::string status = "playing";

  if (
    (
      newEventInfo.event_ == "video_pause" &&
      oldEventInfo.event_ != "video_pause") ||
      // User clicked pause (we need to exclude seeking while paused)
    (
      newEventInfo.event_ == "video_pause" &&
      oldEventInfo.event_ == "video_pause" &&
      oldEventInfo.status_ == "playing") ||
      // User clicked pause as soon as he clicked play
    (
      newEventInfo.event_ == "player_click_vod_seek" &&
      oldEventInfo.status_ == "paused")
      // Seeking a video while it is paused
  ) {
    status = "paused";
  }

  // User pauses a video, then seeks it and plays it again
  if (newEventInfo.event_ == "video_pause" &&
      oldEventInfo.event_ == "player_click_vod_seek" &&
      oldEventInfo.status_ == "paused") {
    status = "playing";
  }

  return status;
}

uint64_t BatGetMedia::getTwitchDuration(
    const ledger::TwitchEventInfo& oldEventInfo,
    const ledger::TwitchEventInfo& newEventInfo) {
  // Remove duplicated events
  if (oldEventInfo.event_ == newEventInfo.event_ &&
      oldEventInfo.time_ == newEventInfo.time_) {
    return 0;
  }

  if (newEventInfo.event_ == "video-play") {  // Start event
    return TWITCH_MINIMUM_SECONDS;
  }

  // TODO(anyone) check if converted properly
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
      (newEventInfo.event_ == "player_click_vod_seek" &&
       oldEventInfo.status_ == "paused") ||  // Vod seek
      (
        newEventInfo.event_ == "video_pause" &&
        (
          (
            oldEventInfo.event_ != "video_pause" &&
            oldEventInfo.event_ != "player_click_vod_seek") ||
          oldEventInfo.status_ == "playing")
      )  // User paused video
    ) {
    time = currentTime - oldTime;
  }

  if (time < 0) {
    return 0;
  }

  // if autoplay is off and play is pressed
  if (oldEventInfo.status_.empty()) {
    return 0;
  }

  if (time > TWITCH_MAXIMUM_SECONDS_CHUNK) {
    time = TWITCH_MAXIMUM_SECONDS_CHUNK;
  }

  return (uint64_t)std::round(time);
}

void BatGetMedia::getPublisherFromMediaPropsCallback(
    const uint64_t& duration,
    const std::string& media_key,
    const std::string& providerName,
    const std::string& mediaURL,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != 200) {
    // TODO(anyone): add error handler
    if (providerName == YOUTUBE_MEDIA_TYPE) {
      if (response_status_code == 401) {  // embedding disabled, need to scrape
        fetchDataFromUrl(visit_data.url,
            std::bind(&BatGetMedia::onFetchDataFromNonEmbeddable,
                      this,
                      window_id,
                      visit_data,
                      providerName,
                      duration,
                      media_key,
                      mediaURL,
                      _1,
                      _2,
                      _3));
      }
    }
    return;
  }

  if (providerName == YOUTUBE_MEDIA_TYPE) {
    std::string publisherURL;
    braveledger_bat_helper::getJSONValue("author_url", response, &publisherURL);
    std::string publisherName;
    braveledger_bat_helper::getJSONValue("author_name",
                                         response,
                                         &publisherName);

    auto callback = std::bind(&BatGetMedia::getPublisherInfoCallback,
                              this,
                              duration,
                              media_key,
                              providerName,
                              mediaURL,
                              publisherURL,
                              publisherName,
                              visit_data,
                              window_id,
                              _1,
                              _2,
                              _3);
    ledger_->LoadURL(publisherURL,
        std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, callback);
    return;
  }

  if (providerName == TWITCH_MEDIA_TYPE) {
    std::string fav_icon;
    braveledger_bat_helper::getJSONValue("author_thumbnail_url",
                                         response,
                                         &fav_icon);
    std::string author_name;
    braveledger_bat_helper::getJSONValue("author_name", response, &author_name);

    std::string twitchMediaID = visit_data.name;
    std::string id = providerName + "#author:" + twitchMediaID;

    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.name = author_name;

    if (fav_icon.length() > 0) {
      updated_visit_data.favicon_url = fav_icon;
    }

    ledger_->SaveMediaVisit(id, updated_visit_data, duration, window_id);
    ledger_->SetMediaPublisherInfo(media_key, id);
  }
}

void BatGetMedia::getPublisherInfoCallback(
    const uint64_t& duration,
    const std::string& media_key,
    const std::string& providerName,
    const std::string& mediaURL,
    const std::string& publisherURL,
    const std::string& publisherName,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code == 200 && providerName == YOUTUBE_MEDIA_TYPE) {
    std::string favIconURL = parseFavIconUrl(response);
    std::string channelId = parseChannelId(response);

    savePublisherInfo(duration,
                      media_key,
                      providerName,
                      publisherURL,
                      publisherName,
                      visit_data,
                      window_id,
                      favIconURL,
                      channelId);
  }
}

void BatGetMedia::savePublisherInfo(const uint64_t& duration,
                                    const std::string& media_key,
                                    const std::string& providerName,
                                    const std::string& publisherURL,
                                    const std::string& publisherName,
                                    const ledger::VisitData& visit_data,
                                    const uint64_t window_id,
                                    const std::string& favIconURL,
                                    const std::string& channelId) {
  std::string publisher_id;
  std::string url;
  if (providerName == YOUTUBE_MEDIA_TYPE) {
    publisher_id = providerName + "#channel:";
    if (channelId.empty()) {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
        "Channel id is missing for: " << media_key;
      return;
    }

    publisher_id += channelId;
    url = publisherURL + "/videos";
  } else if (providerName == TWITCH_MEDIA_TYPE) {
    publisher_id = providerName + "#author:";
    if (channelId.empty()) {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
        "author id is missing for: " << media_key;
      return;
    }
    publisher_id += channelId;
    url = publisherURL + "/videos";
  }

  if (publisher_id.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher id is missing for: " << media_key;
    return;
  }

  ledger::VisitData updated_visit_data(visit_data);

  if (favIconURL.length() > 0) {
    updated_visit_data.favicon_url = favIconURL;
  }

  updated_visit_data.provider = providerName;
  updated_visit_data.name = publisherName;
  updated_visit_data.url = url;

  ledger_->SaveMediaVisit(publisher_id,
                          updated_visit_data,
                          duration,
                          window_id);
  if (!media_key.empty()) {
    ledger_->SetMediaPublisherInfo(media_key, publisher_id);
  }
}

std::string BatGetMedia::getMediaURL(const std::string& mediaId,
                                     const std::string& providerName) {
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
  if (providerName == YOUTUBE_MEDIA_TYPE) {
    res = "https://www.youtube.com/channel/" + publisher_key;
  } else if (providerName == TWITCH_MEDIA_TYPE) {
    res = "https://www.twitch.tv/" + publisher_key;
  }

  return res;
}

void BatGetMedia::onMediaActivityError(const ledger::VisitData& visit_data,
                                       const std::string& providerType,
                                       uint64_t windowId) {
  std::string url;
  std::string name;
  if (providerType == YOUTUBE_MEDIA_TYPE) {
    url = YOUTUBE_TLD;
    name = YOUTUBE_MEDIA_TYPE;
  } else if (providerType == TWITCH_MEDIA_TYPE) {
    url = TWITCH_TLD;
    name = TWITCH_MEDIA_TYPE;
  }

  if (!url.empty()) {
    ledger::VisitData new_data;
    new_data.domain = url;
    new_data.url = "https://" + url;
    new_data.path = "/";
    new_data.name = name;

    ledger_->GetPublisherActivityFromUrl(windowId, new_data, std::string());
  } else {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Media activity error for " << providerType << " (name: "
        << name << ", url: " << visit_data.url << ")";
  }
}

void BatGetMedia::getMediaActivityFromUrl(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& publisher_blob) {
  if (providerType == YOUTUBE_MEDIA_TYPE) {
    processYoutubeMediaPanel(windowId, visit_data, providerType);
  } else if (providerType == TWITCH_MEDIA_TYPE) {
    processTwitchMediaPanel(windowId, visit_data, providerType, publisher_blob);
  } else {
    onMediaActivityError(visit_data, providerType, windowId);
  }
}

void BatGetMedia::processYoutubeMediaPanel(uint64_t windowId,
                                           const ledger::VisitData& visit_data,
                                           const std::string& providerType) {
  if (visit_data.path.find("/watch?") != std::string::npos) {
    processYoutubeWatchPath(windowId, visit_data, providerType);
  } else if (visit_data.path.find("/channel/") != std::string::npos) {
    processYoutubeChannelPath(windowId, visit_data, providerType);
  } else if (visit_data.path.find("/user/") != std::string::npos) {
    processYoutubeUserPath(windowId, visit_data, providerType);
  } else if (!isPredefinedYTPath(visit_data.path)) {
    processYoutubeCustomPath(windowId, visit_data, providerType, std::string());
  } else {
    onMediaActivityError(visit_data, providerType, windowId);
  }
}

bool BatGetMedia::isPredefinedYTPath(const std::string& path) const {
  std::vector<std::string> yt_paths({
      "/feed",
      "/channel",
      "/user",
      "/watch",
      "/account",
      "/gaming",
      "/playlist",
      "/premium",
      "/reporthistory",
      "/pair",
      "/account_notifications",
      "/account_playback",
      "/account_privacy",
      "/account_sharing",
      "/account_billing",
      "/account_advanced",
      "/subscription_manager",
      "/oops"
    });

  // make sure we are ignoring actual YT paths and not
  // a custom path that might start with a YT path
  std::string yt_path = getRealEnteredYTPath(path);
  for (std::string str_path : yt_paths) {
    if (yt_path == str_path) {
      return true;
    }
  }
  return false;
}

std::string BatGetMedia::getRealEnteredYTPath(const std::string& path) const {
  std::string yt_path = path.substr(0, path.find("/", 1));
  if (yt_path.empty() || yt_path == path) {
    yt_path = path.substr(0, path.find("?", 1));
    if (yt_path.empty() || yt_path == path) {
      yt_path = path.substr(0);
    }
  }
  return yt_path;
}

void BatGetMedia::processYoutubeWatchPath(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType) {
  std::string media_id = getYoutubeMediaIdFromUrl(visit_data);
  std::string media_key = getYoutubeMediaKeyFromUrl(providerType, media_id);

  if (!media_key.empty() || !media_id.empty()) {
    ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&BatGetMedia::onMediaPublisherActivity,
                this,
                _1,
                _2,
                windowId,
                visit_data,
                providerType,
                media_key,
                media_id,
                std::string()));
  } else {
    onMediaActivityError(visit_data, providerType, windowId);
  }
}

void BatGetMedia::processYoutubeCustomPath(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& publisher_key) {
  fetchPublisherDataFromDB(
      windowId,
      visit_data,
      providerType,
      publisher_key,
      std::string(),
      true);
}

void BatGetMedia::processYoutubeChannelPath(uint64_t windowId,
  const ledger::VisitData& visit_data,
  const std::string& providerType) {
  std::string publisher_key = "youtube#channel:";
  std::string key = getYoutubePublisherKeyFromUrl(visit_data.path);
  if (!key.empty()) {
    publisher_key += key;
    fetchPublisherDataFromDB(windowId,
                             visit_data,
                             providerType,
                             publisher_key,
                             std::string(),
                             false);
  } else {
    onMediaActivityError(visit_data, providerType, windowId);
  }
}

void BatGetMedia::onMediaUserActivity(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& media_key) {
  if (result != ledger::Result::LEDGER_OK  &&
      result != ledger::Result::NOT_FOUND) {
    onMediaActivityError(visit_data, providerType, windowId);
    return;
  }

  if (!info || result == ledger::Result::NOT_FOUND) {
    fetchDataFromUrl(visit_data.url,
                     std::bind(&BatGetMedia::onGetChannelIdFromUserPage,
                               this,
                               windowId,
                               visit_data,
                               providerType,
                               media_key,
                               _1,
                               _2,
                               _3));

  } else {
    fetchPublisherDataFromDB(windowId, visit_data,
      providerType, info->id, std::string(), false);
  }
}

void BatGetMedia::processYoutubeUserPath(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType) {
  std::string user = getYoutubeUserFromUrl(visit_data.path);

  if (user.empty()) {
    onMediaActivityError(visit_data, providerType, windowId);
  } else {
    std::string media_key = providerType + "_user_" + user;
    ledger_->GetMediaPublisherInfo(media_key,
      std::bind(&BatGetMedia::onMediaUserActivity,
                this,
                _1,
                _2,
                windowId,
                visit_data,
                providerType,
                media_key));
  }
}

void BatGetMedia::fetchPublisherDataFromDB(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& publisher_key,
    const std::string& publisher_blob,
    const bool is_custom_path) {
  auto filter = ledger_->CreateActivityFilter(
    publisher_key,
    ledger::EXCLUDE_FILTER::FILTER_ALL,
    false,
    ledger_->GetReconcileStamp(),
    true,
    false);
  ledger_->GetPanelPublisherInfo(filter,
    std::bind(&BatGetMedia::onFetchPublisherFromDBResponse,
              this,
              _1,
              _2,
              windowId,
              visit_data,
              providerType,
              publisher_key,
              publisher_blob,
              is_custom_path));
}

void BatGetMedia::onFetchPublisherFromDBResponse(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& publisher_key,
    const std::string& publisher_blob,
    const bool is_custom_path) {
  if (!info || (result == ledger::Result::NOT_FOUND &&
    providerType == YOUTUBE_MEDIA_TYPE)) {
    fetchDataFromUrl(visit_data.url,
        std::bind(&BatGetMedia::onGetChannelHeadlineVideo,
                  this,
                  windowId,
                  visit_data,
                  providerType,
                  _1,
                  _2,
                  _3,
                  is_custom_path));
  } else {
    if (providerType == TWITCH_MEDIA_TYPE) {
      if (info->name != visit_data.name) {
        std::string media_id = getTwitchMediaIdFromUrl(
            visit_data, publisher_blob);
        std::transform(media_id.begin(), media_id.end(),
          media_id.begin(), ::tolower);
        std::string media_key = getTwitchMediaKeyFromUrl(
            providerType,
            media_id,
            visit_data.url);
        info->name = getUserFacingHandle(publisher_blob);
        savePublisherInfo(0,
                          media_key,
                          providerType,
                          visit_data.url,
                          info->name,
                          visit_data,
                          windowId,
                          info->favicon_url,
                          media_id);
      }
    }
    ledger_->OnPanelPublisherInfo(result, std::move(info), windowId);
  }
}

void BatGetMedia::fetchDataFromUrl(const std::string& url,
                                   FetchDataFromUrlCallback callback) {
  ledger_->LoadURL(url,
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, callback);
}

void BatGetMedia::onGetChannelIdFromUserPage(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& media_key,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  std::string channelId = parseChannelId(response);
  if (!channelId.empty()) {
    std::string path = "/channel/" + channelId;
    std::string url = getPublisherUrl(channelId, providerType);
    std::string publisher_key = providerType + "#channel:" + channelId;

    ledger_->SetMediaPublisherInfo(media_key, publisher_key);

    ledger::VisitData new_data(visit_data);
    new_data.path = path;
    new_data.url = url;
    new_data.name = "";
    new_data.favicon_url = "";

    getMediaActivityFromUrl(windowId, new_data, providerType, std::string());
  } else {
    onMediaActivityError(visit_data, providerType, windowId);
  }
}

void BatGetMedia::onFetchDataFromNonEmbeddable(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const uint64_t& duration,
    const std::string& media_key,
    const std::string& mediaURL,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code != 200) {
    onMediaActivityError(visit_data, providerType, windowId);
    return;
  }

  if (providerType == YOUTUBE_MEDIA_TYPE) {
    std::string publisher_name = parsePublisherName(response);
    std::string channel_id = parseChannelId(response);
    std::string publisher_url =
        getPublisherUrl(channel_id, providerType);
    std::string favicon_url = parseFavIconUrl(response);
    savePublisherInfo(duration,
                      media_key,
                      providerType,
                      publisher_url,
                      publisher_name,
                      visit_data,
                      windowId,
                      favicon_url,
                      channel_id);
    return;
  }
}

void BatGetMedia::onGetChannelHeadlineVideo(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const bool is_custom_path) {
  if (response_status_code != 200) {
    onMediaActivityError(visit_data, providerType, windowId);
    return;
  }

  if (visit_data.path.find("/channel/") != std::string::npos) {
    std::string title = getNameFromChannel(response);
    std::string favicon = parseFavIconUrl(response);
    std::string channelId = getYoutubePublisherKeyFromUrl(visit_data.path);

    savePublisherInfo(0,
                      std::string(),
                      providerType,
                      visit_data.url,
                      title,
                      visit_data,
                      windowId,
                      favicon,
                      channelId);

  } else if (is_custom_path) {
    std::string title = getNameFromChannel(response);
    std::string favicon = parseFavIconUrl(response);
    std::string channelId = parseChannelIdFromCustomPathPage(response);
    ledger::VisitData new_visit_data(visit_data);
    new_visit_data.path = "/channel/" + channelId;
    processYoutubeCustomPath(windowId, new_visit_data, providerType,
        "youtube#channel:" + channelId);
  } else {
    onMediaActivityError(visit_data, providerType, windowId);
  }
}

void BatGetMedia::processTwitchMediaPanel(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& publisher_blob) {
  if (publisher_blob == ledger::kIgnorePublisherBlob) {
    return;
  }

  if (!publisher_blob.empty()) {
    std::string media_id = getTwitchMediaIdFromUrl(visit_data, publisher_blob);
    std::transform(media_id.begin(),
                   media_id.end(),
                   media_id.begin(),
                   ::tolower);
    std::string media_key = getTwitchMediaKeyFromUrl(providerType, media_id,
      visit_data.url);
    if (!media_key.empty() && !media_id.empty()) {
      ledger_->GetMediaPublisherInfo(
          media_key,
          std::bind(&BatGetMedia::onMediaPublisherActivity,
                    this,
                    _1,
                    _2,
                    windowId,
                    visit_data,
                    providerType,
                    media_key,
                    media_id,
                    publisher_blob));
    } else {
      onMediaActivityError(visit_data, providerType, windowId);
    }
  } else {
    ledger::VisitData new_visit_data(visit_data);
      new_visit_data.path = std::string();
      ledger_->GetPublisherActivityFromUrl(windowId,
                                           new_visit_data,
                                           std::string());
  }
}

std::string BatGetMedia::getTwitchMediaIdFromUrl(
  const ledger::VisitData& visit_data,
  const std::string& publisher_blob) const {
  std::string mediaId =
    extractData(visit_data.url, "twitch.tv/", "/");
  if (visit_data.url.find("twitch.tv/videos/") != std::string::npos) {
    mediaId = extractData(publisher_blob,
      "data-a-target=\"user-channel-header-item\" href=\"/", "\"");
  }
  return mediaId;
}

std::string BatGetMedia::getTwitchMediaKeyFromUrl(
  const std::string& provider_type,
  const std::string& id,
  const std::string& url) const {
  if (id == "twitch") {
    return std::string();
  }
  if (url.find("twitch.tv/videos/") != std::string::npos) {
    std::string vodId = extractData(url, "twitch.tv/videos/", "/");
    return provider_type + "_" + id + "_vod_" + vodId;
  }
  return provider_type + "_" + id;
}

void BatGetMedia::onMediaPublisherActivity(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& media_key,
    const std::string& media_id,
    const std::string& publisher_blob) {
  if (result != ledger::Result::LEDGER_OK &&
    result != ledger::Result::NOT_FOUND) {
    onMediaActivityError(visit_data, providerType, windowId);
    return;
  }

  if (!info || result == ledger::Result::NOT_FOUND) {
    if (providerType == TWITCH_MEDIA_TYPE) {
      // first see if we have the publisher a different way (VOD vs. live stream
      ledger_->GetPublisherInfo("twitch#author:" + media_id,
        std::bind(&BatGetMedia::onGetTwitchPublisherInfo,
                  this,
                  _1,
                  _2,
                  windowId,
                  visit_data,
                  providerType,
                  media_key,
                  media_id,
                  publisher_blob));
    } else if (providerType == YOUTUBE_MEDIA_TYPE) {
      ledger::TwitchEventInfo twitchEventInfo;
      getPublisherInfoDataCallback(media_id,
                                   media_key,
                                   providerType,
                                   0,
                                   twitchEventInfo,
                                   visit_data,
                                   windowId,
                                   result,
                                   std::move(info));
    }
  } else {
    if (providerType == TWITCH_MEDIA_TYPE) {
      if (info->verified && info->favicon_url.empty()) {
        std::string publisher_name;
        std::string publisher_favicon_url;
        updateTwitchPublisherData(&publisher_name,
                                  &publisher_favicon_url,
                                  publisher_blob);

        if (!publisher_favicon_url.empty()) {
          savePublisherInfo(0,
                            media_key,
                            providerType,
                            visit_data.url,
                            publisher_name,
                            visit_data,
                            windowId,
                            publisher_favicon_url,
                            media_id);
          return;
        }
      }

      ledger_->OnPanelPublisherInfo(result, std::move(info), windowId);
    } else if (providerType == YOUTUBE_MEDIA_TYPE) {
      fetchPublisherDataFromDB(windowId,
                               visit_data,
                               providerType,
                               info->id,
                               publisher_blob,
                               false);
    }
  }
}

void BatGetMedia::onGetTwitchPublisherInfo(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info,
    uint64_t windowId,
    const ledger::VisitData visit_data,
    const std::string& providerType,
    const std::string& media_key,
    const std::string& media_id,
    const std::string& publisher_blob) {
  if (result != ledger::Result::LEDGER_OK  &&
    result != ledger::Result::NOT_FOUND) {
    onMediaActivityError(visit_data, providerType, windowId);
    return;
  }
  if (!publisher_info || result == ledger::Result::NOT_FOUND) {
    if (providerType == TWITCH_MEDIA_TYPE) {
      std::string publisher_name;
      std::string publisher_favicon_url;
      updateTwitchPublisherData(&publisher_name,
                                &publisher_favicon_url,
                                publisher_blob);
      savePublisherInfo(0,
                        media_key,
                        providerType,
                        visit_data.url,
                        publisher_name,
                        visit_data,
                        windowId,
                        publisher_favicon_url,
                        media_id);
    }
  } else {
    if (providerType == TWITCH_MEDIA_TYPE) {
      ledger_->OnPanelPublisherInfo(result,
                                    std::move(publisher_info),
                                    windowId);
    }
  }
}

void BatGetMedia::updateTwitchPublisherData(
    std::string* publisher_name,
    std::string* publisher_favicon_url,
    const std::string& publisher_blob) {
  *publisher_name = getUserFacingHandle(publisher_blob);
  *publisher_favicon_url = getFaviconUrl(publisher_blob, *publisher_name);
}

std::string BatGetMedia::getUserFacingHandle(
    const std::string& publisher_blob) const {
  return extractData(publisher_blob,
    "<figure class=\"tw-avatar tw-avatar--size-36\">"
    "<div class=\"tw-border-radius-medium tw-overflow-hidden\">"
    "<img class=\"tw-avatar__img tw-image\" alt=\"", "\"");
}

std::string BatGetMedia::getFaviconUrl(
    const std::string& publisher_blob,
    const std::string& twitchHandle) const {
  return extractData(publisher_blob,
    "<figure class=\"tw-avatar tw-avatar--size-36\">"
    "<div class=\"tw-border-radius-medium tw-overflow-hidden\">"
    "<img class=\"tw-avatar__img tw-image\" alt=\"" + twitchHandle + "\" "
    "src=\"",
    "\"");
}

std::string BatGetMedia::parseFavIconUrl(const std::string& data) {
  std::string favicon_url = extractData(
      data, "\"avatar\":{\"thumbnails\":[{\"url\":\"", "\"");

  if (favicon_url.empty()) {
    favicon_url = extractData(
      data, "\"width\":88,\"height\":88},{\"url\":\"", "\"");
  }

  return favicon_url;
}

std::string BatGetMedia::parseChannelId(const std::string& data) {
  std::string id = extractData(data, "\"ucid\":\"", "\"");
  if (id.empty()) {
    id = extractData(data, "HeaderRenderer\":{\"channelId\":\"", "\"");
  }

  if (id.empty()) {
    id = extractData(
        data,
        "<link rel=\"canonical\" href=\"https://www.youtube.com/channel/",
        "\">");
  }

  if (id.empty()) {
    id = extractData(
      data,
      "browseEndpoint\":{\"browseId\":\"",
      "\"");
  }

  return id;
}

std::string BatGetMedia::parsePublisherName(const std::string& data) {
  std::string publisher_name;
  std::string publisher_json_name = extractData(data, "\"author\":\"", "\"");
  std::string publisher_json = "{\"brave_publisher\":\"" +
      publisher_json_name + "\"}";
  // scraped data could come in with JSON code points added.
  // Make to JSON object above so we can decode.
  braveledger_bat_helper::getJSONValue(
      "brave_publisher", publisher_json, &publisher_name);
  return publisher_name;
}

// static
std::string BatGetMedia::getYoutubeMediaIdFromUrl(
    const ledger::VisitData& visit_data) {
  std::vector<std::string> first_split =
    braveledger_bat_helper::split(visit_data.url, '?');

  if (first_split.size() < 2) {
    return std::string();
  }

  std::vector<std::string> and_split =
    braveledger_bat_helper::split(first_split[1], '&');

  for (const auto& item : and_split) {
    std::vector<std::string> m_url = braveledger_bat_helper::split(item, '=');

    if (m_url.size() < 2) {
      continue;
    }

    if (m_url[0] == "v") {
      return m_url[1];
    }
  }

  return std::string();
}

std::string BatGetMedia::getYoutubeMediaKeyFromUrl(
  const std::string& provider_type,
  const std::string& id) {
  if (!id.empty()) {
    return provider_type + "_" + id;
  }

  return std::string();
}

// static
std::string BatGetMedia::getYoutubePublisherKeyFromUrl(
    const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  const std::string id = extractData(path + "/", "/channel/", "/");

  if (id.empty()) {
    return std::string();
  }

  std::vector<std::string> params = braveledger_bat_helper::split(id, '?');

  return params[0];
}

// static
std::string BatGetMedia::getYoutubeUserFromUrl(
    const std::string& path) {
  if (path.empty()) {
    return std::string();
  }

  const std::string id = extractData(path + "/", "/user/", "/");

  if (id.empty()) {
    return std::string();
  }

  std::vector<std::string> params = braveledger_bat_helper::split(id, '?');

  return params[0];
}

// static
std::string BatGetMedia::extractData(const std::string& data,
                                     const std::string& matchAfter,
                                     const std::string& matchUntil) {
  std::string match;
  size_t match_after_size = matchAfter.size();
  size_t data_size = data.size();

  if (data_size < match_after_size) {
    return match;
  }

  size_t startPos = data.find(matchAfter);
  if (startPos != std::string::npos) {
    startPos += match_after_size;
    size_t endPos = data.find(matchUntil, startPos);
    if (endPos != startPos) {
      if (endPos != std::string::npos && endPos > startPos) {
        match = data.substr(startPos, endPos - startPos);
      } else if (endPos != std::string::npos) {
        match = data.substr(startPos, endPos);
      } else {
        match = data.substr(startPos, std::string::npos);
      }
    }
  }

  return match;
}

std::string BatGetMedia::getNameFromChannel(const std::string& data) {
  std::string publisher_name;
  const std::string publisher_json_name =
      extractData(data, "channelMetadataRenderer\":{\"title\":\"", "\"");
  const std::string publisher_json = "{\"brave_publisher\":\"" +
      publisher_json_name + "\"}";
  // scraped data could come in with JSON code points added.
  // Make to JSON object above so we can decode.
  braveledger_bat_helper::getJSONValue(
      "brave_publisher", publisher_json, &publisher_name);
  return publisher_name;
}

std::string BatGetMedia::parseChannelIdFromCustomPathPage(
    const std::string& data) {
  return extractData(data, "{\"key\":\"browse_id\",\"value\":\"", "\"");
}

}  // namespace braveledger_bat_get_media
