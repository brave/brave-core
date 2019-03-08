/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_GET_MEDIA_H_
#define BRAVELEDGER_BAT_GET_MEDIA_H_

#include <string>
#include <map>

#include "base/gtest_prod_util.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace leveldb {
class DB;
}

namespace braveledger_bat_get_media {

using FetchDataFromUrlCallback = std::function<void(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers)>;

class BatGetMedia {
 public:
  explicit BatGetMedia(bat_ledger::LedgerImpl* ledger);

  ~BatGetMedia();

  static std::string GetLinkType(const std::string& url,
                                 const std::string& first_party_url,
                                 const std::string& referrer);

  void processMedia(const std::map<std::string, std::string>& parts,
                    const std::string& type,
                    const ledger::VisitData& visit_data);

  void getMediaActivityFromUrl(uint64_t windowId,
                               const ledger::VisitData& visit_data,
                               const std::string& providerType,
                               const std::string& publisher_blob);

 private:
  std::string getMediaURL(const std::string& mediaId,
                          const std::string& providerName);

  void getPublisherFromMediaPropsCallback(
      const uint64_t& duration,
      const std::string& media_key,
      const std::string& providerName,
      const std::string& mediaURL,
      const ledger::VisitData& visit_data,
      const uint64_t window_id,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void getPublisherInfoCallback(
      const uint64_t& duration,
      const std::string& media_key,
      const std::string& providerName,
      const std::string& mediaURL,
      const std::string& publisherURL,
      const std::string& publisherName,
      const ledger::VisitData& visit_data,
      const uint64_t window_id,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void savePublisherInfo(
      const uint64_t& duration,
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

  std::string getTwitchStatus(const ledger::TwitchEventInfo& oldEventInfo,
                              const ledger::TwitchEventInfo& newEventInfo);

  void getPublisherInfoDataCallback(
      const std::string& mediaId,
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

  void onMediaPublisherActivity(
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info,
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& providerType,
      const std::string& media_key,
      const std::string& media_id,
      const std::string& publisher_blob);

  void onGetChannelIdFromUserPage(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& providerType,
      const std::string& media_key,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void onGetMediaActivityFromUrl(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string& providerType,
      const std::string& url,
      uint64_t windowId);

  void processYoutubeMediaPanel(uint64_t windowId,
                                const ledger::VisitData& visit_data,
                                const std::string& providerType);

  void processTwitchMediaPanel(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& providerType,
      const std::string& publisher_blob);

  std::string getTwitchMediaIdFromUrl(
      const ledger::VisitData& visit_data,
      const std::string& publisher_blob) const;

  std::string getTwitchMediaKeyFromUrl(
      const std::string& provider_type,
      const std::string& id,
      const std::string& url) const;

  std::string getUserFacingHandle(const std::string& publisher_blob) const;

  std::string getFaviconUrl(
      const std::string& publisher_blob,
      const std::string& twitchHandle) const;

  void onGetTwitchPublisherInfo(
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> publisher_info,
      uint64_t windowId,
      const ledger::VisitData visit_data,
      const std::string& providerType,
      const std::string& media_key,
      const std::string& media_id,
      const std::string& publisher_blob);

  void updateTwitchPublisherData(
      std::string* publisher_name,
      std::string* publisher_favicon_url,
      const std::string& publisher_blob);

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

  void processYoutubeCustomPath(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& providerType,
      const std::string& publisher_key);

  void onGetChannelHeadlineVideo(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& providerType,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const bool is_custom_path);

  void onFetchPublisherFromDBResponse(
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info,
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& providerType,
      const std::string& publisher_key,
      const std::string& publisher_blob,
      const bool is_custom_path);

  void processYoutubeAsPublisherType(const std::string& data,
                                     uint64_t windowId,
                                     const ledger::VisitData& visit_data,
                                     const std::string& providerType);

  std::string parseFavIconUrl(const std::string& data);

  std::string parseChannelId(const std::string& data);

  std::string getYoutubeMediaKeyFromUrl(const std::string& provider_type,
                                        const std::string& media_id);

  std::string getPublisherUrl(const std::string& publisher_key,
                              const std::string& providerName);

  void fetchPublisherDataFromDB(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& providerType,
      const std::string& publisher_key,
      const std::string& publisher_blob,
      const bool is_custom_path);

  void fetchDataFromUrl(const std::string& url,
                        FetchDataFromUrlCallback callback);

  static std::string getYoutubeMediaIdFromUrl(
      const ledger::VisitData& visit_data);

  static std::string getYoutubePublisherKeyFromUrl(const std::string& path);

  static std::string getYoutubeUserFromUrl(const std::string& path);

  static std::string extractData(const std::string& data,
                                 const std::string& matchAfter,
                                 const std::string& matchUntil);

  std::string getNameFromChannel(const std::string& data);

  bool isPredefinedYTPath(const std::string& path) const;

  std::string parseChannelIdFromCustomPathPage(
      const std::string& data);

  std::string getRealEnteredYTPath(const std::string& path) const;

  void onFetchDataFromNonEmbeddable(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& providerType,
    const uint64_t& duration,
    const std::string& media_key,
    const std::string& mediaURL,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  static std::string parsePublisherName(const std::string& data);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  std::map<std::string, ledger::TwitchEventInfo> twitchEvents;

    // For testing purposes
  friend class BatGetMediaTest;
  FRIEND_TEST_ALL_PREFIXES(BatGetMediaTest, GetYoutubeMediaIdFromUrl);
  FRIEND_TEST_ALL_PREFIXES(BatGetMediaTest, GetYoutubePublisherKeyFromUrl);
  FRIEND_TEST_ALL_PREFIXES(BatGetMediaTest, GetYoutubeUserFromUrl);
  FRIEND_TEST_ALL_PREFIXES(BatGetMediaTest, getRealEnteredYTPath);
  FRIEND_TEST_ALL_PREFIXES(BatGetMediaTest, GetNameFromChannel);
  FRIEND_TEST_ALL_PREFIXES(BatGetMediaTest, ParsePublisherName);
};

}  // namespace braveledger_bat_get_media

#endif  // BRAVELEDGER_BAT_GET_MEDIA_H_
