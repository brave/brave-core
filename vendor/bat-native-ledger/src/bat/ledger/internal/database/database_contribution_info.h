/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
#define BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_contribution_info_publishers.h"
#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetContributionInfoCallback =
    std::function<void(type::ContributionInfoPtr)>;

class DatabaseContributionInfo: public DatabaseTable {
 public:
  explicit DatabaseContributionInfo(LedgerImpl* ledger);
  ~DatabaseContributionInfo() override;

  void InsertOrUpdate(
      type::ContributionInfoPtr info,
      ledger::ResultCallback callback);

  void GetRecord(
      const std::string& contribution_id,
      GetContributionInfoCallback callback);

  void GetAllRecords(ledger::ContributionInfoListCallback callback);

  void GetOneTimeTips(
      const type::ActivityMonth month,
      const int year,
      ledger::PublisherInfoListCallback callback);

  void GetContributionReport(
      const type::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback);

  void GetNotCompletedRecords(ledger::ContributionInfoListCallback callback);

  void UpdateStep(
      const std::string& contribution_id,
      const type::ContributionStep step,
      ledger::ResultCallback callback);

  void UpdateStepAndCount(
      const std::string& contribution_id,
      const type::ContributionStep step,
      const int32_t retry_count,
      ledger::ResultCallback callback);

  void UpdateContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void FinishAllInProgressRecords(ledger::ResultCallback callback);

 private:
  void OnGetRecord(
      type::DBCommandResponsePtr response,
      GetContributionInfoCallback callback);

  void OnGetPublishers(
      type::ContributionPublisherList list,
      std::shared_ptr<type::ContributionInfoPtr> shared_contribution,
      GetContributionInfoCallback callback);

  void OnGetOneTimeTips(
      type::DBCommandResponsePtr response,
      ledger::PublisherInfoListCallback callback);

  void OnGetContributionReport(
      type::DBCommandResponsePtr response,
      ledger::GetContributionReportCallback callback);

  void OnGetContributionReportPublishers(
      std::vector<ContributionPublisherInfoPair> publisher_pair_list,
      std::shared_ptr<type::ContributionInfoList> shared_contributions,
      ledger::GetContributionReportCallback callback);

  void OnGetList(
      type::DBCommandResponsePtr response,
      ledger::ContributionInfoListCallback callback);

  void OnGetListPublishers(
      type::ContributionPublisherList list,
      std::shared_ptr<type::ContributionInfoList> shared_contributions,
      ledger::ContributionInfoListCallback callback);

  std::unique_ptr<DatabaseContributionInfoPublishers> publishers_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
