/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_migration_v2.h"

using std::placeholders::_1;

namespace ledger {
namespace state {

StateMigrationV2::StateMigrationV2(LedgerImpl* ledger) :
    ledger_(ledger) {
}

StateMigrationV2::~StateMigrationV2() = default;

void StateMigrationV2::Migrate(ledger::ResultCallback callback) {
  legacy_state_ =
      std::make_unique<braveledger_bat_state::LegacyBatState>(ledger_);

  auto load_callback = std::bind(&StateMigrationV2::OnLoadState,
      this,
      _1,
      callback);

  legacy_state_->Load(load_callback);
}

void StateMigrationV2::OnLoadState(
    const type::Result result,
    ledger::ResultCallback callback) {
  if (result == type::Result::NO_LEDGER_STATE) {
    BLOG(1, "No ledger state");
    callback(type::Result::LEDGER_OK);
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to load ledger state file, setting default values");
    callback(type::Result::LEDGER_OK);
    return;
  }

  ledger_->ledger_client()->SetBooleanState(
      "enabled",
      legacy_state_->GetRewardsMainEnabled());

  ledger_->ledger_client()->SetBooleanState(
      kAutoContributeEnabled,
      legacy_state_->GetAutoContributeEnabled());

  if (legacy_state_->GetUserChangedContribution()) {
    ledger_->ledger_client()->SetDoubleState(
      kAutoContributeAmount,
      legacy_state_->GetAutoContributionAmount());
  }

  ledger_->ledger_client()->SetUint64State(
      kNextReconcileStamp,
      legacy_state_->GetReconcileStamp());

  ledger_->ledger_client()->SetUint64State(
      kCreationStamp,
      legacy_state_->GetCreationStamp());

  const auto seed = legacy_state_->GetRecoverySeed();
  ledger_->ledger_client()->SetStringState(
      kRecoverySeed,
      base::Base64Encode(seed));

  ledger_->ledger_client()->SetStringState(
      kPaymentId,
      legacy_state_->GetPaymentId());

  ledger_->ledger_client()->SetBooleanState(
      kInlineTipRedditEnabled,
      legacy_state_->GetInlineTipSetting("reddit"));

  ledger_->ledger_client()->SetBooleanState(
      kInlineTipTwitterEnabled,
      legacy_state_->GetInlineTipSetting("twitter"));

  ledger_->ledger_client()->SetBooleanState(
      kInlineTipGithubEnabled,
      legacy_state_->GetInlineTipSetting("github"));

  callback(type::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
