/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_GET_MEDIA_H_
#define BAT_GET_MEDIA_H_

#include "base/callback.h"
#include "bat_client_webrequest.h"
#include "bat_helper.h"
#include <string>
#include <map>
#include <mutex>

namespace leveldb {
	class DB;
}

namespace bat_get_media {

class BatGetMedia {
public:
	typedef base::Callback<void(const uint64_t&, const MEDIA_PUBLISHER_INFO&)> GetMediaPublisherInfoCallback;

	BatGetMedia();
	~BatGetMedia();

	void getPublisherFromMediaProps(const std::string& mediaId, const std::string& mediaKey, const std::string& providerName,
		const uint64_t& duration, const TWITCH_EVENT_INFO& twitchEventInfo, BatGetMedia::GetMediaPublisherInfoCallback callback);

private:
	void openMediaPublishersDB();
	std::string getMediaURL(const std::string& mediaId, const std::string& providerName);
	void getPublisherFromMediaPropsCallback(bool result, const std::string& response, 
		const FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
	void getPublisherInfoCallback(bool result, const std::string& response, 
		const FETCH_CALLBACK_EXTRA_DATA_ST& extraData);
	void saveMediaPublisherInfo(const std::string& mediaKey, const std::string& stringifiedPublisher);
	uint64_t getTwitchDuration(const TWITCH_EVENT_INFO& oldEventInfo, const TWITCH_EVENT_INFO& newEventInfo);
	std::string getTwitchStatus(const TWITCH_EVENT_INFO& oldEventInfo, const TWITCH_EVENT_INFO& newEventInfo);

	leveldb::DB* level_db_;
	bat_client::BatClientWebRequest batClientWebRequest_;
	std::map<std::string, BatGetMedia::GetMediaPublisherInfoCallback> mapCallbacks_;
	std::mutex callbacks_access_mutex_;
};

}

#endif	// BAT_GET_MEDIA_H_