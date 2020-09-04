/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_REPORT_REPORT_H_
#define BRAVELEDGER_REPORT_REPORT_H_

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
namespace report {

class Report {
 public:
  explicit Report(LedgerImpl* ledger);

  ~Report();

  void GetMonthly(
      const type::ActivityMonth month,
      const int year,
      ledger::GetMonthlyReportCallback callback);

  void GetAllMonthlyIds(ledger::GetAllMonthlyReportIdsCallback callback);

 private:
  void OnBalance(
      const type::Result result,
      type::BalanceReportInfoPtr balance_report,
      const type::ActivityMonth month,
      const uint32_t year,
      ledger::GetMonthlyReportCallback callback);

  void OnTransactions(
      type::TransactionReportInfoList transaction_report,
      const type::ActivityMonth month,
      const uint32_t year,
      std::shared_ptr<type::MonthlyReportInfoPtr> shared_report,
      ledger::GetMonthlyReportCallback callback);

  void OnContributions(
      type::ContributionReportInfoList contribution_report,
      std::shared_ptr<type::MonthlyReportInfoPtr> shared_report,
      ledger::GetMonthlyReportCallback callback);

  void OnGetAllBalanceReports(
      type::BalanceReportInfoList reports,
      ledger::GetAllMonthlyReportIdsCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace report
}  // namespace ledger

#endif  // BRAVELEDGER_REPORT_REPORT_H_
