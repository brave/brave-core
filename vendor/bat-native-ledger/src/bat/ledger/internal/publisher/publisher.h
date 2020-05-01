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
#include "bat/ledger/internal/properties/publisher_settings_properties.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
struct PublisherSettings;
}

namespace braveledger_publisher {

class PublisherServerList;

using ParsePublisherListCallback = std::function<void(const ledger::Result)>;
using DownloadServerPublisherListCallback =
    std::function<void(const ledger::Result)>;

class Publisher {
 public:
  explicit Publisher(bat_ledger::LedgerImpl* ledger);

  ~Publisher();

  // Called when timer is triggered
  void OnTimer(uint32_t timer_id);

  void RefreshPublisher(
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback);

  void SetPublisherServerListTimer(const bool rewards_enabled);

  bool loadState(const std::string& data);

  void SaveVisit(const std::string& publisher_key,
                 const ledger::VisitData& visit_data,
                 const uint64_t& duration,
                 uint64_t window_id,
                 const ledger::PublisherInfoCallback callback);

  void setPublisherMinVisitTime(const uint64_t& duration);  // In seconds

  void setPublisherMinVisits(const unsigned int visits);

  void SetPublisherExclude(
      const std::string& publisher_id,
      const ledger::PublisherExclude& exclude,
      ledger::ResultCallback callback);

  void setPublisherAllowNonVerified(const bool& allow);

  void setPublisherAllowVideos(const bool& allow);

  void setBalanceReport(ledger::ActivityMonth month,
                        int year,
                        const ledger::BalanceReportInfo& report_info);

  void GetBalanceReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetBalanceReportCallback callback);

  std::map<std::string, ledger::BalanceReportInfoPtr> GetAllBalanceReports();

  uint64_t getPublisherMinVisitTime() const;  // In milliseconds

  unsigned int GetPublisherMinVisits() const;

  bool getPublisherAllowNonVerified() const;

  bool getPublisherAllowVideos() const;

  void OnPublisherInfoSaved(const ledger::Result result);

  std::string GetBalanceReportName(ledger::ActivityMonth month, int year);

  void ParsePublisherList(
      const std::string& data,
      ParsePublisherListCallback callback);

  void getPublisherActivityFromUrl(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& publisher_blob);

  void GetPublisherBanner(const std::string& publisher_key,
                          ledger::PublisherBannerCallback callback);

  void SetBalanceReportItem(
      const ledger::ActivityMonth month,
      const int year,
      const ledger::ReportType type,
      const double amount);

  ledger::ActivityInfoFilterPtr CreateActivityFilter(
      const std::string& publisher_id,
      ledger::ExcludeFilter excluded,
      bool min_duration,
      const uint64_t& current_reconcile_stamp,
      bool non_verified,
      bool min_visits);

  void clearAllBalanceReports();

  void NormalizeContributeWinners(ledger::PublisherInfoList* newList,
                                  const ledger::PublisherInfoList* list,
                                  uint32_t /* next_record */);

  void SavePublisherProcessed(const std::string& publisher_key);

  bool WasPublisherAlreadyProcessed(const std::string& publisher_key) const;

  void OnRestorePublishers(
      const ledger::Result result,
      ledger::ResultCallback callback);

  bool IsConnectedOrVerified(const ledger::PublisherStatus status);

 private:
  void OnRefreshPublisher(
    const ledger::Result result,
    const std::string& publisher_key,
    ledger::OnRefreshPublisherCallback callback);

  void OnRefreshPublisherServerPublisher(
    ledger::ServerPublisherInfoPtr info,
    ledger::OnRefreshPublisherCallback callback);

  void onPublisherActivitySave(uint64_t windowId,
                               const ledger::VisitData& visit_data,
                               ledger::Result result,
                               ledger::PublisherInfoPtr info);

  void OnPublisherStateSaved(const ledger::Result result);

  bool IsExcluded(
      const std::string& publisher_id,
      const bool server_exclude,
      const ledger::PublisherExclude& excluded);

  void SaveVisitInternal(
      const ledger::PublisherStatus,
      bool server_excluded,
      const std::string& publisher_key,
      const ledger::VisitData& visit_data,
      uint64_t duration,
      uint64_t window_id,
      const ledger::PublisherInfoCallback callback,
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

  void OnSaveVisitServerPublisher(
    ledger::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    const ledger::VisitData& visit_data,
    uint64_t duration,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback);

  ledger::Result GetBalanceReportInternal(
      const ledger::ActivityMonth month,
      const int year,
      ledger::BalanceReportInfo* report_info);

  void onFetchFavIcon(const std::string& publisher_key,
                      uint64_t window_id,
                      bool success,
                      const std::string& favicon_url);

  void onFetchFavIconDBResponse(ledger::Result result,
                                ledger::PublisherInfoPtr info,
                                const std::string& favicon_url,
                                uint64_t window_id);

  void OnSetPublisherExclude(
    ledger::PublisherExclude exclude,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback);

  void calcScoreConsts(const uint64_t& min_duration_seconds);

  double concaveScore(const uint64_t& duration_seconds);

  void saveState();

  void SynopsisNormalizer();

  void SynopsisNormalizerCallback(ledger::PublisherInfoList list);

  void synopsisNormalizerInternal(ledger::PublisherInfoList* newList,
                                  const ledger::PublisherInfoList* list,
                                  uint32_t /* next_record */);

  bool GetMigrateScore() const;

  void SetMigrateScore(bool value);

  void OnSaveVisitInternal(
    ledger::Result result,
    ledger::PublisherInfoPtr info);

  void OnPanelPublisherInfo(
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info,
      uint64_t windowId,
      const ledger::VisitData& visit_data);

  void OnGetPublisherBanner(
      ledger::ServerPublisherInfoPtr info,
      const std::string& publisher_key,
      ledger::PublisherBannerCallback callback);

  void OnGetPublisherBannerPublisher(
      ledger::PublisherBannerCallback callback,
      const ledger::PublisherBanner& banner,
      ledger::Result result,
      ledger::PublisherInfoPtr publisher_info);

  ledger::PublisherStatus ParsePublisherStatus(const std::string& status);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::PublisherSettingsProperties> state_;
  std::unique_ptr<PublisherServerList> server_list_;

  double a_;

  double a2_;

  double a4_;

  double b_;

  double b2_;

  // For testing purposes
  friend class PublisherTest;
  FRIEND_TEST_ALL_PREFIXES(PublisherTest, calcScoreConsts);
  FRIEND_TEST_ALL_PREFIXES(PublisherTest, concaveScore);
  FRIEND_TEST_ALL_PREFIXES(PublisherTest, synopsisNormalizerInternal);
};

}  // namespace braveledger_publisher

#endif  // BRAVELEDGER_PUBLISHER_PUBLISHER_H_
