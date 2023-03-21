/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REPORT_REPORT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REPORT_REPORT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger.h"

namespace ledger {
namespace report {

class Report {
 public:
  explicit Report(LedgerImpl* ledger);

  ~Report();

  void GetMonthly(const mojom::ActivityMonth month,
                  const int year,
                  ledger::GetMonthlyReportCallback callback);

  void GetAllMonthlyIds(ledger::GetAllMonthlyReportIdsCallback callback);

 private:
  void OnBalance(ledger::GetMonthlyReportCallback callback,
                 const mojom::ActivityMonth month,
                 const uint32_t year,
                 const mojom::Result result,
                 mojom::BalanceReportInfoPtr balance_report);

  void OnTransactions(
      std::vector<mojom::TransactionReportInfoPtr> transaction_report,
      const mojom::ActivityMonth month,
      const uint32_t year,
      std::shared_ptr<mojom::MonthlyReportInfoPtr> shared_report,
      ledger::GetMonthlyReportCallback callback);

  void OnContributions(
      std::vector<mojom::ContributionReportInfoPtr> contribution_report,
      std::shared_ptr<mojom::MonthlyReportInfoPtr> shared_report,
      ledger::GetMonthlyReportCallback callback);

  void OnGetAllBalanceReports(std::vector<mojom::BalanceReportInfoPtr> reports,
                              ledger::GetAllMonthlyReportIdsCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace report
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REPORT_REPORT_H_
