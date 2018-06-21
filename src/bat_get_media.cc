/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "bat_get_media.h"
#include "static_values.h"
#include "leveldb/db.h"
#include "url_util.h"
#include "url_canon_stdstring.h"

namespace bat_get_media {

BatGetMedia::BatGetMedia():
	level_db_(nullptr) {

	scoped_refptr<base::SequencedTaskRunner> task_runner =
   base::CreateSequencedTaskRunnerWithTraits(
       {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(FROM_HERE, base::Bind(&BatGetMedia::openMediaPublishersDB, base::Unretained(this)));
}

BatGetMedia::~BatGetMedia() {
	if (nullptr != level_db_) {
    delete level_db_;
  }
}

void BatGetMedia::openMediaPublishersDB() {
  base::FilePath dbFilePath;
  base::PathService::Get(base::DIR_HOME, &dbFilePath);
  dbFilePath = dbFilePath.Append(MEDIA_CACHE_DB_NAME);

  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, dbFilePath.value().c_str(), &level_db_);
  if (!status.ok() || !level_db_) {
    if (level_db_) {
      delete level_db_;
      level_db_ = nullptr;
    }

    LOG(ERROR) << "openMediaPublishersDB level db open error " << dbFilePath.value().c_str();
  }
}

void BatGetMedia::getPublisherFromMediaProps(const std::string& mediaId, const std::string& mediaKey, const std::string& providerName,
		const uint64_t& duration, const TWITCH_EVENT_INFO& twitchEventInfo, BatGetMedia::GetMediaPublisherInfoCallback callback) {

	// Check if the publisher's info is already cached
	DCHECK(level_db_);
	if (level_db_) {
    std::string value;
		leveldb::Status status = level_db_->Get(leveldb::ReadOptions(), mediaKey, &value);
		if (!value.empty()) {
			MEDIA_PUBLISHER_INFO publisherInfo;
			BatHelper::getJSONMediaPublisherInfo(value, publisherInfo);
			//LOG(ERROR) << "!!!from JSON " << publisherInfo.publisherName_;

			uint64_t realDuration = duration;
			if (TWITCH_MEDIA_TYPE == providerName) {
				realDuration = getTwitchDuration(publisherInfo.twitchEventInfo_, twitchEventInfo);
				//LOG(ERROR) << "!!!realDuration == " << realDuration;
				TWITCH_EVENT_INFO oldInfo = publisherInfo.twitchEventInfo_;
				publisherInfo.twitchEventInfo_ = twitchEventInfo;
				publisherInfo.twitchEventInfo_.status_ = getTwitchStatus(oldInfo, twitchEventInfo);
				saveMediaPublisherInfo(mediaKey, BatHelper::stringifyMediaPublisherInfo(publisherInfo));
				if (0 == realDuration) {
					return;
				}
			}

			callback.Run(realDuration, publisherInfo);

			return;
		}
  }

	std::string mediaURL = getMediaURL(mediaId, providerName);

	std::string mediaURLEncoded;
  url::StdStringCanonOutput mediaURLCanon(&mediaURLEncoded);
  url::EncodeURIComponent(mediaURL.c_str(), mediaURL.length(), &mediaURLCanon);
  mediaURLCanon.Complete();

  FETCH_CALLBACK_EXTRA_DATA_ST extraData;
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
		batClientWebRequest_.run((std::string)YOUTUBE_PROVIDER_URL + "?format=json&url=" + mediaURLEncoded,
	      base::Bind(&BatGetMedia::getPublisherFromMediaPropsCallback, base::Unretained(this)), 
	      std::vector<std::string>(), "", "", extraData, URL_METHOD::GET);
	} else if (TWITCH_MEDIA_TYPE == providerName) {
		MEDIA_PUBLISHER_INFO publisherInfo;
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
		publisherInfo.twitchEventInfo_.status_ = getTwitchStatus(TWITCH_EVENT_INFO(), twitchEventInfo);
		//LOG(ERROR) << "!!!publisherURL_ == " << publisherInfo.publisherURL_;
		//LOG(ERROR) << "!!!channelName_ == " << publisherInfo.channelName_;

		saveMediaPublisherInfo(mediaKey, BatHelper::stringifyMediaPublisherInfo(publisherInfo));

		uint64_t realDuration = getTwitchDuration(TWITCH_EVENT_INFO(), twitchEventInfo);
		//LOG(ERROR) << "!!!realDuration == " << realDuration;
		if (0 == realDuration) {
			return;
		}

		{
	  	std::lock_guard<std::mutex> guard(callbacks_access_mutex_);

	  	std::map<std::string, BatGetMedia::GetMediaPublisherInfoCallback>::iterator iter = mapCallbacks_.find(extraData.string5);
	  	DCHECK(iter != mapCallbacks_.end());
	  	BatGetMedia::GetMediaPublisherInfoCallback callback = iter->second;
	  	mapCallbacks_.erase(iter);
	  	callback.Run(realDuration, publisherInfo);
	  }
	}
}

std::string BatGetMedia::getTwitchStatus(const TWITCH_EVENT_INFO& oldEventInfo, const TWITCH_EVENT_INFO& newEventInfo) {
	std::string status = "playing";

	if (
		(
			newEventInfo.event_ == "video_pause" &&
			oldEventInfo.event_ != "video_pause"
		) ||	// User clicked pause (we need to exclude seeking while paused)
		(
			newEventInfo.event_ == "video_pause" &&
			oldEventInfo.event_ == "video_pause" &&
			oldEventInfo.status_ == "playing"
		) ||	// User clicked pause as soon as he clicked play
		(
			newEventInfo.event_ == "player_click_vod_seek" &&
			oldEventInfo.status_ == "paused"
		)	// Seeking a video while it is paused
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

uint64_t BatGetMedia::getTwitchDuration(const TWITCH_EVENT_INFO& oldEventInfo, const TWITCH_EVENT_INFO& newEventInfo) {
	// Remove duplicated events
	if (oldEventInfo.event_ == newEventInfo.event_ &&
			oldEventInfo.time_ == newEventInfo.time_) {
		return 0;
	}

	if (newEventInfo.event_ == "video-play") {	// Start event
		return TWITCH_MINIMUM_SECONDS * 1000;
	}

	double time = 0;
	std::stringstream tempTime(newEventInfo.time_);
  double currentTime = 0;
  tempTime >> currentTime;
  std::stringstream tempOld(oldEventInfo.time_);
  double oldTime = 0;
  tempOld >> oldTime;

  if (oldEventInfo.event_ == "video-play") {
  	time = currentTime - oldTime - TWITCH_MINIMUM_SECONDS;
  } else if (newEventInfo.event_ == "minute-watched" ||	// Minute watched
  		newEventInfo.event_ == "buffer-empty" ||	// Run out of buffer
  		newEventInfo.event_ == "video_error" ||	// Video has some problems
  		newEventInfo.event_ == "video_end" ||	// Video ended
  		(newEventInfo.event_ == "player_click_vod_seek" && oldEventInfo.status_ == "paused") ||	// Vod seek
  		(
  			newEventInfo.event_ == "video_pause" &&
  			(
  				(
  					oldEventInfo.event_ != "video_pause" &&
  					oldEventInfo.event_ != "player_click_vod_seek"
  				) ||
  				oldEventInfo.status_ == "playing"
				)
			)	// User paused video
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
		const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  //LOG(ERROR) << "!!!!getPublisherFromMediaPropsCallback response == " << response;
  if (YOUTUBE_MEDIA_TYPE == extraData.string1) {
		std::string publisherURL = BatHelper::getJSONValue("author_url", response);
		std::string publisherName = BatHelper::getJSONValue("author_name", response);
		//LOG(ERROR) << "!!!!publisherURL == " << publisherURL;
		//LOG(ERROR) << "!!!!publisherName == " << publisherName;
		FETCH_CALLBACK_EXTRA_DATA_ST newExtraData(extraData);
		newExtraData.string3 = publisherURL;
		newExtraData.string4 = publisherName;
		batClientWebRequest_.run(publisherURL,
	      base::Bind(&BatGetMedia::getPublisherInfoCallback, base::Unretained(this)), 
	      std::vector<std::string>(), "", "", newExtraData, URL_METHOD::GET);
	}
}

void BatGetMedia::getPublisherInfoCallback(bool result, const std::string& response, 
		const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
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
		MEDIA_PUBLISHER_INFO publisherInfo;
		publisherInfo.publisherName_ = extraData.string4;
		publisherInfo.publisherURL_ = extraData.string3;
		publisherInfo.favIconURL_ = favIconURL;
		publisherInfo.channelName_ = channelName;
		publisherInfo.publisher_ = publisher;

		scoped_refptr<base::SequencedTaskRunner> task_runner =
     base::CreateSequencedTaskRunnerWithTraits(
         {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  	task_runner->PostTask(FROM_HERE, base::Bind(&BatGetMedia::saveMediaPublisherInfo, base::Unretained(this),
    	extraData.string5, BatHelper::stringifyMediaPublisherInfo(publisherInfo)));

		{
	  	std::lock_guard<std::mutex> guard(callbacks_access_mutex_);

	  	std::map<std::string, BatGetMedia::GetMediaPublisherInfoCallback>::iterator iter = mapCallbacks_.find(extraData.string5);
	  	DCHECK(iter != mapCallbacks_.end());
	  	BatGetMedia::GetMediaPublisherInfoCallback callback = iter->second;
	  	mapCallbacks_.erase(iter);
	  	callback.Run(extraData.value1, publisherInfo);
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

}
