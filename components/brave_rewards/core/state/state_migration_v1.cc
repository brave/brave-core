/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration_v1.h"

using std::placeholders::_1;

namespace ledger {
namespace state {

StateMigrationV1::StateMigrationV1(LedgerImpl* ledger) : ledger_(ledger) {}

StateMigrationV1::~StateMigrationV1() = default;

void StateMigrationV1::Migrate(ledger::LegacyResultCallback callback) {
  legacy_publisher_ =
      std::make_unique<publisher::LegacyPublisherState>(ledger_);

  auto load_callback =
      std::bind(&StateMigrationV1::OnLoadState, this, _1, callback);

  legacy_publisher_->Load(load_callback);
}

void StateMigrationV1::OnLoadState(mojom::Result result,
                                   ledger::LegacyResultCallback callback) {
  if (result == mojom::Result::NO_PUBLISHER_STATE) {
    BLOG(1, "No publisher state");
    ledger_->publisher()->CalcScoreConsts(
        ledger_->GetState<int>(kMinVisitTime));

    callback(mojom::Result::LEDGER_OK);
    return;
  }

  if (result != mojom::Result::LEDGER_OK) {
    ledger_->publisher()->CalcScoreConsts(
        ledger_->GetState<int>(kMinVisitTime));

    BLOG(0, "Failed to load publisher state file, setting default values");
    callback(mojom::Result::LEDGER_OK);
    return;
  }

  legacy_data_migrated_ = true;

  ledger_->SetState(
      kMinVisitTime,
      static_cast<int>(legacy_publisher_->GetPublisherMinVisitTime()));
  ledger_->publisher()->CalcScoreConsts(ledger_->GetState<int>(kMinVisitTime));

  ledger_->SetState(
      kMinVisits, static_cast<int>(legacy_publisher_->GetPublisherMinVisits()));

  ledger_->SetState(kAllowNonVerified,
                    legacy_publisher_->GetPublisherAllowNonVerified());

  std::vector<mojom::BalanceReportInfoPtr> reports;
  legacy_publisher_->GetAllBalanceReports(&reports);
  if (!reports.empty()) {
    auto save_callback =
        std::bind(&StateMigrationV1::BalanceReportsSaved, this, _1, callback);

    ledger_->database()->SaveBalanceReportInfoList(std::move(reports),
                                                   save_callback);
  }
}

void StateMigrationV1::BalanceReportsSaved(
    mojom::Result result,
    ledger::LegacyResultCallback callback) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Balance report save failed");
    callback(result);
    return;
  }
  callback(mojom::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
