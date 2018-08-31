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
  std::unique_ptr<ledger::MediaPublisherInfo> media_publisher_info) {
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
  //LOG(ERROR) << "!!!mediaId == " << mediaId;
  std::string mediaKey = braveledger_bat_helper::getMediaKey(mediaId, type);
  //LOG(ERROR) << "!!!mediaKey == " << mediaKey;
  uint64_t duration = 0;
  ledger::TwitchEventInfo twitchEventInfo;
  if (YOUTUBE_MEDIA_TYPE == type) {
    duration = braveledger_bat_helper::getMediaDuration(parts, mediaKey, type);
    //LOG(ERROR) << "!!!duration == " << duration;
  } else if (TWITCH_MEDIA_TYPE == type) {
    std::map<std::string, std::string>::const_iterator iter = parts.find("event");
    if (iter != parts.end()) {
      twitchEventInfo.event_ = iter->second;
    }
    iter = parts.find("time");
    if (iter != parts.end()) {
      twitchEventInfo.time_ = iter->second;
    }
  }

  getPublisherFromMediaProps(mediaId, mediaKey, type, duration, twitchEventInfo, visit_data);
}

void BatGetMedia::getPublisherFromMediaProps(const std::string& mediaId, const std::string& mediaKey,
   const std::string& providerName, const uint64_t& duration, 
   const ledger::TwitchEventInfo& twitchEventInfo, const ledger::VisitData& visit_data) {
  ledger_->GetMediaPublisherInfo(mediaKey, std::bind(&BatGetMedia::getPublisherInfoDataCallback, this,
                    mediaId, mediaKey, providerName, duration, twitchEventInfo, visit_data, _1, _2));
}

void BatGetMedia::getPublisherInfoDataCallback(const std::string& mediaId, const std::string& mediaKey, const std::string& providerName,
      const uint64_t& duration, const ledger::TwitchEventInfo& twitchEventInfo, const ledger::VisitData& visit_data,
      ledger::Result result, std::unique_ptr<ledger::MediaPublisherInfo> media_publisher_info) {
  if (result != ledger::Result::OK) {
    // TODO error handling
    return;
  }

  if (!media_publisher_info.get()) {
    std::string mediaURL = getMediaURL(mediaId, providerName);
    if (YOUTUBE_MEDIA_TYPE == providerName) {
      auto request = ledger_->LoadURL((std::string)YOUTUBE_PROVIDER_URL + "?format=json&url=" + ledger_->URIEncode(mediaURL),
        std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
      handler_.AddRequestHandler(std::move(request),
          std::bind(&BatGetMedia::getPublisherFromMediaPropsCallback,
          this,
          duration,
          mediaKey,
          providerName,
          mediaURL,
          visit_data,
          _1,
          _2));
    } else if (TWITCH_MEDIA_TYPE == providerName) {

      std::unique_ptr<ledger::MediaPublisherInfo> media_publisher_info(new ledger::MediaPublisherInfo(mediaKey));
      media_publisher_info->favIconURL_ = "";
      media_publisher_info->channelName_ = getMediaURL(mediaId, providerName);
      media_publisher_info->publisherURL_ = media_publisher_info->channelName_ + "/videos";
      media_publisher_info->publisher_ = providerName + "#author:";
      size_t pos = media_publisher_info->channelName_.rfind("/");
      if (pos != std::string::npos && pos < media_publisher_info->channelName_.length() - 1) {
        media_publisher_info->publisher_ += media_publisher_info->channelName_.substr(pos + 1);
        //LOG(ERROR) << "!!!publisher == " << publisherInfo.publisher_;
      }
      media_publisher_info->publisherName_ = media_publisher_info->publisher_;
      //LOG(ERROR) << "!!!publisherName == " << media_publisher_info->publisherName_;

      media_publisher_info->twitchEventInfo_ = twitchEventInfo;
      media_publisher_info->twitchEventInfo_.status_ = getTwitchStatus(ledger::TwitchEventInfo(), twitchEventInfo);

      uint64_t realDuration = getTwitchDuration(ledger::TwitchEventInfo(), twitchEventInfo);
      //LOG(ERROR) << "!!!realDuration == " << realDuration;
      if (0 == realDuration) {
        return;
      }
      ledger::VisitData updated_visit_data(visit_data);
      updated_visit_data.tld = media_publisher_info->publisherName_ + " on " + TWITCH_PROVIDER_NAME;
      updated_visit_data.favIconURL = media_publisher_info->favIconURL_;
      ledger_->SetMediaPublisherInfo(realDuration, std::move(media_publisher_info), updated_visit_data,
        std::bind(&onVisitSavedDummy, _1, _2));
    }
  } else {
    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.tld = media_publisher_info->publisherName_ + " on ";
    if (YOUTUBE_MEDIA_TYPE == providerName) {
      updated_visit_data.tld += YOUTUBE_PROVIDER_NAME;
      updated_visit_data.favIconURL = media_publisher_info->favIconURL_;
      ledger_->SaveMediaVisit(updated_visit_data, duration);
    } else if (TWITCH_MEDIA_TYPE == providerName) {
      updated_visit_data.tld += TWITCH_PROVIDER_NAME;
      updated_visit_data.favIconURL = media_publisher_info->favIconURL_;
      uint64_t realDuration = getTwitchDuration(media_publisher_info->twitchEventInfo_, twitchEventInfo);
      //LOG(ERROR) << "!!!realDuration == " << realDuration;
      ledger::TwitchEventInfo oldInfo = media_publisher_info->twitchEventInfo_;
      media_publisher_info->twitchEventInfo_ = twitchEventInfo;
      media_publisher_info->twitchEventInfo_.status_ = getTwitchStatus(oldInfo, twitchEventInfo);

      ledger_->SetMediaPublisherInfo(realDuration, std::move(media_publisher_info), updated_visit_data,
        std::bind(&onVisitSavedDummy, _1, _2));
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
    return TWITCH_MINIMUM_SECONDS * 1000;
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

  return (uint64_t)(time * 1000.0);
}

void BatGetMedia::getPublisherFromMediaPropsCallback(const uint64_t& duration, const std::string& mediaKey,
    const std::string& providerName, const std::string& mediaURL, const ledger::VisitData& visit_data, 
    bool result, const std::string& response) {
  //LOG(ERROR) << "!!!!getPublisherFromMediaPropsCallback response == " << response;
  if (YOUTUBE_MEDIA_TYPE == providerName) {
    std::string publisherURL;
    braveledger_bat_helper::getJSONValue("author_url", response, publisherURL);
    std::string publisherName;
    braveledger_bat_helper::getJSONValue("author_name", response, publisherName);
    //LOG(ERROR) << "!!!!publisherURL == " << publisherURL;
    //LOG(ERROR) << "!!!!publisherName == " << publisherName;

    auto request = ledger_->LoadURL(publisherURL,
        std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
    handler_.AddRequestHandler(std::move(request),
        std::bind(&BatGetMedia::getPublisherInfoCallback,
                  this,
                  duration,
                  mediaKey,
                  providerName,
                  mediaURL,
                  publisherURL,
                  publisherName,
                  visit_data,
                  _1,
                  _2));
  }
}

void BatGetMedia::getPublisherInfoCallback(const uint64_t& duration, const std::string& mediaKey,
    const std::string& providerName, const std::string& mediaURL, const std::string& publisherURL,
    const std::string& publisherName, const ledger::VisitData& visit_data, bool result, const std::string& response) {
  LOG(ERROR) << "!!!!getPublisherInfoCallback == " << response;
  if (YOUTUBE_MEDIA_TYPE == providerName) {
    size_t pos = response.find("<div id=\"img-preload\"");
    std::string favIconURL;
    // TODO it doesn't always work on Android
    do {
      if (pos != std::string::npos) {
        pos = response.find("<img src=\"", pos);
        if (pos != std::string::npos) {
          size_t posEnd = response.find("\">", pos);
          if (posEnd != std::string::npos) {
            favIconURL = response.substr(pos + 10, posEnd - pos - 10);
          }
        }
      } else {
        break;
      }
      pos++;
      if (pos > response.length() - 1) {
        pos = std::string::npos;
      }
    } while (favIconURL.find("photo.jpg") == std::string::npos);
    LOG(ERROR) << "publisher's picture URL == " << favIconURL;
    std::string channelName = publisherURL + "/videos";
    pos = publisherURL.rfind("/");
    std::string publisher = providerName + "#channel:";
    if (pos != std::string::npos && pos < publisherURL.length() - 1) {
      publisher += publisherURL.substr(pos + 1);
      //LOG(ERROR) << "!!!publisher == " << publisher;
    }
    std::unique_ptr<ledger::MediaPublisherInfo> media_publisher_info(new ledger::MediaPublisherInfo(mediaKey));
    //LOG(ERROR) << "!!!publisherName == " << publisherName;
    media_publisher_info->publisherName_ = publisherName;
    media_publisher_info->publisherURL_ = publisherURL;
    media_publisher_info->favIconURL_ = favIconURL;
    media_publisher_info->channelName_ = channelName;
    media_publisher_info->publisher_ = publisher;

    ledger::VisitData updated_visit_data(visit_data);
    updated_visit_data.tld = publisherName + " on " + YOUTUBE_PROVIDER_NAME;
    updated_visit_data.favIconURL = media_publisher_info->favIconURL_;
    ledger_->SetMediaPublisherInfo(duration, std::move(media_publisher_info), updated_visit_data,
      std::bind(&onVisitSavedDummy, _1, _2));
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
