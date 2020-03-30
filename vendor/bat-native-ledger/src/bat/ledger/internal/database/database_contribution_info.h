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

namespace braveledger_database {

class DatabaseContributionInfo: public DatabaseTable {
 public:
  explicit DatabaseContributionInfo(bat_ledger::LedgerImpl* ledger);
  ~DatabaseContributionInfo() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::ContributionInfoPtr info,
      ledger::ResultCallback callback);

  void GetRecord(
      const std::string& contribution_id,
      ledger::GetContributionInfoCallback callback);

  void GetAllRecords(ledger::ContributionInfoListCallback callback);

  void GetOneTimeTips(
      const ledger::ActivityMonth month,
      const int year,
      ledger::PublisherInfoListCallback callback);

  void GetContributionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback);

  void GetNotCompletedRecords(ledger::ContributionInfoListCallback callback);

  void UpdateStep(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      ledger::ResultCallback callback);

  void UpdateStepAndCount(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count,
      ledger::ResultCallback callback);

  void UpdateContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

 private:
  bool CreateTableV2(ledger::DBTransaction* transaction);

  bool CreateTableV8(ledger::DBTransaction* transaction);

  bool CreateTableV11(ledger::DBTransaction* transaction);

  bool CreateIndexV2(ledger::DBTransaction* transaction);

  bool CreateIndexV8(ledger::DBTransaction* transaction);

  bool MigrateToV2(ledger::DBTransaction* transaction);

  bool MigrateToV8(ledger::DBTransaction* transaction);

  bool MigrateToV11(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  bool MigrateToV16(ledger::DBTransaction* transaction);

  bool MigrateToV17(ledger::DBTransaction* transaction);

  void OnGetRecord(
      ledger::DBCommandResponsePtr response,
      ledger::GetContributionInfoCallback callback);

  void OnGetPublishers(
      ledger::ContributionPublisherList list,
      const std::string& contribution_string,
      ledger::GetContributionInfoCallback callback);

  void OnGetOneTimeTips(
      ledger::DBCommandResponsePtr response,
      ledger::PublisherInfoListCallback callback);

  void OnGetContributionReport(
      ledger::DBCommandResponsePtr response,
      ledger::GetContributionReportCallback callback);

  void OnGetContributionReportPublishers(
      std::vector<ContributionPublisherInfoPair> publisher_pair_list,
      const std::string& contribution_list_string,
      ledger::GetContributionReportCallback callback);

  void OnGetList(
      ledger::DBCommandResponsePtr response,
      ledger::ContributionInfoListCallback callback);

  void OnGetListPublishers(
      ledger::ContributionPublisherList list,
      const std::string& contribution_list_string,
      ledger::ContributionInfoListCallback callback);

  std::unique_ptr<DatabaseContributionInfoPublishers> publishers_;
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
