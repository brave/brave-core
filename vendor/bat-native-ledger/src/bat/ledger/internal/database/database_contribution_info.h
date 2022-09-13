/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_INFO_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_contribution_info_publishers.h"
#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetContributionInfoCallback =
    std::function<void(mojom::ContributionInfoPtr)>;

class DatabaseContributionInfo: public DatabaseTable {
 public:
  explicit DatabaseContributionInfo(LedgerImpl* ledger);
  ~DatabaseContributionInfo() override;

  void InsertOrUpdate(mojom::ContributionInfoPtr info,
                      ledger::LegacyResultCallback callback);

  void GetRecord(
      const std::string& contribution_id,
      GetContributionInfoCallback callback);

  void GetAllRecords(ledger::ContributionInfoListCallback callback);

  void GetOneTimeTips(const mojom::ActivityMonth month,
                      const int year,
                      ledger::PublisherInfoListCallback callback);

  void GetContributionReport(const mojom::ActivityMonth month,
                             const int year,
                             ledger::GetContributionReportCallback callback);

  void GetNotCompletedRecords(ledger::ContributionInfoListCallback callback);

  void UpdateStep(const std::string& contribution_id,
                  mojom::ContributionStep step,
                  ledger::LegacyResultCallback callback);

  void UpdateStepAndCount(const std::string& contribution_id,
                          mojom::ContributionStep step,
                          int32_t retry_count,
                          ledger::LegacyResultCallback callback);

  void UpdateContributedAmount(const std::string& contribution_id,
                               const std::string& publisher_key,
                               ledger::LegacyResultCallback callback);

  void FinishAllInProgressRecords(ledger::LegacyResultCallback callback);

 private:
  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   GetContributionInfoCallback callback);

  void OnGetPublishers(
      std::vector<mojom::ContributionPublisherPtr> list,
      std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution,
      GetContributionInfoCallback callback);

  void OnGetOneTimeTips(mojom::DBCommandResponsePtr response,
                        ledger::PublisherInfoListCallback callback);

  void OnGetContributionReport(mojom::DBCommandResponsePtr response,
                               ledger::GetContributionReportCallback callback);

  void OnGetContributionReportPublishers(
      std::vector<ContributionPublisherInfoPair> publisher_pair_list,
      std::shared_ptr<std::vector<mojom::ContributionInfoPtr>>
          shared_contributions,
      ledger::GetContributionReportCallback callback);

  void OnGetList(mojom::DBCommandResponsePtr response,
                 ledger::ContributionInfoListCallback callback);

  void OnGetListPublishers(
      std::vector<mojom::ContributionPublisherPtr> list,
      std::shared_ptr<std::vector<mojom::ContributionInfoPtr>>
          shared_contributions,
      ledger::ContributionInfoListCallback callback);

  std::unique_ptr<DatabaseContributionInfoPublishers> publishers_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
