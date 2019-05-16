/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_MEDIA_YOUTUBE_H_
#define BRAVELEDGER_MEDIA_YOUTUBE_H_

#include <map>
#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/media/helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_media {

class MediaYouTube : public ledger::LedgerCallbackHandler {
 public:
  explicit MediaYouTube(bat_ledger::LedgerImpl* ledger);

  ~MediaYouTube() override;

  void ProcessMedia(const std::map<std::string, std::string>& parts,
                    const ledger::VisitData& visit_data);

  static std::string GetLinkType(const std::string& url);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::VisitData& visit_data);

 private:
  static std::string GetMediaIdFromParts(
      const std::map<std::string, std::string>& parts);

  static uint64_t GetMediaDurationFromParts(
      const std::map<std::string, std::string>& data,
      const std::string& media_key);

  static std::string GetVideoUrl(const std::string& media_id);

  static std::string GetChannelUrl(const std::string& publisher_key);

  static std::string GetFavIconUrl(const std::string& data);

  static std::string GetChannelId(const std::string& data);

  static std::string GetPublisherName(const std::string& data);

  static std::string GetMediaIdFromUrl(const ledger::VisitData& visit_data);

  static std::string GetNameFromChannel(const std::string& data);

  static std::string GetPublisherKeyFromUrl(const std::string& path);

  static std::string GetChannelIdFromCustomPathPage(const std::string& data);

  static std::string GetBasicPath(const std::string& path);

  static bool IsPredefinedPath(const std::string& path);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetUserFromUrl(const std::string& path);

  void OnMediaActivityError(const ledger::VisitData& visit_data,
                            uint64_t window_id);

  void OnSaveMediaVisit(ledger::Result result,
                        ledger::PublisherInfoPtr info);

  void OnMediaPublisherInfo(
    const std::string& media_id,
    const std::string& media_key,
    const uint64_t duration,
    const ledger::VisitData& visit_data,
    uint64_t window_id,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info);

  void OnEmbedResponse(
    const uint64_t duration,
    const std::string& media_key,
    const std::string& media_url,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void OnPublisherPage(
    const uint64_t duration,
    const std::string& media_key,
    std::string publisher_url,
    std::string publisher_name,
    const ledger::VisitData& visit_data,
    const uint64_t window_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void SavePublisherInfo(const uint64_t duration,
                         const std::string& media_key,
                         const std::string& publisher_url,
                         const std::string& publisher_name,
                         const ledger::VisitData& visit_data,
                         const uint64_t window_id,
                         const std::string& fav_icon,
                         const std::string& channel_id);

  void FetchDataFromUrl(const std::string& url,
                        braveledger_media::FetchDataFromUrlCallback callback);

  void WatchPath(uint64_t window_id,
                 const ledger::VisitData& visit_data);

  void OnMediaPublisherActivity(ledger::Result result,
                                ledger::PublisherInfoPtr info,
                                uint64_t window_id,
                                const ledger::VisitData& visit_data,
                                const std::string& media_key,
                                const std::string& media_id);

  void GetPublisherPanleInfo(uint64_t window_id,
                             const ledger::VisitData& visit_data,
                             const std::string& publisher_key,
                             bool is_custom_path);

  void OnPublisherPanleInfo(uint64_t window_id,
                            const ledger::VisitData& visit_data,
                            const std::string& publisher_key,
                            bool is_custom_path,
                            ledger::Result result,
                            ledger::PublisherInfoPtr info);

  void GetChannelHeadlineVideo(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    bool is_custom_path,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void ChannelPath(uint64_t window_id,
                   const ledger::VisitData& visit_data);

  void UserPath(uint64_t windowId,
                const ledger::VisitData& visit_data);

  void OnUserActivity(uint64_t window_id,
                      const ledger::VisitData& visit_data,
                      const std::string& media_key,
                      ledger::Result result,
                      ledger::PublisherInfoPtr info);

  void OnChannelIdForUser(
    uint64_t window_id,
    const ledger::VisitData& visit_data,
    const std::string& media_key,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  // For testing purposes
  friend class MediaYouTubeTest;
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetMediaIdFromUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetPublisherKeyFromUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetUserFromUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetBasicPath);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetNameFromChannel);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetPublisherName);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetMediaIdFromParts);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetMediaDurationFromParts);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetVideoUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetChannelUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetFavIconUrl);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetChannelId);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetChannelIdFromCustomPathPage);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, IsPredefinedPath);
  FRIEND_TEST_ALL_PREFIXES(MediaYouTubeTest, GetPublisherKey);
};

}  // namespace braveledger_media

#endif  // BRAVELEDGER_MEDIA_YOUTUBE_H_
