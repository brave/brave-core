/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration_v2.h"

using std::placeholders::_1;

namespace ledger {
namespace state {

StateMigrationV2::StateMigrationV2(LedgerImpl& ledger) : ledger_(ledger) {}

StateMigrationV2::~StateMigrationV2() = default;

void StateMigrationV2::Migrate(ledger::LegacyResultCallback callback) {
  legacy_state_ =
      std::make_unique<braveledger_bat_state::LegacyBatState>(*ledger_);

  auto load_callback =
      std::bind(&StateMigrationV2::OnLoadState, this, _1, callback);

  legacy_state_->Load(load_callback);
}

void StateMigrationV2::OnLoadState(mojom::Result result,
                                   ledger::LegacyResultCallback callback) {
  if (result == mojom::Result::NO_LEDGER_STATE) {
    BLOG(1, "No ledger state");
    callback(mojom::Result::LEDGER_OK);
    return;
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to load ledger state file, setting default values");
    callback(mojom::Result::LEDGER_OK);
    return;
  }

  ledger_->SetState("enabled", legacy_state_->GetRewardsMainEnabled());

  ledger_->SetState(kAutoContributeEnabled,
                    legacy_state_->GetAutoContributeEnabled());

  if (legacy_state_->GetUserChangedContribution()) {
    ledger_->SetState(kAutoContributeAmount,
                      legacy_state_->GetAutoContributionAmount());
  }

  ledger_->SetState(kNextReconcileStamp, legacy_state_->GetReconcileStamp());

  ledger_->SetState(kCreationStamp, legacy_state_->GetCreationStamp());

  const auto seed = legacy_state_->GetRecoverySeed();
  ledger_->SetState(kRecoverySeed, base::Base64Encode(seed));

  ledger_->SetState(kPaymentId, legacy_state_->GetPaymentId());

  ledger_->SetState(kInlineTipRedditEnabled,
                    legacy_state_->GetInlineTipSetting("reddit"));

  ledger_->SetState(kInlineTipTwitterEnabled,
                    legacy_state_->GetInlineTipSetting("twitter"));

  ledger_->SetState(kInlineTipGithubEnabled,
                    legacy_state_->GetInlineTipSetting("github"));

  callback(mojom::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
