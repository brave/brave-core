/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration_v2.h"

namespace brave_rewards::internal::state {

StateMigrationV2::StateMigrationV2(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV2::~StateMigrationV2() = default;

void StateMigrationV2::Migrate(ResultCallback callback) {
  legacy_state_ = std::make_unique<LegacyBatState>(*engine_);

  legacy_state_->Load(base::BindOnce(&StateMigrationV2::OnLoadState,
                                     weak_factory_.GetWeakPtr(),
                                     std::move(callback)));
}

void StateMigrationV2::OnLoadState(ResultCallback callback,
                                   mojom::Result result) {
  if (result == mojom::Result::NO_LEGACY_STATE) {
    engine_->Log(FROM_HERE) << "No engine state";
    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE)
        << "Failed to load engine state file, setting default values";
    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  engine_->SetState("enabled", legacy_state_->GetRewardsMainEnabled());

  engine_->SetState(kAutoContributeEnabled,
                    legacy_state_->GetAutoContributeEnabled());

  if (legacy_state_->GetUserChangedContribution()) {
    engine_->SetState(kAutoContributeAmount,
                      legacy_state_->GetAutoContributionAmount());
  }

  engine_->SetState(kNextReconcileStamp, legacy_state_->GetReconcileStamp());

  engine_->SetState(kCreationStamp, legacy_state_->GetCreationStamp());

  const auto seed = legacy_state_->GetRecoverySeed();
  engine_->SetState(kRecoverySeed, base::Base64Encode(seed));

  engine_->SetState(kPaymentId, legacy_state_->GetPaymentId());

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace brave_rewards::internal::state
