/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_multi_tables.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/promotion/promotion_util.h"

using std::placeholders::_1;

namespace braveledger_database {

DatabaseMultiTables::DatabaseMultiTables(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  ledger_ = ledger;
}

DatabaseMultiTables::~DatabaseMultiTables() = default;

void DatabaseMultiTables::GetTransactionReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetTransactionReportCallback callback) {
  auto promotion_callback = std::bind(
      &DatabaseMultiTables::OnGetTransactionReportPromotion,
      this,
      _1,
      month,
      year,
      callback);
  ledger_->database()->GetAllPromotions(promotion_callback);
}

void DatabaseMultiTables::OnGetTransactionReportPromotion(
    ledger::PromotionMap promotions,
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetTransactionReportCallback callback) {
  const auto converted_month = static_cast<int>(month);
  ledger::TransactionReportInfoList list;

  for (const auto& promotion : promotions) {
    if (!promotion.second ||
        promotion.second->status != ledger::PromotionStatus::FINISHED ||
        promotion.second->claimed_at == 0) {
      continue;
    }

    base::Time time = base::Time::FromDoubleT(promotion.second->claimed_at);
    base::Time::Exploded exploded;
    time.LocalExplode(&exploded);
    if (exploded.year != year || exploded.month != converted_month) {
      continue;
    }

    auto report = ledger::TransactionReportInfo::New();
    report->type = braveledger_promotion::ConvertPromotionTypeToReportType(
        promotion.second->type);
    report->amount = promotion.second->approximate_value;
    report->created_at = promotion.second->claimed_at;
    list.push_back(std::move(report));
  }

  callback(std::move(list));
}

}  // namespace braveledger_database
