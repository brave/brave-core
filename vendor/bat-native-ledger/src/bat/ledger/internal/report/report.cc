/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <utility>
#include <vector>
#include <iostream>

#include "base/strings/string_split.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/report/report.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace report {

Report::Report(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

Report::~Report() = default;

void Report::GetMonthly(const mojom::ActivityMonth month,
                        const int year,
                        ledger::GetMonthlyReportCallback callback) {
  auto balance_callback = std::bind(&Report::OnBalance,
      this,
      _1,
      _2,
      month,
      year,
      callback);

  ledger_->database()->GetBalanceReportInfo(month, year, balance_callback);
}

void Report::OnBalance(const mojom::Result result,
                       mojom::BalanceReportInfoPtr balance_report,
                       const mojom::ActivityMonth month,
                       const uint32_t year,
                       ledger::GetMonthlyReportCallback callback) {
  if (result != mojom::Result::LEDGER_OK || !balance_report) {
    BLOG(0, "Could not get balance report");
    callback(result, nullptr);
    return;
  }

  auto monthly_report = mojom::MonthlyReportInfo::New();
  monthly_report->balance = std::move(balance_report);

  auto transaction_callback = std::bind(
      &Report::OnTransactions, this, _1, month, year,
      std::make_shared<mojom::MonthlyReportInfoPtr>(std::move(monthly_report)),
      callback);

  ledger_->database()->GetTransactionReport(month, year, transaction_callback);
}

void Report::OnTransactions(
    std::vector<mojom::TransactionReportInfoPtr> transaction_report,
    const mojom::ActivityMonth month,
    const uint32_t year,
    std::shared_ptr<mojom::MonthlyReportInfoPtr> shared_report,
    ledger::GetMonthlyReportCallback callback) {
  if (!shared_report) {
    BLOG(0, "Could not parse monthly report");
    callback(mojom::Result::LEDGER_ERROR, nullptr);
    return;
  }

  (*shared_report)->transactions = std::move(transaction_report);

  auto contribution_callback = std::bind(&Report::OnContributions,
      this,
      _1,
      shared_report,
      callback);

  ledger_->database()->GetContributionReport(
      month,
      year,
      contribution_callback);
}

void Report::OnContributions(
    std::vector<mojom::ContributionReportInfoPtr> contribution_report,
    std::shared_ptr<mojom::MonthlyReportInfoPtr> shared_report,
    ledger::GetMonthlyReportCallback callback) {
  if (!shared_report) {
    BLOG(0, "Could not parse monthly report");
    callback(mojom::Result::LEDGER_ERROR, nullptr);
    return;
  }

  (*shared_report)->contributions = std::move(contribution_report);

  callback(mojom::Result::LEDGER_OK, std::move(*shared_report));
}

// This will be removed when we move reports in database and just order in db
bool CompareReportIds(const std::string& id_1, const std::string& id_2) {
  auto id_1_parts = base::SplitString(
      id_1,
      "_",
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);

  auto id_2_parts = base::SplitString(
      id_2,
      "_",
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);

  DCHECK(id_1_parts.size() == 2 && id_2_parts.size() == 2);

  const int id_1_year = std::stoi(id_1_parts[0]);
  const int id_2_year = std::stoi(id_2_parts[0]);
  const int id_1_month = std::stoi(id_1_parts[1]);
  const int id_2_month = std::stoi(id_2_parts[1]);

  if (id_1_year == id_2_year) {
    return id_1_month > id_2_month;
  }

  return id_1_year > id_2_year;
}

void Report::GetAllMonthlyIds(ledger::GetAllMonthlyReportIdsCallback callback) {
  auto balance_reports_callback = std::bind(&Report::OnGetAllBalanceReports,
      this,
      _1,
      callback);

  ledger_->database()->GetAllBalanceReports(balance_reports_callback);
}

void Report::OnGetAllBalanceReports(
    std::vector<mojom::BalanceReportInfoPtr> reports,
    ledger::GetAllMonthlyReportIdsCallback callback) {
  if (reports.empty()) {
    callback({});
    return;
  }

  std::vector<std::string> ids;

  for (const auto& report : reports) {
    ids.push_back(report->id);
  }

  std::sort(ids.begin(), ids.end(), CompareReportIds);

  callback(ids);
}

}  // namespace report
}  // namespace ledger
