/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration_v1.h"

using std::placeholders::_1;

namespace brave_rewards::internal::state {

StateMigrationV1::StateMigrationV1() = default;

StateMigrationV1::~StateMigrationV1() = default;

void StateMigrationV1::Migrate(LegacyResultCallback callback) {
  legacy_publisher_ = std::make_unique<publisher::LegacyPublisherState>();

  auto load_callback =
      std::bind(&StateMigrationV1::OnLoadState, this, _1, callback);

  legacy_publisher_->Load(load_callback);
}

void StateMigrationV1::OnLoadState(mojom::Result result,
                                   LegacyResultCallback callback) {
  if (result == mojom::Result::NO_PUBLISHER_STATE) {
    BLOG(1, "No publisher state");
    ledger().publisher()->CalcScoreConsts(
        ledger().GetState<int>(kMinVisitTime));

    callback(mojom::Result::LEDGER_OK);
    return;
  }

  if (result != mojom::Result::LEDGER_OK) {
    ledger().publisher()->CalcScoreConsts(
        ledger().GetState<int>(kMinVisitTime));

    BLOG(0, "Failed to load publisher state file, setting default values");
    callback(mojom::Result::LEDGER_OK);
    return;
  }

  legacy_data_migrated_ = true;

  ledger().SetState(
      kMinVisitTime,
      static_cast<int>(legacy_publisher_->GetPublisherMinVisitTime()));
  ledger().publisher()->CalcScoreConsts(ledger().GetState<int>(kMinVisitTime));

  ledger().SetState(
      kMinVisits, static_cast<int>(legacy_publisher_->GetPublisherMinVisits()));

  std::vector<mojom::BalanceReportInfoPtr> reports;
  legacy_publisher_->GetAllBalanceReports(&reports);
  if (!reports.empty()) {
    auto save_callback =
        std::bind(&StateMigrationV1::BalanceReportsSaved, this, _1, callback);

    ledger().database()->SaveBalanceReportInfoList(std::move(reports),
                                                   save_callback);
  }
}

void StateMigrationV1::BalanceReportsSaved(mojom::Result result,
                                           LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Balance report save failed");
    callback(result);
    return;
  }
  callback(mojom::Result::LEDGER_OK);
}

}  // namespace brave_rewards::internal::state
