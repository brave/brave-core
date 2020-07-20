/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_H_
#define BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseBalanceReport : public DatabaseTable {
 public:
  explicit DatabaseBalanceReport(bat_ledger::LedgerImpl* ledger);
  ~DatabaseBalanceReport() override;

  void InsertOrUpdate(
      ledger::BalanceReportInfoPtr info,
      ledger::ResultCallback callback);

  void InsertOrUpdateList(
      ledger::BalanceReportInfoList list,
      ledger::ResultCallback callback);

  void SetAmount(
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
  void OnGetRecord(
      ledger::DBCommandResponsePtr response,
      ledger::GetBalanceReportCallback callback);

  void OnGetAllRecords(
      ledger::DBCommandResponsePtr response,
      ledger::GetBalanceReportListCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_BALANCE_REPORT_H_
