/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_GET_MEDIA_H_
#define BRAVELEDGER_BAT_GET_MEDIA_H_

#include <string>
#include <map>
#include <mutex>

#include "bat/ledger/ledger.h"
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
  static std::string GetLinkType(const std::string& url, const std::string& first_party_url, const std::string& referrer);

  BatGetMedia(bat_ledger::LedgerImpl* ledger);
  ~BatGetMedia();

  void processMedia(const std::map<std::string, std::string>& parts, const std::string& type, const ledger::VisitData& visit_data);

  void getPublisherFromMediaProps(const std::string& mediaId, const std::string& media_key, const std::string& providerName,
      const uint64_t& duration, const ledger::TwitchEventInfo& twitchEventInfo,
      const ledger::VisitData& visit_data);

 private:
  std::string getMediaURL(const std::string& mediaId, const std::string& providerName);
  void getPublisherFromMediaPropsCallback(const uint64_t& duration, const std::string& media_key,
    const std::string& providerName, const std::string& mediaURL, const ledger::VisitData& visit_data, 
    bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  void getPublisherInfoCallback(const uint64_t& duration, const std::string& media_key,
    const std::string& providerName, const std::string& mediaURL, const std::string& publisherURL,
    const std::string& publisherName, const ledger::VisitData& visit_data, bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);
  uint64_t getTwitchDuration(const ledger::TwitchEventInfo& oldEventInfo, const ledger::TwitchEventInfo& newEventInfo);
  std::string getTwitchStatus(const ledger::TwitchEventInfo& oldEventInfo, const ledger::TwitchEventInfo& newEventInfo);

  void getPublisherInfoDataCallback(const std::string& mediaId, const std::string& media_key, const std::string& providerName,
      const uint64_t& duration, const ledger::TwitchEventInfo& twitchEventInfo,
      const ledger::VisitData& visit_data, ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> media_publisher_info);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  bat_ledger::URLRequestHandler handler_;

  std::map<std::string, ledger::TwitchEventInfo> twitchEvents;
};

}  // namespace braveledger_bat_get_media

#endif  // BRAVELEDGER_BAT_GET_MEDIA_H_
