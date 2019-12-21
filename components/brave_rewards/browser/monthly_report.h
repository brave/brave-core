/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_MONTHLY_REPORT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_MONTHLY_REPORT_H_

#include <vector>

#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/contribution_report_info.h"
#include "brave/components/brave_rewards/browser/transaction_report_info.h"

namespace brave_rewards {

struct MonthlyReport {
  MonthlyReport();
  ~MonthlyReport();
  MonthlyReport(const MonthlyReport& properties);

  BalanceReport balance;
  std::vector<TransactionReportInfo> transactions;
  std::vector<ContributionReportInfo> contributions;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_MONTHLY_REPORT_H_
