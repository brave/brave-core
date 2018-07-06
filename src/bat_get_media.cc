/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "bat_get_media.h"

#include <sstream>

#include "bat_get_media.h"
#include "bat_helper.h"
#include "leveldb/db.h"
#include "rapidjson_bat_helper.h"
#include "static_values.h"
#include "third_party/leveldatabase/env_chromium.h"

namespace braveledger_bat_get_media {

// TODO(bridiver) - revisit this class for possible delegation to deal with
// threading and file location issues
BatGetMedia::BatGetMedia():
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

  leveldb_env::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb_env::OpenDB(options, db_path, &level_db_);

  if (status.IsCorruption()) {
    LOG(WARNING) << "Deleting possibly-corrupt database";
    // base::DeleteFile(path_, true);
    status = leveldb_env::OpenDB(options, db_path, &level_db_);
  }

  if (!status.ok()) {
    LOG(ERROR) << "init level db open error " << db_path;
    return false;
  }

  return true;
}

void BatGetMedia::getPublisherFromMediaProps(const std::string& mediaId, const std::string& mediaKey, const std::string& providerName,
    const uint64_t& duration, const braveledger_bat_helper::TWITCH_EVENT_INFO& twitchEventInfo, braveledger_bat_helper::GetMediaPublisherInfoCallback callback) {

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

      braveledger_bat_helper::run_runnable(callback, std::cref(realDuration), std::cref(publisherInfo) );
      return;
    }
  }

  std::string mediaURL = getMediaURL(mediaId, providerName);
  std::string mediaURLEncoded;
  braveledger_bat_helper::encodeURIComponent(mediaURL, mediaURLEncoded);

  braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST extraData;
  extraData.value1 = duration;
  extraData.string5 = mediaKey;
  extraData.string1 = providerName;
  extraData.string2 = mediaURL;
  {
    std::lock_guard<std::mutex> guard(callbacks_access_mutex_);
    DCHECK(mapCallbacks_.find(mediaKey) == mapCallbacks_.end());
    mapCallbacks_[mediaKey] = callback;
  }
  if (YOUTUBE_MEDIA_TYPE == providerName) {

    auto runnable = braveledger_bat_helper::bat_mem_fun_binder3(*this, &BatGetMedia::getPublisherFromMediaPropsCallback);
    braveledger_bat_helper::batClientWebRequest->run((std::string)YOUTUBE_PROVIDER_URL + "?format=json&url=" + mediaURLEncoded,
      runnable, std::vector<std::string>(), "", "", extraData, braveledger_bat_client_webrequest::URL_METHOD::GET);

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

      std::map<std::string, braveledger_bat_helper::GetMediaPublisherInfoCallback>::iterator iter = mapCallbacks_.find(extraData.string5);
      DCHECK(iter != mapCallbacks_.end());
      braveledger_bat_helper::GetMediaPublisherInfoCallback callback = iter->second;
      mapCallbacks_.erase(iter);

      braveledger_bat_helper::run_runnable (callback, std::cref(realDuration), std::cref(publisherInfo) );
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

void BatGetMedia::getPublisherFromMediaPropsCallback(bool result, const std::string& response,
    const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!!getPublisherFromMediaPropsCallback response == " << response;
  if (YOUTUBE_MEDIA_TYPE == extraData.string1) {
    std::string publisherURL;
    braveledger_bat_helper::getJSONValue("author_url", response, publisherURL);
    std::string publisherName;
    braveledger_bat_helper::getJSONValue("author_name", response, publisherName);
    //LOG(ERROR) << "!!!!publisherURL == " << publisherURL;
    //LOG(ERROR) << "!!!!publisherName == " << publisherName;
    braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST newExtraData(extraData);
    newExtraData.string3 = publisherURL;
    newExtraData.string4 = publisherName;

    auto runnable = braveledger_bat_helper::bat_mem_fun_binder3(*this, &BatGetMedia::getPublisherInfoCallback);
    braveledger_bat_helper::batClientWebRequest->run(publisherURL, runnable,
        std::vector<std::string>(), "", "", newExtraData, braveledger_bat_client_webrequest::URL_METHOD::GET);
  }
}

void BatGetMedia::getPublisherInfoCallback(bool result, const std::string& response,
    const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!!getPublisherInfoCallback == " << response;
  if (YOUTUBE_MEDIA_TYPE == extraData.string1) {
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
    std::string channelName = extraData.string3 + "/videos";
    pos = extraData.string3.rfind("/");
    std::string publisher = extraData.string1 + "#channel:";
    if (pos != std::string::npos && pos < extraData.string3.length() - 1) {
      publisher += extraData.string3.substr(pos + 1);
      //LOG(ERROR) << "!!!publisher == " << publisher;
    }
    braveledger_bat_helper::MEDIA_PUBLISHER_INFO publisherInfo;
    publisherInfo.publisherName_ = extraData.string4;
    publisherInfo.publisherURL_ = extraData.string3;
    publisherInfo.favIconURL_ = favIconURL;
    publisherInfo.channelName_ = channelName;
    publisherInfo.publisher_ = publisher;

    std::string medPubJson;
    braveledger_bat_helper::saveToJsonString(publisherInfo, medPubJson);

    auto runnable = braveledger_bat_helper::bat_mem_fun_binder(*this, &BatGetMedia::saveMediaPublisherInfo, extraData.string5, medPubJson);
    braveledger_bat_helper::PostTask(runnable);

    {
      std::lock_guard<std::mutex> guard(callbacks_access_mutex_);

      std::map<std::string, braveledger_bat_helper::GetMediaPublisherInfoCallback>::iterator iter = mapCallbacks_.find(extraData.string5);
      DCHECK(iter != mapCallbacks_.end());
      braveledger_bat_helper::GetMediaPublisherInfoCallback callback = iter->second;
      mapCallbacks_.erase(iter);
      braveledger_bat_helper::run_runnable (callback, std::cref(extraData.value1), std::cref(publisherInfo) );
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
