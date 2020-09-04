/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_H_
#define BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseBalanceReport : public DatabaseTable {
 public:
  explicit DatabaseBalanceReport(LedgerImpl* ledger);
  ~DatabaseBalanceReport() override;

  void InsertOrUpdate(
      type::BalanceReportInfoPtr info,
      ledger::ResultCallback callback);

  void InsertOrUpdateList(
      type::BalanceReportInfoList list,
      ledger::ResultCallback callback);

  void SetAmount(
      type::ActivityMonth month,
      int year,
      type::ReportType type,
      double amount,
      ledger::ResultCallback callback);

  void GetRecord(
      type::ActivityMonth month,
      int year,
      ledger::GetBalanceReportCallback callback);

  void GetAllRecords(
      ledger::GetBalanceReportListCallback callback);

  void DeleteAllRecords(
      ledger::ResultCallback callback);

 private:
  void OnGetRecord(
      type::DBCommandResponsePtr response,
      ledger::GetBalanceReportCallback callback);

  void OnGetAllRecords(
      type::DBCommandResponsePtr response,
      ledger::GetBalanceReportListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_H_
