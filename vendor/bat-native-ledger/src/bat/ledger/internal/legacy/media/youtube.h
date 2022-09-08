/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_MEDIA_YOUTUBE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_MEDIA_YOUTUBE_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "bat/ledger/internal/legacy/media/helper.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;
}

namespace braveledger_media {

class YouTube {
 public:
  explicit YouTube(ledger::LedgerImpl* ledger);

  ~YouTube();

  void ProcessMedia(const base::flat_map<std::string, std::string>& parts,
                    const ledger::mojom::VisitData& visit_data);

  static std::string GetLinkType(const std::string& url);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const ledger::mojom::VisitData& visit_data);

 private:
  static std::string GetMediaIdFromParts(
      const base::flat_map<std::string, std::string>& parts);

  static uint64_t GetMediaDurationFromParts(
      const base::flat_map<std::string, std::string>& data,
      const std::string& media_key);

  static std::string GetVideoUrl(const std::string& media_id);

  static std::string GetChannelUrl(const std::string& publisher_key);

  static std::string GetFavIconUrl(const std::string& data);

  static std::string GetChannelId(const std::string& data);

  static std::string GetPublisherName(const std::string& data);

  static std::string GetMediaIdFromUrl(const std::string& url);

  static std::string GetNameFromChannel(const std::string& data);

  static std::string GetPublisherKeyFromUrl(const std::string& path);

  static std::string GetChannelIdFromCustomPathPage(const std::string& data);

  static std::string GetBasicPath(const std::string& path);

  static bool IsPredefinedPath(const std::string& path);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetUserFromUrl(const std::string& path);

  void OnMediaActivityError(const ledger::mojom::VisitData& visit_data,
                            uint64_t window_id);

  void OnMediaPublisherInfo(const std::string& media_id,
                            const std::string& media_key,
                            const uint64_t duration,
                            const ledger::mojom::VisitData& visit_data,
                            uint64_t window_id,
                            ledger::mojom::Result result,
                            ledger::mojom::PublisherInfoPtr publisher_info);

  void OnEmbedResponse(const uint64_t duration,
                       const std::string& media_key,
                       const std::string& media_url,
                       const ledger::mojom::VisitData& visit_data,
                       const uint64_t window_id,
                       const ledger::mojom::UrlResponse& response);

  void OnPublisherPage(const uint64_t duration,
                       const std::string& media_key,
                       std::string publisher_url,
                       std::string publisher_name,
                       const ledger::mojom::VisitData& visit_data,
                       const uint64_t window_id,
                       const ledger::mojom::UrlResponse& response);

  void SavePublisherInfo(const uint64_t duration,
                         const std::string& media_key,
                         const std::string& publisher_url,
                         const std::string& publisher_name,
                         const ledger::mojom::VisitData& visit_data,
                         const uint64_t window_id,
                         const std::string& fav_icon,
                         const std::string& channel_id);

  void FetchDataFromUrl(const std::string& url,
                        ledger::client::LegacyLoadURLCallback callback);

  void WatchPath(uint64_t window_id,
                 const ledger::mojom::VisitData& visit_data);

  void OnMediaPublisherActivity(ledger::mojom::Result result,
                                ledger::mojom::PublisherInfoPtr info,
                                uint64_t window_id,
                                const ledger::mojom::VisitData& visit_data,
                                const std::string& media_key,
                                const std::string& media_id);

  void GetPublisherPanleInfo(uint64_t window_id,
                             const ledger::mojom::VisitData& visit_data,
                             const std::string& publisher_key,
                             bool is_custom_path);

  void OnPublisherPanleInfo(uint64_t window_id,
                            const ledger::mojom::VisitData& visit_data,
                            const std::string& publisher_key,
                            bool is_custom_path,
                            ledger::mojom::Result result,
                            ledger::mojom::PublisherInfoPtr info);

  void GetChannelHeadlineVideo(uint64_t window_id,
                               const ledger::mojom::VisitData& visit_data,
                               bool is_custom_path,
                               const ledger::mojom::UrlResponse& response);

  void ChannelPath(uint64_t window_id,
                   const ledger::mojom::VisitData& visit_data);

  void UserPath(uint64_t windowId, const ledger::mojom::VisitData& visit_data);

  void OnUserActivity(uint64_t window_id,
                      const ledger::mojom::VisitData& visit_data,
                      const std::string& media_key,
                      ledger::mojom::Result result,
                      ledger::mojom::PublisherInfoPtr info);

  void OnChannelIdForUser(uint64_t window_id,
                          const ledger::mojom::VisitData& visit_data,
                          const std::string& media_key,
                          const ledger::mojom::UrlResponse& response);

  ledger::LedgerImpl* ledger_;  // NOT OWNED

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

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_MEDIA_YOUTUBE_H_
