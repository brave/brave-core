/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration_v1.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace state {

StateMigrationV1::StateMigrationV1(RewardsEngineImpl& engine)
    : engine_(engine) {}

StateMigrationV1::~StateMigrationV1() = default;

void StateMigrationV1::Migrate(LegacyResultCallback callback) {
  legacy_publisher_ =
      std::make_unique<publisher::LegacyPublisherState>(*engine_);

  auto load_callback =
      std::bind(&StateMigrationV1::OnLoadState, this, _1, callback);

  legacy_publisher_->Load(load_callback);
}

void StateMigrationV1::OnLoadState(mojom::Result result,
                                   LegacyResultCallback callback) {
  if (result == mojom::Result::NO_PUBLISHER_STATE) {
    engine_->Log(FROM_HERE) << "No publisher state";
    engine_->publisher()->CalcScoreConsts(
        engine_->GetState<int>(kMinVisitTime));

    callback(mojom::Result::OK);
    return;
  }

  if (result != mojom::Result::OK) {
    engine_->publisher()->CalcScoreConsts(
        engine_->GetState<int>(kMinVisitTime));

    engine_->LogError(FROM_HERE)
        << "Failed to load publisher state file, setting default values";
    callback(mojom::Result::OK);
    return;
  }

  legacy_data_migrated_ = true;

  engine_->SetState(
      kMinVisitTime,
      static_cast<int>(legacy_publisher_->GetPublisherMinVisitTime()));
  engine_->publisher()->CalcScoreConsts(engine_->GetState<int>(kMinVisitTime));

  engine_->SetState(
      kMinVisits, static_cast<int>(legacy_publisher_->GetPublisherMinVisits()));

  std::vector<mojom::BalanceReportInfoPtr> reports;
  legacy_publisher_->GetAllBalanceReports(&reports);
  if (!reports.empty()) {
    auto save_callback =
        std::bind(&StateMigrationV1::BalanceReportsSaved, this, _1, callback);

    engine_->database()->SaveBalanceReportInfoList(std::move(reports),
                                                   save_callback);
  }
}

void StateMigrationV1::BalanceReportsSaved(mojom::Result result,
                                           LegacyResultCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Balance report save failed";
    callback(result);
    return;
  }
  callback(mojom::Result::OK);
}

}  // namespace state
}  // namespace brave_rewards::internal
