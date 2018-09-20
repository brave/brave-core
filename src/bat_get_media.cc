/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "bat_get_media.h"

#include <sstream>

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

  std::vector<std::string> split = braveledger_bat_helper::split(mediaId, '_');
  std::string new_media_id = mediaId;
  if (!split.empty()) {
    new_media_id = split[0];
  }

  if (!publisher_info.get()) {
    std::string mediaURL = getMediaURL(new_media_id, providerName);
    if (YOUTUBE_MEDIA_TYPE == providerName) {
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
    } else if (TWITCH_MEDIA_TYPE == providerName) {
      const std::string mediaUrl = getMediaURL(new_media_id, providerName);
      std::unique_ptr<ledger::PublisherInfo> new_publisher_info(new ledger::PublisherInfo());
      new_publisher_info->favicon_url = "";
      new_publisher_info->url = mediaUrl + "/videos";
      std::string id = providerName + "#author:" + new_media_id;
      new_publisher_info->name = new_media_id;
      new_publisher_info->id = id;

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

      ledger::VisitData updated_visit_data(visit_data);
      updated_visit_data.favicon_url = new_publisher_info->favicon_url;
      updated_visit_data.provider = TWITCH_PROVIDER_NAME;
      updated_visit_data.name = new_publisher_info->name;
      updated_visit_data.url = new_publisher_info->url;
      ledger_->SaveMediaVisit(id, updated_visit_data, realDuration);
      ledger_->SetMediaPublisherInfo(media_key, id);
    }
  } else {
    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.name = publisher_info->name;
    updated_visit_data.url = publisher_info->url;
    if (YOUTUBE_MEDIA_TYPE == providerName) {
      updated_visit_data.provider = YOUTUBE_PROVIDER_NAME;
      updated_visit_data.favicon_url = publisher_info->favicon_url;
      std::string id = publisher_info->id;
      ledger_->SaveMediaVisit(id, updated_visit_data, duration);
    } else if (TWITCH_MEDIA_TYPE == providerName) {
      updated_visit_data.provider = TWITCH_PROVIDER_NAME;
      updated_visit_data.favicon_url = publisher_info->url;

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

  if (time > TWITCH_MAXIMUM_SECONDS_CHUNK) {
    time = TWITCH_MAXIMUM_SECONDS_CHUNK;
  }

  return (uint64_t)time;
}

void BatGetMedia::getPublisherFromMediaPropsCallback(const uint64_t& duration, const std::string& media_key,
    const std::string& providerName, const std::string& mediaURL, const ledger::VisitData& visit_data, 
    bool success, const std::string& response, const std::map<std::string, std::string>& headers) {
  if (success && YOUTUBE_MEDIA_TYPE == providerName) {
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
  }
}

void BatGetMedia::getPublisherInfoCallback(const uint64_t& duration, const std::string& media_key,
    const std::string& providerName, const std::string& mediaURL, const std::string& publisherURL,
    const std::string& publisherName, const ledger::VisitData& visit_data, bool success, const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (success && YOUTUBE_MEDIA_TYPE == providerName) {
    std::string favIconURL;
//    size_t pos = response.find("<div id=\"img-preload\"");
//
//    do {
//      if (pos != std::string::npos) {
//        pos = response.find("<img src=\"", pos);
//        if (pos != std::string::npos) {
//          size_t posEnd = response.find("\">", pos);
//          if (posEnd != std::string::npos) {
//            favIconURL = response.substr(pos + 10, posEnd - pos - 10);
//          }
//        }
//      } else {
//        break;
//      }
//      pos++;
//      if (pos > response.length() - 1) {
//        pos = std::string::npos;
//      }
//    } while (favIconURL.find("photo.jpg") == std::string::npos);
//    LOG(ERROR) << "publisher's picture URL == " << favIconURL;
    std::string mediaURL = publisherURL + "/videos";
    size_t pos = publisherURL.rfind("/");
    std::string publisher_id = providerName + "#channel:";
    if (pos != std::string::npos && pos < publisherURL.length() - 1) {
      publisher_id += publisherURL.substr(pos + 1);
    }
    std::unique_ptr<ledger::PublisherInfo> publisher_info(new ledger::PublisherInfo());
    publisher_info->name = publisherName;
    publisher_info->url = mediaURL;
    publisher_info->favicon_url = favIconURL;
    publisher_info->id = publisher_id;

    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.favicon_url = publisher_info->favicon_url;
    updated_visit_data.provider = YOUTUBE_PROVIDER_NAME;
    updated_visit_data.name = publisher_info->name;
    updated_visit_data.url = publisher_info->url;

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

}  // namespace braveledger_bat_get_media
