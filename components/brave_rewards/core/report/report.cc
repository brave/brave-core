/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/report/report.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal::report {

Report::Report(RewardsEngineImpl& engine) : engine_(engine) {}

Report::~Report() = default;

void Report::GetMonthly(mojom::ActivityMonth month,
                        int year,
                        GetMonthlyReportCallback callback) {
  engine_->database()->GetBalanceReportInfo(
      month, year,
      base::BindOnce(&Report::OnBalance, weak_factory_.GetWeakPtr(), month,
                     year, std::move(callback)));
}

void Report::OnBalance(mojom::ActivityMonth month,
                       uint32_t year,
                       GetMonthlyReportCallback callback,
                       mojom::Result result,
                       mojom::BalanceReportInfoPtr balance_report) {
  if (result != mojom::Result::OK || !balance_report) {
    engine_->LogError(FROM_HERE) << "Could not get balance report";
    std::move(callback).Run(result, nullptr);
    return;
  }

  auto monthly_report = mojom::MonthlyReportInfo::New();
  monthly_report->balance = std::move(balance_report);

  engine_->database()->GetContributionReport(
      month, year,
      base::BindOnce(&Report::OnContributions, weak_factory_.GetWeakPtr(),
                     std::move(monthly_report), std::move(callback)));
}

void Report::OnContributions(
    mojom::MonthlyReportInfoPtr report,
    GetMonthlyReportCallback callback,
    std::vector<mojom::ContributionReportInfoPtr> contribution_report) {
  if (!report) {
    engine_->LogError(FROM_HERE) << "Could not parse monthly report";
    std::move(callback).Run(mojom::Result::FAILED, nullptr);
    return;
  }

  report->contributions = std::move(contribution_report);

  std::move(callback).Run(mojom::Result::OK, std::move(report));
}

// This will be removed when we move reports in database and just order in db
bool CompareReportIds(const std::string& id_1, const std::string& id_2) {
  auto id_1_parts = base::SplitString(id_1, "_", base::TRIM_WHITESPACE,
                                      base::SPLIT_WANT_NONEMPTY);

  auto id_2_parts = base::SplitString(id_2, "_", base::TRIM_WHITESPACE,
                                      base::SPLIT_WANT_NONEMPTY);

  DCHECK(id_1_parts.size() == 2 && id_2_parts.size() == 2);

  int id_1_year = 0;
  base::StringToInt(id_1_parts[0], &id_1_year);
  int id_2_year = 0;
  base::StringToInt(id_2_parts[0], &id_2_year);
  int id_1_month = 0;
  base::StringToInt(id_1_parts[1], &id_1_month);
  int id_2_month = 0;
  base::StringToInt(id_2_parts[1], &id_2_month);

  if (id_1_year == id_2_year) {
    return id_1_month > id_2_month;
  }

  return id_1_year > id_2_year;
}

void Report::GetAllMonthlyIds(GetAllMonthlyReportIdsCallback callback) {
  engine_->database()->GetAllBalanceReports(
      base::BindOnce(&Report::OnGetAllBalanceReports,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void Report::OnGetAllBalanceReports(
    GetAllMonthlyReportIdsCallback callback,
    std::vector<mojom::BalanceReportInfoPtr> reports) {
  if (reports.empty()) {
    std::move(callback).Run({});
    return;
  }

  std::vector<std::string> ids;

  for (const auto& report : reports) {
    ids.push_back(report->id);
  }

  std::sort(ids.begin(), ids.end(), CompareReportIds);

  std::move(callback).Run(ids);
}

}  // namespace brave_rewards::internal::report
