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
		const uint64_t& duration, BatGetMedia::GetMediaPublisherInfoCallback callback) {

	// Check if the publisher's info is already cached
	DCHECK(level_db_);
	if (level_db_) {
    std::string value;
		leveldb::Status status = level_db_->Get(leveldb::ReadOptions(), mediaKey, &value);
		LOG(ERROR) << "!!!status == " << status.ToString();
		//DCHECK(status.ok());
		LOG(ERROR) << "!!!value == " << value;
		if (!value.empty()) {
			MEDIA_PUBLISHER_INFO publisherInfo;
			BatHelper::getJSONMediaPublisherInfo(value, publisherInfo);
			LOG(ERROR) << "!!!from JSON " << publisherInfo.publisherName_;
			callback.Run(duration, publisherInfo);

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
		// TODO
	}
}

void BatGetMedia::getPublisherFromMediaPropsCallback(bool result, const std::string& response, 
		const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
  LOG(ERROR) << "!!!!getPublisherFromMediaPropsCallback response == " << response;
  if (YOUTUBE_MEDIA_TYPE == extraData.string1) {
		std::string publisherURL = BatHelper::getJSONValue("author_url", response);
		std::string publisherName = BatHelper::getJSONValue("author_name", response);
		LOG(ERROR) << "!!!!publisherURL == " << publisherURL;
		LOG(ERROR) << "!!!!publisherName == " << publisherName;
		FETCH_CALLBACK_EXTRA_DATA_ST newExtraData(extraData);
		newExtraData.string3 = publisherURL;
		newExtraData.string4 = publisherName;
		batClientWebRequest_.run(publisherURL,
	      base::Bind(&BatGetMedia::getPublisherInfoCallback, base::Unretained(this)), 
	      std::vector<std::string>(), "", "", newExtraData, URL_METHOD::GET);
	} else if (TWITCH_MEDIA_TYPE == extraData.string1) {
		// TODO
	}
}

void BatGetMedia::getPublisherInfoCallback(bool result, const std::string& response, 
		const FETCH_CALLBACK_EXTRA_DATA_ST& extraData) {
	//LOG(ERROR) << "!!!!getPublisherInfoCallback == " << response;
	LOG(ERROR) << "!!!!getPublisherInfoCallback response.length() == " << response.length();
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
		LOG(ERROR) << "publisher's picture URL == " << favIconURL;
		std::string channelName = extraData.string3 + "/videos";
		pos = extraData.string3.rfind("/");
		std::string publisher = extraData.string1 + "#channel:";
		if (pos != std::string::npos && pos < extraData.string3.length() - 1) {
			publisher += extraData.string3.substr(pos + 1);
			LOG(ERROR) << "!!!publisher == " << publisher;
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
	} else if (TWITCH_MEDIA_TYPE == extraData.string1) {
		// TODO
	}
	// TODO implement the fetch of a photo and channel name
	//std::string title = BatHelper::getHTMLItem(response, /*"og:title"*/"div id=\"mi");
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
		// TODO
	}

	return res;
}

}