/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_GET_MEDIA_H_
#define BRAVELEDGER_BAT_GET_MEDIA_H_

#include <string>
#include <map>
#include <mutex>

#include "bat_helper.h"
#include "url_request_handler.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace leveldb {
class DB;
}

namespace braveledger_bat_get_media {

class BatGetMedia {
 public:

  BatGetMedia(bat_ledger::LedgerImpl* ledger);
  ~BatGetMedia();

  void getPublisherFromMediaProps(const std::string& mediaId, const std::string& mediaKey, const std::string& providerName,
      uint64_t duration, const braveledger_bat_helper::TWITCH_EVENT_INFO& twitchEventInfo, braveledger_bat_helper::GetMediaPublisherInfoCallback callback);

 private:
  bool Init();
  bool EnsureInitialized();
  std::string getMediaURL(const std::string& mediaId, const std::string& providerName);
  void getPublisherFromMediaPropsCallback(const uint64_t& duration, const std::string& mediaKey,
    const std::string& providerName, const std::string& mediaURL, bool result, const std::string& response);
  void getPublisherInfoCallback(const uint64_t& duration, const std::string& mediaKey,
    const std::string& providerName, const std::string& mediaURL, const std::string& publisherURL,
    const std::string& publisherName, bool result, const std::string& response);
  void saveMediaPublisherInfo(const std::string& mediaKey, const std::string& stringifiedPublisher);
  uint64_t getTwitchDuration(const braveledger_bat_helper::TWITCH_EVENT_INFO& oldEventInfo, const braveledger_bat_helper::TWITCH_EVENT_INFO& newEventInfo);
  std::string getTwitchStatus(const braveledger_bat_helper::TWITCH_EVENT_INFO& oldEventInfo, const braveledger_bat_helper::TWITCH_EVENT_INFO& newEventInfo);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<leveldb::DB> level_db_;
  std::map<std::string, braveledger_bat_helper::GetMediaPublisherInfoCallback> mapCallbacks_;
  std::mutex callbacks_access_mutex_;

  bat_ledger::URLRequestHandler handler_;
};

}  // namespace braveledger_bat_get_media

#endif  // BRAVELEDGER_BAT_GET_MEDIA_H_
