/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <utility>

#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/legacy/bat_state.h"
#include "brave/components/brave_rewards/core/legacy/client_properties.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace brave_rewards::internal {

LegacyBatState::LegacyBatState(RewardsEngineImpl& engine) : engine_(engine) {}

LegacyBatState::~LegacyBatState() = default;

void LegacyBatState::Load(LegacyResultCallback callback) {
  engine_->client()->LoadLegacyState(base::BindOnce(
      &LegacyBatState::OnLoad, base::Unretained(this), std::move(callback)));
}

void LegacyBatState::OnLoad(LegacyResultCallback callback,
                            mojom::Result result,
                            const std::string& data) {
  if (result != mojom::Result::OK) {
    callback(result);
    return;
  }

  ClientProperties state;
  if (!state.FromJson(data)) {
    engine_->LogError(FROM_HERE) << "Failed to load client state";
    engine_->Log(FROM_HERE) << "Client state contents: " << data;
    callback(mojom::Result::FAILED);
    return;
  }

  state_ = std::move(state);

  // fix timestamp ms to s conversion
  if (std::to_string(state_.reconcile_timestamp).length() > 10) {
    state_.reconcile_timestamp = state_.reconcile_timestamp / 1000;
  }

  // fix timestamp ms to s conversion
  if (std::to_string(state_.boot_timestamp).length() > 10) {
    state_.boot_timestamp = state_.boot_timestamp / 1000;
  }

  callback(mojom::Result::OK);
}

bool LegacyBatState::GetRewardsMainEnabled() const {
  return state_.rewards_enabled;
}

double LegacyBatState::GetAutoContributionAmount() const {
  return state_.fee_amount;
}

bool LegacyBatState::GetUserChangedContribution() const {
  return state_.user_changed_fee;
}

bool LegacyBatState::GetAutoContributeEnabled() const {
  return state_.auto_contribute;
}

const std::string& LegacyBatState::GetCardIdAddress() const {
  return state_.wallet_info.address_card_id;
}

uint64_t LegacyBatState::GetReconcileStamp() const {
  return state_.reconcile_timestamp;
}

const std::string& LegacyBatState::GetPaymentId() const {
  return state_.wallet_info.payment_id;
}

const std::vector<uint8_t>& LegacyBatState::GetRecoverySeed() const {
  return state_.wallet_info.key_info_seed;
}

uint64_t LegacyBatState::GetCreationStamp() const {
  return state_.boot_timestamp;
}

}  // namespace brave_rewards::internal
