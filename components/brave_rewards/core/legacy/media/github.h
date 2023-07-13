/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_MEDIA_GITHUB_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_MEDIA_GITHUB_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/legacy/media/helper.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {

class RewardsEngineImpl;

class GitHub {
 public:
  explicit GitHub(RewardsEngineImpl& engine);

  static std::string GetLinkType(const std::string& url);

  void SaveMediaInfo(const base::flat_map<std::string, std::string>& data,
                     PublisherInfoCallback callback);

  void ProcessActivityFromUrl(uint64_t window_id,
                              const mojom::VisitData& visit_data);

  void ProcessMedia(const base::flat_map<std::string, std::string> parts,
                    const mojom::VisitData& visit_data);

  ~GitHub();

 private:
  void OnMediaPublisherActivity(mojom::Result result,
                                mojom::PublisherInfoPtr info,
                                uint64_t window_id,
                                const mojom::VisitData& visit_data,
                                const std::string& media_key);

  void FetchDataFromUrl(const std::string& url, LegacyLoadURLCallback callback);

  void OnUserPage(const uint64_t duration,
                  uint64_t window_id,
                  const mojom::VisitData& visit_data,
                  mojom::UrlResponsePtr response);

  void SavePublisherInfo(const uint64_t duration,
                         const std::string& user_id,
                         const std::string& user_name,
                         const std::string& publisher_name,
                         const std::string& profile_picture,
                         const uint64_t window_id,
                         PublisherInfoCallback callback);

  void GetPublisherPanelInfo(uint64_t window_id,
                             const mojom::VisitData& visit_data,
                             const std::string& publisher_key);

  void OnPublisherPanelInfo(uint64_t window_id,
                            const mojom::VisitData& visit_data,
                            const std::string& publisher_key,
                            mojom::Result result,
                            mojom::PublisherInfoPtr info);

  void OnMediaActivityError(uint64_t window_id);

  void OnMetaDataGet(PublisherInfoCallback callback,
                     mojom::UrlResponsePtr response);

  void OnMediaPublisherInfo(uint64_t window_id,
                            const std::string& user_id,
                            const std::string& screen_name,
                            const std::string& publisher_name,
                            const std::string& profile_picture,
                            PublisherInfoCallback callback,
                            mojom::Result result,
                            mojom::PublisherInfoPtr publisher_info);

  static std::string GetUserNameFromURL(const std::string& path);

  static std::string GetUserName(const std::string& json_string);

  static std::string GetMediaKey(const std::string& user_name);

  static std::string GetUserId(const std::string& json_string);

  static std::string GetPublisherName(const std::string& json_string);

  static std::string GetProfileURL(const std::string& user_name);

  static std::string GetProfileAPIURL(const std::string& user_name);

  static std::string GetPublisherKey(const std::string& key);

  static std::string GetProfileImageURL(const std::string& json_string);

  static bool IsExcludedPath(const std::string& path);

  static bool GetJSONStringValue(const std::string& key,
                                 const std::string& json_string,
                                 std::string* result);

  static bool GetJSONIntValue(const std::string& key,
                              const std::string& json_string,
                              int64_t* result);

  // For testing purposes
  friend class MediaGitHubTest;
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetLinkType);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileURL);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileAPIURL);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetProfileImageURL);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetPublisherKey);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetMediaKey);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserNameFromURL);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserName);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetUserId);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetPublisherName);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetJSONStringValue);
  FRIEND_TEST_ALL_PREFIXES(MediaGitHubTest, GetJSONIntValue);

  const raw_ref<RewardsEngineImpl> engine_;
};
}  // namespace brave_rewards::internal
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_MEDIA_GITHUB_H_
