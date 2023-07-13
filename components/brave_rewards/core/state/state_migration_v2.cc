/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration_v2.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace state {

StateMigrationV2::StateMigrationV2(RewardsEngineImpl& engine)
    : engine_(engine) {}

StateMigrationV2::~StateMigrationV2() = default;

void StateMigrationV2::Migrate(LegacyResultCallback callback) {
  legacy_state_ = std::make_unique<LegacyBatState>(*engine_);

  auto load_callback =
      std::bind(&StateMigrationV2::OnLoadState, this, _1, callback);

  legacy_state_->Load(load_callback);
}

void StateMigrationV2::OnLoadState(mojom::Result result,
                                   LegacyResultCallback callback) {
  if (result == mojom::Result::NO_LEGACY_STATE) {
    BLOG(1, "No engine state");
    callback(mojom::Result::OK);
    return;
  }

  if (result != mojom::Result::OK) {
    BLOG(0, "Failed to load engine state file, setting default values");
    callback(mojom::Result::OK);
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

  engine_->SetState(kInlineTipRedditEnabled,
                    legacy_state_->GetInlineTipSetting("reddit"));

  engine_->SetState(kInlineTipTwitterEnabled,
                    legacy_state_->GetInlineTipSetting("twitter"));

  engine_->SetState(kInlineTipGithubEnabled,
                    legacy_state_->GetInlineTipSetting("github"));

  callback(mojom::Result::OK);
}

}  // namespace state
}  // namespace brave_rewards::internal
