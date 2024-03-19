/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REPORT_REPORT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REPORT_REPORT_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal::report {

class Report {
 public:
  explicit Report(RewardsEngineImpl& engine);

  ~Report();

  void GetMonthly(mojom::ActivityMonth month,
                  int year,
                  GetMonthlyReportCallback callback);

  void GetAllMonthlyIds(GetAllMonthlyReportIdsCallback callback);

 private:
  void OnBalance(mojom::ActivityMonth month,
                 uint32_t year,
                 GetMonthlyReportCallback callback,
                 mojom::Result result,
                 mojom::BalanceReportInfoPtr balance_report);

  void OnContributions(
      mojom::MonthlyReportInfoPtr report,
      GetMonthlyReportCallback callback,
      std::vector<mojom::ContributionReportInfoPtr> contribution_report);

  void OnGetAllBalanceReports(GetAllMonthlyReportIdsCallback callback,
                              std::vector<mojom::BalanceReportInfoPtr> reports);

  const raw_ref<RewardsEngineImpl> engine_;
  base::WeakPtrFactory<Report> weak_factory_{this};
};

}  // namespace brave_rewards::internal::report

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REPORT_REPORT_H_
