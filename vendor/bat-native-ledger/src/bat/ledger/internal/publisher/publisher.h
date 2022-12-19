/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_H_

#include <string>
#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace publisher {

class PublisherPrefixListUpdater;
class ServerPublisherFetcher;

class Publisher {
 public:
  explicit Publisher(LedgerImpl* ledger);

  ~Publisher();

  bool ShouldFetchServerPublisherInfo(mojom::ServerPublisherInfo* server_info);

  void FetchServerPublisherInfo(
      const std::string& publisher_key,
      client::GetServerPublisherInfoCallback callback);

  void RefreshPublisher(
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback);

  void SetPublisherServerListTimer();

  void SaveVisit(const std::string& publisher_key,
                 const mojom::VisitData& visit_data,
                 const uint64_t duration,
                 const bool first_visit,
                 uint64_t window_id,
                 const ledger::PublisherInfoCallback callback);

  void SaveVideoVisit(const std::string& publisher_id,
                      const mojom::VisitData& visit_data,
                      uint64_t duration,
                      const bool first_visit,
                      uint64_t window_id,
                      ledger::PublisherInfoCallback callback);

  void SetPublisherExclude(const std::string& publisher_id,
                           const mojom::PublisherExclude& exclude,
                           ledger::ResultCallback callback);

  void OnPublisherInfoSaved(const mojom::Result result);

  void GetPublisherActivityFromUrl(uint64_t windowId,
                                   mojom::VisitDataPtr visit_data,
                                   const std::string& publisher_blob);

  void GetPublisherBanner(const std::string& publisher_key,
                          ledger::PublisherBannerCallback callback);

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

  void OnRestorePublishers(mojom::Result result,
                           ledger::ResultCallback callback);

  bool IsVerified(mojom::PublisherStatus);

  void SynopsisNormalizer();

  void CalcScoreConsts(const int min_duration_seconds);

  void GetServerPublisherInfo(
      const std::string& publisher_key,
      client::GetServerPublisherInfoCallback callback);

  void GetServerPublisherInfo(const std::string& publisher_key,
                              bool use_prefix_list,
                              client::GetServerPublisherInfoCallback callback);

  void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      const uint64_t duration,
      const bool first_visit);

  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      ledger::GetPublisherInfoCallback callback);

  void SavePublisherInfo(uint64_t window_id,
                         mojom::PublisherInfoPtr publisher_info,
                         ledger::LegacyResultCallback callback);

  static std::string GetShareURL(
      const base::flat_map<std::string, std::string>& args);

 private:
  void OnGetPublisherInfoForUpdateMediaDuration(mojom::Result result,
                                                mojom::PublisherInfoPtr info,
                                                const uint64_t window_id,
                                                const uint64_t duration,
                                                const bool first_visit);

  void OnGetPanelPublisherInfo(const mojom::Result result,
                               mojom::PublisherInfoPtr info,
                               ledger::GetPublisherInfoCallback callback);

  void onPublisherActivitySave(uint64_t windowId,
                               const mojom::VisitData& visit_data,
                               mojom::Result result,
                               mojom::PublisherInfoPtr info);

  void OnGetActivityInfo(std::vector<mojom::PublisherInfoPtr> list,
                         ledger::PublisherInfoCallback callback,
                         const std::string& publisher_key);

  void SaveVisitInternal(const mojom::PublisherStatus,
                         const std::string& publisher_key,
                         const mojom::VisitData& visit_data,
                         const uint64_t duration,
                         const bool first_visit,
                         uint64_t window_id,
                         const ledger::PublisherInfoCallback callback,
                         mojom::Result result,
                         mojom::PublisherInfoPtr publisher_info);

  void OnSaveVisitServerPublisher(mojom::ServerPublisherInfoPtr server_info,
                                  const std::string& publisher_key,
                                  const mojom::VisitData& visit_data,
                                  const uint64_t duration,
                                  const bool first_visit,
                                  uint64_t window_id,
                                  const ledger::PublisherInfoCallback callback);

  void onFetchFavIcon(const std::string& publisher_key,
                      uint64_t window_id,
                      bool success,
                      const std::string& favicon_url);

  void onFetchFavIconDBResponse(mojom::Result result,
                                mojom::PublisherInfoPtr info,
                                const std::string& favicon_url,
                                uint64_t window_id);

  void OnSetPublisherExclude(ledger::ResultCallback callback,
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

  void OnPanelPublisherInfo(mojom::Result result,
                            mojom::PublisherInfoPtr publisher_info,
                            uint64_t windowId,
                            const mojom::VisitData& visit_data);

  void OnGetPublisherBanner(mojom::ServerPublisherInfoPtr info,
                            const std::string& publisher_key,
                            ledger::PublisherBannerCallback callback);

  void OnGetPublisherBannerPublisher(ledger::PublisherBannerCallback callback,
                                     const mojom::PublisherBanner& banner,
                                     mojom::Result result,
                                     mojom::PublisherInfoPtr publisher_info);

  void OnGetPublisherBannerForSavePublisherInfo(
      mojom::PublisherBannerPtr banner,
      uint64_t window_id,
      const std::string& publisher_key,
      const mojom::VisitData& visit_data,
      ledger::LegacyResultCallback callback);

  mojom::PublisherStatus ParsePublisherStatus(const std::string& status);

  void OnServerPublisherInfoLoaded(
      mojom::ServerPublisherInfoPtr server_info,
      const std::string& publisher_key,
      bool use_prefix_list,
      client::GetServerPublisherInfoCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<PublisherPrefixListUpdater> prefix_list_updater_;
  std::unique_ptr<ServerPublisherFetcher> server_publisher_fetcher_;

  // For testing purposes
  friend class PublisherTest;
  FRIEND_TEST_ALL_PREFIXES(PublisherTest, concaveScore);
  FRIEND_TEST_ALL_PREFIXES(PublisherTest, synopsisNormalizerInternal);
};

}  // namespace publisher
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_H_
