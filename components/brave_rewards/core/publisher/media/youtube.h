/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_MEDIA_YOUTUBE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_MEDIA_YOUTUBE_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/publisher/media/helper.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {

class RewardsEngine;

class YouTube {
 public:
  explicit YouTube(RewardsEngine& engine);

  ~YouTube();

  void ProcessMedia(const base::flat_map<std::string, std::string>& parts,
                    const mojom::VisitData& visit_data);

  static std::string GetLinkType(const std::string& url);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const mojom::VisitData& visit_data);

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

  void OnMediaActivityError(const mojom::VisitData& visit_data,
                            uint64_t window_id);

  void OnMediaPublisherInfo(const std::string& media_id,
                            const std::string& media_key,
                            const uint64_t duration,
                            const mojom::VisitData& visit_data,
                            uint64_t window_id,
                            mojom::Result result,
                            mojom::PublisherInfoPtr publisher_info);

  void OnEmbedResponse(const uint64_t duration,
                       const std::string& media_key,
                       const std::string& media_url,
                       const mojom::VisitData& visit_data,
                       const uint64_t window_id,
                       mojom::UrlResponsePtr response);

  void OnPublisherPage(const uint64_t duration,
                       const std::string& media_key,
                       std::string publisher_url,
                       std::string publisher_name,
                       const mojom::VisitData& visit_data,
                       const uint64_t window_id,
                       mojom::UrlResponsePtr response);

  void SavePublisherInfo(const uint64_t duration,
                         const std::string& media_key,
                         const std::string& publisher_url,
                         const std::string& publisher_name,
                         const mojom::VisitData& visit_data,
                         const uint64_t window_id,
                         const std::string& fav_icon,
                         const std::string& channel_id);

  void FetchDataFromUrl(const std::string& url, LoadURLCallback callback);

  void WatchPath(uint64_t window_id, const mojom::VisitData& visit_data);

  void OnMediaPublisherActivity(uint64_t window_id,
                                const mojom::VisitData& visit_data,
                                const std::string& media_key,
                                const std::string& media_id,
                                mojom::Result result,
                                mojom::PublisherInfoPtr info);

  void GetPublisherPanleInfo(uint64_t window_id,
                             const mojom::VisitData& visit_data,
                             const std::string& publisher_key,
                             bool is_custom_path);

  void OnPublisherPanleInfo(uint64_t window_id,
                            const mojom::VisitData& visit_data,
                            const std::string& publisher_key,
                            bool is_custom_path,
                            mojom::Result result,
                            mojom::PublisherInfoPtr info);

  void GetChannelHeadlineVideo(uint64_t window_id,
                               const mojom::VisitData& visit_data,
                               bool is_custom_path,
                               mojom::UrlResponsePtr response);

  void ChannelPath(uint64_t window_id, const mojom::VisitData& visit_data);

  void UserPath(uint64_t windowId, const mojom::VisitData& visit_data);

  void OnUserActivity(uint64_t window_id,
                      const mojom::VisitData& visit_data,
                      const std::string& media_key,
                      mojom::Result result,
                      mojom::PublisherInfoPtr info);

  void OnChannelIdForUser(uint64_t window_id,
                          const mojom::VisitData& visit_data,
                          const std::string& media_key,
                          mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngine> engine_;
  base::WeakPtrFactory<YouTube> weak_factory_{this};

  // For testing purposes
  friend class RewardsMediaYouTubeTest;
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetMediaIdFromUrl);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetPublisherKeyFromUrl);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetUserFromUrl);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetBasicPath);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetNameFromChannel);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetPublisherName);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetMediaIdFromParts);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetMediaDurationFromParts);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetVideoUrl);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetChannelUrl);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetFavIconUrl);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetChannelId);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest,
                           GetChannelIdFromCustomPathPage);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, IsPredefinedPath);
  FRIEND_TEST_ALL_PREFIXES(RewardsMediaYouTubeTest, GetPublisherKey);
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_MEDIA_YOUTUBE_H_
