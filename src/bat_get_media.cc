/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "bat_get_media.h"

#include <sstream>

#include "bat_get_media.h"
#include "bat_helper.h"
#include "ledger_impl.h"
#include "leveldb/db.h"
#include "rapidjson_bat_helper.h"
#include "static_values.h"

using namespace std::placeholders;

namespace braveledger_bat_get_media {

BatGetMedia::BatGetMedia(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger),
  level_db_(nullptr) {
}

BatGetMedia::~BatGetMedia() {}

bool BatGetMedia::EnsureInitialized() {
  if (level_db_.get()) return true;
  return Init();
}

bool BatGetMedia::Init() {
  std::string db_path;
  std::string root;
  braveledger_bat_helper::getHomeDir(root);
  braveledger_bat_helper::appendPath(root, MEDIA_CACHE_DB_NAME, db_path);

  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::DB * db_ptr = nullptr;
  leveldb::Status status = leveldb::DB::Open(options, db_path, &db_ptr);

  if (status.IsCorruption()) {
    LOG(WARNING) << "Deleting possibly-corrupt database";
    // base::DeleteFile(path_, true);
    leveldb::Status status = leveldb::DB::Open(options, db_path, &db_ptr);
  }

  bool succeded = false;
  if (!status.ok()) {
    LOG(ERROR) << "init level db open error " << db_path;
    succeded = false;
  }
  else
  {
    level_db_.reset(db_ptr);
    succeded = true;
  }

  return succeded;
}

void BatGetMedia::getPublisherFromMediaProps(const std::string& mediaId, const std::string& mediaKey, const std::string& providerName,
    uint64_t duration, const braveledger_bat_helper::TWITCH_EVENT_INFO& twitchEventInfo, braveledger_bat_helper::GetMediaPublisherInfoCallback callback) {

  // Check if the publisher's info is already cached
  if (EnsureInitialized()) {
    std::string value;
    leveldb::Status status = level_db_->Get(leveldb::ReadOptions(), mediaKey, &value);
    if (!value.empty()) {
      braveledger_bat_helper::MEDIA_PUBLISHER_INFO publisherInfo;
      braveledger_bat_helper::loadFromJson(publisherInfo, value);
      //LOG(ERROR) << "!!!from JSON " << publisherInfo.publisherName_;

      uint64_t realDuration = duration;
      if (TWITCH_MEDIA_TYPE == providerName) {
        realDuration = getTwitchDuration(publisherInfo.twitchEventInfo_, twitchEventInfo);
        //LOG(ERROR) << "!!!realDuration == " << realDuration;
        braveledger_bat_helper::TWITCH_EVENT_INFO oldInfo = publisherInfo.twitchEventInfo_;
        publisherInfo.twitchEventInfo_ = twitchEventInfo;
        publisherInfo.twitchEventInfo_.status_ = getTwitchStatus(oldInfo, twitchEventInfo);

        std::string medPubJson;
        braveledger_bat_helper::saveToJsonString(publisherInfo, medPubJson);
        saveMediaPublisherInfo(mediaKey, medPubJson);

        if (0 == realDuration) {
          return;
        }
      }

      ledger_->RunTask(std::bind(callback, realDuration, publisherInfo));
      return;
    }
  }

  std::string mediaURL = getMediaURL(mediaId, providerName);
  std::string mediaURLEncoded;
  braveledger_bat_helper::encodeURIComponent(mediaURL, mediaURLEncoded);

  {
    // TODO(bridiver) - need to find out what is going on with mapCallbacks_
    // it's keyed to the mediaKey, but every lookup just gets the last entry
    // which doesn't seem right to me
    std::lock_guard<std::mutex> guard(callbacks_access_mutex_);
    DCHECK(mapCallbacks_.find(mediaKey) == mapCallbacks_.end());
    mapCallbacks_[mediaKey] = callback;
  }
  if (YOUTUBE_MEDIA_TYPE == providerName) {
    // TODO(bridiver) - this is also an issue because we're making these calls from the IO thread
    auto request = ledger_->LoadURL((std::string)YOUTUBE_PROVIDER_URL + "?format=json&url=" + mediaURLEncoded,
      std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, &handler_);
    handler_.AddRequestHandler(std::move(request),
        std::bind(&BatGetMedia::getPublisherFromMediaPropsCallback,
        this,
        duration,
        mediaKey,
        providerName,
        mediaURL,
        _1,
        _2));
  } else if (TWITCH_MEDIA_TYPE == providerName) {
    braveledger_bat_helper::MEDIA_PUBLISHER_INFO publisherInfo;
    publisherInfo.favIconURL_ = "";
    publisherInfo.channelName_ = getMediaURL(mediaId, providerName);
    publisherInfo.publisherURL_ = publisherInfo.channelName_ + "/videos";
    publisherInfo.publisher_ = providerName + "#author:";
    size_t pos = publisherInfo.channelName_.rfind("/");
    if (pos != std::string::npos && pos < publisherInfo.channelName_.length() - 1) {
      publisherInfo.publisher_ += publisherInfo.channelName_.substr(pos + 1);
      //LOG(ERROR) << "!!!publisher == " << publisherInfo.publisher_;
    }
    publisherInfo.publisherName_ = publisherInfo.publisher_;
    publisherInfo.twitchEventInfo_ = twitchEventInfo;
    publisherInfo.twitchEventInfo_.status_ = getTwitchStatus(braveledger_bat_helper::TWITCH_EVENT_INFO(), twitchEventInfo);
    //LOG(ERROR) << "!!!publisherURL_ == " << publisherInfo.publisherURL_;
    //LOG(ERROR) << "!!!channelName_ == " << publisherInfo.channelName_;

    std::string medPubJson;
    braveledger_bat_helper::saveToJsonString(publisherInfo, medPubJson);
    saveMediaPublisherInfo(mediaKey, medPubJson);

    uint64_t realDuration = getTwitchDuration(braveledger_bat_helper::TWITCH_EVENT_INFO(), twitchEventInfo);
    //LOG(ERROR) << "!!!realDuration == " << realDuration;
    if (0 == realDuration) {
      return;
    }

    {
      std::lock_guard<std::mutex> guard(callbacks_access_mutex_);

      std::map<std::string, braveledger_bat_helper::GetMediaPublisherInfoCallback>::iterator iter = mapCallbacks_.find(mediaKey);
      DCHECK(iter != mapCallbacks_.end());
      braveledger_bat_helper::GetMediaPublisherInfoCallback callback = iter->second;
      mapCallbacks_.erase(iter);

      // TODO(bridiver) -  why are we using a different callback here than the one that was passed in?
      ledger_->RunTask(std::bind(callback, realDuration, publisherInfo));
    }
  }
}

std::string BatGetMedia::getTwitchStatus(const braveledger_bat_helper::TWITCH_EVENT_INFO& oldEventInfo, const braveledger_bat_helper::TWITCH_EVENT_INFO& newEventInfo) {
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

uint64_t BatGetMedia::getTwitchDuration(const braveledger_bat_helper::TWITCH_EVENT_INFO& oldEventInfo, const braveledger_bat_helper::TWITCH_EVENT_INFO& newEventInfo) {
  // Remove duplicated events
  if (oldEventInfo.event_ == newEventInfo.event_ &&
      oldEventInfo.time_ == newEventInfo.time_) {
    return 0;
  }

  if (newEventInfo.event_ == "video-play") {  // Start event
    return TWITCH_MINIMUM_SECONDS * 1000;
  }

  //TODO: check if converted properly
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
    const std::string& providerName, const std::string& mediaURL, bool result, const std::string& response) {
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
                  _1,
                  _2));
  }
}

void BatGetMedia::getPublisherInfoCallback(const uint64_t& duration, const std::string& mediaKey,
    const std::string& providerName, const std::string& mediaURL, const std::string& publisherURL,
    const std::string& publisherName, bool result, const std::string& response) {
  //LOG(ERROR) << "!!!!getPublisherInfoCallback == " << response;
  if (YOUTUBE_MEDIA_TYPE == providerName) {
    size_t pos = response.find("<div id=\"img-preload\"");
    std::string favIconURL;
    do {
      if (pos != std::string::npos) {
        pos = response.find("<img src=\"", pos);
        if (pos != std::string::npos) {
          size_t posEnd = response.find("\">", pos);
          if (posEnd != std::string::npos) {
            favIconURL = response.substr(pos + 10, posEnd - pos - 10);
            //LOG(ERROR) << "!!!publisher's picture try == " << favIconURL;
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
    //LOG(ERROR) << "publisher's picture URL == " << favIconURL;
    std::string channelName = publisherURL + "/videos";
    pos = publisherURL.rfind("/");
    std::string publisher = providerName + "#channel:";
    if (pos != std::string::npos && pos < publisherURL.length() - 1) {
      publisher += publisherURL.substr(pos + 1);
      //LOG(ERROR) << "!!!publisher == " << publisher;
    }
    braveledger_bat_helper::MEDIA_PUBLISHER_INFO publisherInfo;
    publisherInfo.publisherName_ = publisherName;
    publisherInfo.publisherURL_ = publisherURL;
    publisherInfo.favIconURL_ = favIconURL;
    publisherInfo.channelName_ = channelName;
    publisherInfo.publisher_ = publisher;

    std::string medPubJson;
    braveledger_bat_helper::saveToJsonString(publisherInfo, medPubJson);

    auto io_task = std::bind(&BatGetMedia::saveMediaPublisherInfo, this, mediaKey, medPubJson);
    ledger_->RunIOTask(io_task);

    {
      std::lock_guard<std::mutex> guard(callbacks_access_mutex_);

      std::map<std::string, braveledger_bat_helper::GetMediaPublisherInfoCallback>::iterator iter = mapCallbacks_.find(mediaKey);
      DCHECK(iter != mapCallbacks_.end());
      braveledger_bat_helper::GetMediaPublisherInfoCallback callback = iter->second;
      mapCallbacks_.erase(iter);
      callback(duration, publisherInfo);
    }
  }
}

void BatGetMedia::saveMediaPublisherInfo(const std::string& mediaKey, const std::string& stringifiedPublisher) {
  if (!level_db_) {
    DCHECK(false);

    return;
  }

  // Save the publisher to the database
  leveldb::Status status = level_db_->Put(leveldb::WriteOptions(), mediaKey, stringifiedPublisher);
  DCHECK(status.ok());
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
