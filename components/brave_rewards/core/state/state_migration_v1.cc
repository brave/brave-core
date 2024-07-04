/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration_v1.h"

namespace brave_rewards::internal::state {

StateMigrationV1::StateMigrationV1(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV1::~StateMigrationV1() = default;

void StateMigrationV1::Migrate(ResultCallback callback) {
  legacy_publisher_ =
      std::make_unique<publisher::LegacyPublisherState>(*engine_);

  legacy_publisher_->Load(base::BindOnce(&StateMigrationV1::OnLoadState,
                                         weak_factory_.GetWeakPtr(),
                                         std::move(callback)));
}

void StateMigrationV1::OnLoadState(ResultCallback callback,
                                   mojom::Result result) {
  if (result == mojom::Result::NO_PUBLISHER_STATE) {
    engine_->Log(FROM_HERE) << "No publisher state";
    engine_->publisher()->CalcScoreConsts(
        engine_->GetState<int>(kMinVisitTime));

    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  if (result != mojom::Result::OK) {
    engine_->publisher()->CalcScoreConsts(
        engine_->GetState<int>(kMinVisitTime));

    engine_->LogError(FROM_HERE)
        << "Failed to load publisher state file, setting default values";
    std::move(callback).Run(mojom::Result::OK);
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
    engine_->database()->SaveBalanceReportInfoList(
        std::move(reports),
        base::BindOnce(&StateMigrationV1::BalanceReportsSaved,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
  }
}

void StateMigrationV1::BalanceReportsSaved(ResultCallback callback,
                                           mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Balance report save failed";
    std::move(callback).Run(result);
    return;
  }
  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace brave_rewards::internal::state
