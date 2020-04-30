/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_INFO_H_
#define BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_INFO_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseBalanceReportInfo : public DatabaseTable {
 public:
  explicit DatabaseBalanceReportInfo(bat_ledger::LedgerImpl* ledger);
  ~DatabaseBalanceReportInfo() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::BalanceReportInfoPtr info,
      ledger::ResultCallback callback);

  void InsertOrUpdateItem(
      ledger::ActivityMonth month,
      int year,
      ledger::ReportType type,
      double amount,
      ledger::ResultCallback callback);

  void GetRecord(
      ledger::ActivityMonth month,
      int year,
      ledger::GetBalanceReportCallback callback);

  void GetAllRecords(
      ledger::GetBalanceReportListCallback callback);

  void DeleteAllRecords(
      ledger::ResultCallback callback);

 private:
  bool CreateTableV21(ledger::DBTransaction* transaction);

  bool CreateIndexV21(ledger::DBTransaction* transaction);

  bool MigrateToV21(ledger::DBTransaction* transaction);

  void UpdateItemAmount(
      ledger::BalanceReportInfoPtr info,
      ledger::ReportType type,
      double amount,
      ledger::ResultCallback callback);

  void OnGetRecord(
      ledger::DBCommandResponsePtr response,
      ledger::ActivityMonth month,
      int year,
      ledger::GetBalanceReportCallback callback);

  void OnGetAllRecords(
      ledger::DBCommandResponsePtr response,
      ledger::GetBalanceReportListCallback callback);

  void OnInsertOrUpdateInternal(
      ledger::Result result,
      ledger::ActivityMonth month,
      int year,
      ledger::GetBalanceReportCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_INFO_H_
