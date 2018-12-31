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

namespace bat_ledger {
class LedgerImpl;
}

namespace leveldb {
class DB;
}

namespace braveledger_bat_get_media {

using FetchDataFromUrlCallback = std::function<void(
    bool result,
    const std::string& response,
    const std::map<std::string, std::string>& headers)>;

class BatGetMedia {
 public:
  static std::string GetLinkType(const std::string& url,
                                 const std::string& first_party_url,
                                 const std::string& referrer);

  BatGetMedia(bat_ledger::LedgerImpl* ledger);
  ~BatGetMedia();

  void processMedia(const std::map<std::string, std::string>& parts,
                    const std::string& type,
                    const ledger::VisitData& visit_data);

  void getMediaActivityFromUrl(uint64_t windowId,
                               const ledger::VisitData& visit_data,
                               const std::string& providerType);

 private:
  std::string getMediaURL(const std::string& mediaId, const std::string& providerName);
  void getPublisherFromMediaPropsCallback(const uint64_t& duration,
                                          const std::string& media_key,
                                          const std::string& providerName,
                                          const std::string& mediaURL,
                                          const ledger::VisitData& visit_data,
                                          const uint64_t window_id,
                                          bool result,
                                          const std::string& response,
                                          const std::map<std::string, std::string>& headers);
  void getPublisherInfoCallback(const uint64_t& duration,
                                const std::string& media_key,
                                const std::string& providerName,
                                const std::string& mediaURL,
                                const std::string& publisherURL,
                                const std::string& publisherName,
                                const ledger::VisitData& visit_data,
                                const uint64_t window_id,
                                bool result,
                                const std::string& response,
                                const std::map<std::string, std::string>& headers);

  void savePublisherInfo(const uint64_t& duration,
                         const std::string& media_key,
                         const std::string& providerName,
                         const std::string& publisherURL,
                         const std::string& publisherName,
                         const ledger::VisitData& visit_data,
                         const uint64_t window_id,
                         const std::string& favIconURL,
                         const std::string& channelId);

  uint64_t getTwitchDuration(const ledger::TwitchEventInfo& oldEventInfo,
                             const ledger::TwitchEventInfo& newEventInfo);

  void onFetchFavIcon(const std::string& publisher_key,
                      bool success,
                      const std::string& favicon_url);

  void onFetchFavIconDBResponse(ledger::Result result,
                           std::unique_ptr<ledger::PublisherInfo> info,
                           const std::string& favicon_url);

  std::string getTwitchStatus(const ledger::TwitchEventInfo& oldEventInfo,
                              const ledger::TwitchEventInfo& newEventInfo);

  void getPublisherInfoDataCallback(const std::string& mediaId,
                                    const std::string& media_key,
                                    const std::string& providerName,
                                    const uint64_t& duration,
                                    const ledger::TwitchEventInfo& twitchEventInfo,
                                    const ledger::VisitData& visit_data,
                                    const uint64_t window_id,
                                    ledger::Result result,
                                    std::unique_ptr<ledger::PublisherInfo> media_publisher_info);

  void onMediaActivityError(const ledger::VisitData& visit_data,
                            const std::string& providerType,
                            uint64_t windowId);

  void onMediaPublisherActivity(ledger::Result result,
                                std::unique_ptr<ledger::PublisherInfo> info,
                                uint64_t windowId,
                                const ledger::VisitData& visit_data,
                                const std::string& providerType,
                                const std::string& media_key,
                                const std::string& media_id);

  void onGetChannelIdFromUserPage(uint64_t windowId,
                                  const ledger::VisitData& visit_data,
                                  const std::string& providerType,
                                  const std::string& media_key,
                                  bool success,
                                  const std::string& response,
                                  const std::map<std::string, std::string>& headers);

  void onGetMediaActivityFromUrl(bool success,
                                 const std::string& response,
                                 const std::map<std::string, std::string>& headers,
                                 const std::string& providerType,
                                 const std::string& url,
                                 uint64_t windowId);

  void processYoutubeMediaPanel(uint64_t windowId,
                                const ledger::VisitData& visit_data,
                                const std::string& providerType);

  void processTwitchMediaPanel(uint64_t windowId,
                               const ledger::VisitData& visit_data,
                               const std::string& providerType);

  void processYoutubeWatchPath(uint64_t windowId,
                               const ledger::VisitData& visit_data,
                               const std::string& providerType);

  void processYoutubeChannelPath(uint64_t windowId,
                                 const ledger::VisitData& visit_data,
                                 const std::string& providerType);

  void onMediaUserActivity(ledger::Result result,
                           std::unique_ptr<ledger::PublisherInfo> info,
                           uint64_t windowId,
                           const ledger::VisitData& visit_data,
                           const std::string& providerType,
                           const std::string& media_key);

  void processYoutubeUserPath(uint64_t windowId,
                              const ledger::VisitData& visit_data,
                              const std::string& providerType);

  void onGetChannelHeadlineVideo(uint64_t windowId,
                                 const ledger::VisitData& visit_data,
                                 const std::string& providerType,
                                 bool success,
                                 const std::string& response,
                                 const std::map<std::string, std::string>& headers);

  void onFetchPublisherFromDBResponse(ledger::Result result,
                                      std::unique_ptr<ledger::PublisherInfo> info,
                                      uint64_t windowId,
                                      const ledger::VisitData& visit_data,
                                      const std::string& providerType,
                                      const std::string& publisher_key);

  void processYoutubeAsPublisherType(const std::string& data,
                                     uint64_t windowId,
                                     const ledger::VisitData& visit_data,
                                     const std::string& providerType);

  std::string parseFavIconUrl(const std::string& data);

  std::string parseChannelId(const std::string& data);

  std::string getYoutubeMediaIdFromUrl(const ledger::VisitData& visit_data);

  std::string getYoutubeMediaKeyFromUrl(const std::string& provider_type, const std::string& media_id);

  std::string getYoutubePublisherKeyFromUrl(const ledger::VisitData& visit_data);

  std::string getYoutubeUserFromUrl(const ledger::VisitData& visit_data);

  std::string extractData(const std::string& data,
                          const std::string& matchAfter,
                          const std::string& matchUntil);

  std::string getPublisherUrl(const std::string& publisher_key, const std::string& providerName);

  void fetchPublisherDataFromDB(uint64_t windowId,
                                const ledger::VisitData& visit_data,
                                const std::string& providerType,
                                const std::string& publisher_key);

  void fetchDataFromUrl(const std::string& url, FetchDataFromUrlCallback callback);

  std::string getNameFromChannel(const std::string& data);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  std::map<std::string, ledger::TwitchEventInfo> twitchEvents;
};

}  // namespace braveledger_bat_get_media

#endif  // BRAVELEDGER_BAT_GET_MEDIA_H_
