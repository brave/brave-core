/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PUBLISHER_PUBLISHER_H_
#define BRAVELEDGER_PUBLISHER_PUBLISHER_H_

#include <string>
#include <map>
#include <memory>
#include <vector>

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

  bool ShouldFetchServerPublisherInfo(
      type::ServerPublisherInfo* server_info);

  void FetchServerPublisherInfo(
      const std::string& publisher_key,
      client::GetServerPublisherInfoCallback callback);

  void RefreshPublisher(
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback);

  void SetPublisherServerListTimer();

  void SaveVisit(const std::string& publisher_key,
                 const type::VisitData& visit_data,
                 const uint64_t& duration,
                 uint64_t window_id,
                 const ledger::PublisherInfoCallback callback);

  void SaveVideoVisit(
      const std::string& publisher_id,
      const type::VisitData& visit_data,
      uint64_t duration,
      uint64_t window_id,
      ledger::PublisherInfoCallback callback);

  void SetPublisherExclude(
      const std::string& publisher_id,
      const type::PublisherExclude& exclude,
      ledger::ResultCallback callback);

  void OnPublisherInfoSaved(const type::Result result);

  void GetPublisherActivityFromUrl(
      uint64_t windowId,
      type::VisitDataPtr visit_data,
      const std::string& publisher_blob);

  void GetPublisherBanner(const std::string& publisher_key,
                          ledger::PublisherBannerCallback callback);

  type::ActivityInfoFilterPtr CreateActivityFilter(
      const std::string& publisher_id,
      type::ExcludeFilter excluded,
      bool min_duration,
      const uint64_t& current_reconcile_stamp,
      bool non_verified,
      bool min_visits);

  void NormalizeContributeWinners(
      type::PublisherInfoList* newList,
      const type::PublisherInfoList* list,
      uint32_t /* next_record */);

  void OnRestorePublishers(
      const type::Result result,
      ledger::ResultCallback callback);

  bool IsConnectedOrVerified(const type::PublisherStatus status);

  void SynopsisNormalizer();

  void CalcScoreConsts(const int min_duration_seconds);

  void GetServerPublisherInfo(
      const std::string& publisher_key,
      client::GetServerPublisherInfoCallback callback);

  void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      const uint64_t duration);

  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      ledger::GetPublisherInfoCallback callback);

  void SavePublisherInfo(
      const uint64_t window_id,
      type::PublisherInfoPtr publisher_info,
      ledger::ResultCallback callback);

 private:
  void OnGetPublisherInfoForUpdateMediaDuration(
      type::Result result,
      type::PublisherInfoPtr info,
      const uint64_t window_id,
      const uint64_t duration);

  void OnGetPanelPublisherInfo(
      const type::Result result,
      type::PublisherInfoPtr info,
      ledger::GetPublisherInfoCallback callback);

  void onPublisherActivitySave(
      uint64_t windowId,
      const type::VisitData& visit_data,
      type::Result result,
      type::PublisherInfoPtr info);

  void OnGetActivityInfo(
      type::PublisherInfoList list,
      ledger::PublisherInfoCallback callback,
      const std::string& publisher_key);

  void SaveVisitInternal(
      const type::PublisherStatus,
      const std::string& publisher_key,
      const type::VisitData& visit_data,
      uint64_t duration,
      uint64_t window_id,
      const ledger::PublisherInfoCallback callback,
      type::Result result,
      type::PublisherInfoPtr publisher_info);

  void OnSaveVisitServerPublisher(
    type::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    const type::VisitData& visit_data,
    uint64_t duration,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback);

  void onFetchFavIcon(const std::string& publisher_key,
                      uint64_t window_id,
                      bool success,
                      const std::string& favicon_url);

  void onFetchFavIconDBResponse(type::Result result,
                                type::PublisherInfoPtr info,
                                const std::string& favicon_url,
                                uint64_t window_id);

  void OnSetPublisherExclude(
    type::PublisherExclude exclude,
    type::Result result,
    type::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback);

  double concaveScore(const uint64_t& duration_seconds);

  void SynopsisNormalizerCallback(type::PublisherInfoList list);

  void synopsisNormalizerInternal(type::PublisherInfoList* newList,
                                  const type::PublisherInfoList* list,
                                  uint32_t /* next_record */);

  void OnSaveVisitInternal(
    type::Result result,
    type::PublisherInfoPtr info);

  void OnPanelPublisherInfo(
      type::Result result,
      type::PublisherInfoPtr publisher_info,
      uint64_t windowId,
      const type::VisitData& visit_data);

  void OnGetPublisherBanner(
      type::ServerPublisherInfoPtr info,
      const std::string& publisher_key,
      ledger::PublisherBannerCallback callback);

  void OnGetPublisherBannerPublisher(
      ledger::PublisherBannerCallback callback,
      const type::PublisherBanner& banner,
      type::Result result,
      type::PublisherInfoPtr publisher_info);

  type::PublisherStatus ParsePublisherStatus(const std::string& status);

  void OnServerPublisherInfoLoaded(
      type::ServerPublisherInfoPtr server_info,
      const std::string& publisher_key,
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

#endif  // BRAVELEDGER_PUBLISHER_PUBLISHER_H_
