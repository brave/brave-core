/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/database/database_multi_tables.h"
#include "brave/components/brave_rewards/core/promotion/promotion_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace database {

DatabaseMultiTables::DatabaseMultiTables(RewardsEngineImpl& engine)
    : engine_(engine) {}

DatabaseMultiTables::~DatabaseMultiTables() = default;

void DatabaseMultiTables::GetTransactionReport(
    const mojom::ActivityMonth month,
    const int year,
    GetTransactionReportCallback callback) {
  auto promotion_callback =
      std::bind(&DatabaseMultiTables::OnGetTransactionReportPromotion, this, _1,
                month, year, callback);
  engine_->database()->GetAllPromotions(promotion_callback);
}

void DatabaseMultiTables::OnGetTransactionReportPromotion(
    base::flat_map<std::string, mojom::PromotionPtr> promotions,
    const mojom::ActivityMonth month,
    const int year,
    GetTransactionReportCallback callback) {
  const auto converted_month = static_cast<int>(month);
  std::vector<mojom::TransactionReportInfoPtr> list;

  for (const auto& promotion : promotions) {
    if (!promotion.second ||
        promotion.second->status != mojom::PromotionStatus::FINISHED ||
        promotion.second->claimed_at == 0) {
      continue;
    }

    base::Time time =
        base::Time::FromSecondsSinceUnixEpoch(promotion.second->claimed_at);
    base::Time::Exploded exploded;
    time.LocalExplode(&exploded);
    if (exploded.year != year || exploded.month != converted_month) {
      continue;
    }

    auto report = mojom::TransactionReportInfo::New();
    report->type =
        promotion::ConvertPromotionTypeToReportType(promotion.second->type);
    report->amount = promotion.second->approximate_value;
    report->created_at = promotion.second->claimed_at;
    list.push_back(std::move(report));
  }

  callback(std::move(list));
}

}  // namespace database
}  // namespace brave_rewards::internal
