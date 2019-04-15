/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_TWITCH_H_
#define BRAVELEDGER_MEDIA_TWITCH_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/gtest_prod_util.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/media/helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_media {

class MediaTwitch : public ledger::LedgerCallbackHandler {
 public:
  explicit MediaTwitch(bat_ledger::LedgerImpl* ledger);

  ~MediaTwitch() override;

  void OnMediaActivityError(const ledger::VisitData& visit_data,
                            uint64_t window_id);

  void ProcessMedia(const std::map<std::string, std::string>& parts,
                    const ledger::VisitData& visit_data);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::VisitData& visit_data,
                              const std::string& publisher_blob);

  static std::string GetLinkType(const std::string& url,
                                 const std::string& first_party_url,
                                 const std::string& referrer);

 private:
  static std::pair<std::string, std::string> GetMediaIdFromParts(
      const std::map<std::string, std::string>& parts);

  static std::string GetMediaURL(const std::string& mediaId);

  static std::string GetTwitchStatus(
    const ledger::TwitchEventInfo& old_event,
    const ledger::TwitchEventInfo& new_event);

  static uint64_t GetTwitchDuration(
    const ledger::TwitchEventInfo& old_event,
    const ledger::TwitchEventInfo& new_event);

  static std::string GetMediaIdFromUrl(const std::string& url,
                                       const std::string& publisher_blob);

  static std::string GetMediaKeyFromUrl(const std::string& id,
                                        const std::string& url);

  static std::string GetPublisherKey(const std::string& key);

  static void UpdatePublisherData(std::string* publisher_name,
                                  std::string* publisher_favicon_url,
                                  const std::string& publisher_blob);

  static std::string GetPublisherName(const std::string& publisher_blob);

  static std::string GetFaviconUrl(const std::string& publisher_blob,
                                   const std::string& twitchHandle);

  void OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const ledger::TwitchEventInfo& twitch_info,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    const std::string& user_id,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void FetchDataFromUrl(
    const std::string& url,
    braveledger_media::FetchDataFromUrlCallback callback);

  void OnEmbedResponse(
    const uint64_t duration,
    const std::string& media_key,
    const std::string& media_url,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    const std::string& user_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void OnMediaPublisherActivity(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& media_key,
    const std::string& media_id,
    const std::string& publisher_blob,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info);

  void OnPublisherInfo(
    uint64_t window_id,
    const ledger::VisitData visit_data,
    const std::string& media_key,
    const std::string& media_id,
    const std::string& publisher_blob,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void SavePublisherInfo(const uint64_t duration,
                         const std::string& media_key,
                         const std::string& publisher_url,
                         const std::string& publisher_name,
                         const ledger::VisitData& visit_data,
                         const uint64_t window_id,
                         const std::string& fav_icon,
                         const std::string& channel_id);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, ledger::TwitchEventInfo> twitch_events;

  // For testing purposes
  friend class MediaTwitchTest;
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetMediaIdFromParts);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetMediaURL);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetTwitchStatus);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetTwitchDuration);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetMediaIdFromUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetMediaKeyFromUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetPublisherKey);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, UpdatePublisherData);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetPublisherName);
  FRIEND_TEST_ALL_PREFIXES(MediaTwitchTest, GetFaviconUrl);
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_TWITCH_H_
