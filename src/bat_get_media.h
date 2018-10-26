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

using MediaPublisherInfoCallback = std::function<void(ledger::Result)>;

class BatGetMedia {
 public:
  static std::string GetLinkType(const std::string& url, const std::string& first_party_url, const std::string& referrer);

  BatGetMedia(bat_ledger::LedgerImpl* ledger);
  ~BatGetMedia();

  void processMedia(const std::map<std::string, std::string>& parts, const std::string& type, const ledger::VisitData& visit_data);

  void getPublisherFromMediaProps(const std::string& mediaId, const std::string& media_key, const std::string& providerName,
      const uint64_t& duration, const ledger::TwitchEventInfo& twitchEventInfo,
      const ledger::VisitData& visit_data);
  void getMediaActivityFromUrl(uint64_t windowId,
                               const ledger::VisitData& visit_data,
                               const std::string& providerType,
                               bool altPath = false);

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

  void getMediaActivity(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    ledger::Result result);

  void onMediaPublisherActivity(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& media_key);

  void createMediaVisit(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& media_key);

  void onGetChannelInfo(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string publisherURL,
    const std::string& publisherName, const std::string& media_key,
    bool success, const std::string& response,
    const std::map<std::string, std::string>& headers);

  void onGetExternalIdFromUserPage(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    bool success, const std::string& response,
    const std::map<std::string, std::string>& headers);

  void onGetMediaPublisher(
    uint64_t windowId, const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& media_key, bool success,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void onGetMediaActivityFromUrl(bool success,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& providerType,
    const std::string& url,
    uint64_t windowId);

  void saveMediaVisit(uint64_t windowId, const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string publisherName,
    const std::string& publisherURL,
    const std::string& favIconURL, const std::string& publisher_id,
    const std::string& media_key, MediaPublisherInfoCallback callback);

  void processYoutubeMediaPanel(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    bool altPath);

  void processTwitchMediaPanel(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType);

  void processYoutubeWatchPath(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType);

  void processYoutubeChannelPath(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    bool altPath = false);

  void processYoutubeUserPath(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType);

  void processYoutubeAlternatePath(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    bool altPath);

  void getTwitchMediaKey(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType);

  void onGetChannelHeadlineVideo(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    bool altPath, bool success, const std::string& response,
    const std::map<std::string, std::string>& headers);

  void onFetchPublisherFromDBResponse(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& publisher_key,
    bool altPath = false);

  void onCheckTwitchResponse(
    bool success, const std::string& response,
    const std::map<std::string, std::string>& headers);
  void processYoutubeAsMediaType(
    const std::string& data,
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType);

  void processYoutubeAsPublisherType(
    const std::string& data,
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    bool altPath = false);

  void fetchYoutubeEmbedInfo(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& media_key);

  std::string parsePublisherName(const std::string& data);

  std::string parseFavIconUrl(const std::string& data);

  std::string parsePublisherId(const std::string& publisherURL,
    const std::string& provider_name);

  std::string parseChannelId(const std::string& data);

  std::string getYoutubeMediaKeyFromUrl(
    const std::string& provider_type,
    const ledger::VisitData& visit_data);

  std::string getYoutubePublisherKeyFromUrl(
    const std::string& provider_type,
    const ledger::VisitData& visit_data);

  std::string parseHeadlineMediaKey(const std::string& data);

  std::string extractData(const std::string& data,
    const std::string& matchAfter, const std::string& matchUntil);

  std::string parsePublisherLink(const std::string& data);

  std::string getPublisherUrl(const std::string& publisher_key,
    const std::string& providerName);

  std::string getPublisherKeyFromUrl(const std::string& data);

  bool channelHasAuthor(const std::string& data);

  void fetchPublisherDataFromDB(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const std::string& publisher_key,
    bool altPath = false);

  void fetchPublisherDataFromUrl(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    bool altPath = false);

  void fetchPublisherDataFromUserUrl(uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  bat_ledger::URLRequestHandler handler_;

  std::map<std::string, ledger::TwitchEventInfo> twitchEvents;
};

}  // namespace braveledger_bat_get_media

#endif  // BRAVELEDGER_BAT_GET_MEDIA_H_
