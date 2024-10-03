/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/database/database_server_publisher_info.h"
#include "brave/components/brave_rewards/core/publisher/publisher_prefix_list_updater.h"
#include "brave/components/brave_rewards/core/publisher/server_publisher_fetcher.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace publisher {

class Publisher {
 public:
  explicit Publisher(RewardsEngine& engine);

  ~Publisher();

  bool ShouldFetchServerPublisherInfo(mojom::ServerPublisherInfo* server_info);

  void FetchServerPublisherInfo(const std::string& publisher_key,
                                ServerPublisherFetcher::FetchCallback callback);

  void RefreshPublisher(const std::string& publisher_key,
                        RefreshPublisherCallback callback);

  void SetPublisherServerListTimer();

  void SaveVisit(const std::string& publisher_key,
                 const mojom::VisitData& visit_data,
                 const uint64_t duration,
                 const bool first_visit,
                 uint64_t window_id,
                 PublisherInfoCallback callback);

  void SetPublisherExclude(const std::string& publisher_id,
                           const mojom::PublisherExclude& exclude,
                           ResultCallback callback);

  void OnPublisherInfoSaved(const mojom::Result result);

  void GetPublisherActivityFromUrl(uint64_t windowId,
                                   mojom::VisitDataPtr visit_data,
                                   const std::string& publisher_blob);

  void GetPublisherBanner(const std::string& publisher_key,
                          GetPublisherBannerCallback callback);

  mojom::ActivityInfoFilterPtr CreateActivityFilter(
      const std::string& publisher_id,
      mojom::ExcludeFilter excluded,
      bool min_duration,
      const uint64_t& current_reconcile_stamp,
      bool non_verified,
      bool min_visits);

  void NormalizeContributeWinners(
      std::vector<mojom::PublisherInfoPtr>* newList,
      const std::vector<mojom::PublisherInfoPtr>* list,
      uint32_t /* next_record */);

  void OnRestorePublishers(mojom::Result result, ResultCallback callback);

  void SynopsisNormalizer();

  void CalcScoreConsts(const int min_duration_seconds);

  void GetServerPublisherInfo(const std::string& publisher_key,
                              GetServerPublisherInfoCallback callback);

  void GetServerPublisherInfo(const std::string& publisher_key,
                              bool use_prefix_list,
                              GetServerPublisherInfoCallback callback);

  void GetPublisherPanelInfo(const std::string& publisher_key,
                             GetPublisherPanelInfoCallback callback);

  void SavePublisherInfo(uint64_t window_id,
                         mojom::PublisherInfoPtr publisher_info,
                         ResultCallback callback);

  static std::string GetShareURL(
      const base::flat_map<std::string, std::string>& args);

 private:
  void OnPrefixListUpdated();

  void OnSearchPrefixListForSaveVisit(const std::string& publisher_key,
                                      GetServerPublisherInfoCallback callback,
                                      bool publisher_exists);

  void OnSearchPrefixListForGetServerPublisherInfo(
      const std::string& publisher_key,
      GetServerPublisherInfoCallback callback,
      bool publisher_exists);

  void OnGetPanelPublisherInfo(GetPublisherPanelInfoCallback callback,
                               const mojom::Result result,
                               mojom::PublisherInfoPtr info);

  void onPublisherActivitySave(uint64_t windowId,
                               const mojom::VisitData& visit_data,
                               mojom::Result result,
                               mojom::PublisherInfoPtr info);

  void OnGetActivityInfo(PublisherInfoCallback callback,
                         const std::string& publisher_key,
                         std::vector<mojom::PublisherInfoPtr> list);

  void SaveVisitInternal(const mojom::PublisherStatus,
                         const std::string& publisher_key,
                         const mojom::VisitData& visit_data,
                         const uint64_t duration,
                         const bool first_visit,
                         uint64_t window_id,
                         PublisherInfoCallback callback,
                         mojom::Result result,
                         mojom::PublisherInfoPtr publisher_info);

  void OnSaveVisitServerPublisher(const std::string& publisher_key,
                                  const mojom::VisitData& visit_data,
                                  const uint64_t duration,
                                  const bool first_visit,
                                  uint64_t window_id,
                                  PublisherInfoCallback callback,
                                  mojom::ServerPublisherInfoPtr server_info);

  void onFetchFavIcon(const std::string& publisher_key,
                      uint64_t window_id,
                      bool success,
                      const std::string& favicon_url);

  void onFetchFavIconDBResponse(const std::string& favicon_url,
                                uint64_t window_id,
                                mojom::Result result,
                                mojom::PublisherInfoPtr info);

  void OnSetPublisherExclude(ResultCallback callback,
                             mojom::PublisherExclude exclude,
                             mojom::Result result,
                             mojom::PublisherInfoPtr publisher_info);

  double concaveScore(const uint64_t& duration_seconds);

  void SynopsisNormalizerCallback(std::vector<mojom::PublisherInfoPtr> list);

  void synopsisNormalizerInternal(
      std::vector<mojom::PublisherInfoPtr>* newList,
      const std::vector<mojom::PublisherInfoPtr>* list,
      uint32_t /* next_record */);

  void OnSaveVisitInternal(mojom::Result result, mojom::PublisherInfoPtr info);

  void OnPanelPublisherInfo(uint64_t windowId,
                            const mojom::VisitData& visit_data,
                            mojom::Result result,
                            mojom::PublisherInfoPtr publisher_info);

  void OnGetPublisherBanner(const std::string& publisher_key,
                            GetPublisherBannerCallback callback,
                            mojom::ServerPublisherInfoPtr info);

  void OnGetPublisherBannerPublisher(GetPublisherBannerCallback callback,
                                     const mojom::PublisherBanner& banner,
                                     mojom::Result result,
                                     mojom::PublisherInfoPtr publisher_info);

  void OnGetPublisherBannerForSavePublisherInfo(
      uint64_t window_id,
      const std::string& publisher_key,
      const mojom::VisitData& visit_data,
      ResultCallback callback,
      mojom::PublisherBannerPtr banner);

  void OnSaveVisitForSavePublisherInfo(ResultCallback callback,
                                       mojom::Result result,
                                       mojom::PublisherInfoPtr publisher_info);

  mojom::PublisherStatus ParsePublisherStatus(const std::string& status);

  void OnServerPublisherInfoLoaded(const std::string& publisher_key,
                                   bool use_prefix_list,
                                   GetServerPublisherInfoCallback callback,
                                   mojom::ServerPublisherInfoPtr server_info);

  const raw_ref<RewardsEngine> engine_;
  PublisherPrefixListUpdater prefix_list_updater_;
  ServerPublisherFetcher server_publisher_fetcher_;
  base::WeakPtrFactory<Publisher> weak_factory_{this};

  // For testing purposes
  friend class RewardsPublisherTest;
  FRIEND_TEST_ALL_PREFIXES(RewardsPublisherTest, concaveScore);
  FRIEND_TEST_ALL_PREFIXES(RewardsPublisherTest, synopsisNormalizerInternal);
};

}  // namespace publisher
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_H_
