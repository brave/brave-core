/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_BALANCE_REPORT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_BALANCE_REPORT_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseBalanceReport : public DatabaseTable {
 public:
  explicit DatabaseBalanceReport(LedgerImpl& ledger);
  ~DatabaseBalanceReport() override;

  void InsertOrUpdate(mojom::BalanceReportInfoPtr info,
                      ledger::LegacyResultCallback callback);

  void InsertOrUpdateList(std::vector<mojom::BalanceReportInfoPtr> list,
                          ledger::LegacyResultCallback callback);

  void SetAmount(mojom::ActivityMonth month,
                 int year,
                 mojom::ReportType type,
                 double amount,
                 ledger::LegacyResultCallback callback);

  void GetRecord(mojom::ActivityMonth month,
                 int year,
                 ledger::GetBalanceReportCallback callback);

  void GetAllRecords(ledger::GetBalanceReportListCallback callback);

  void DeleteAllRecords(ledger::LegacyResultCallback callback);

 private:
  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   ledger::GetBalanceReportCallback callback);

  void OnGetAllRecords(mojom::DBCommandResponsePtr response,
                       ledger::GetBalanceReportListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_BALANCE_REPORT_H_
