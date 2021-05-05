/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <utility>

#include "bat/ledger/internal/legacy/bat_helper.h"
#include "bat/ledger/internal/legacy/bat_state.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/client_state.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_bat_state {

LegacyBatState::LegacyBatState(ledger::LedgerImpl* ledger) :
      ledger_(ledger),
      state_(new ledger::ClientProperties()) {
}

LegacyBatState::~LegacyBatState() = default;

void LegacyBatState::Load(ledger::ResultCallback callback) {
  auto load_callback = std::bind(&LegacyBatState::OnLoad,
      this,
      _1,
      _2,
      callback);
  ledger_->ledger_client()->LoadLedgerState(load_callback);
}

void LegacyBatState::OnLoad(
      const ledger::type::Result result,
      const std::string& data,
      ledger::ResultCallback callback) {
  if (result != ledger::type::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  ledger::ClientProperties state;
  const ledger::ClientState client_state;
  if (!client_state.FromJson(data, &state)) {
    BLOG(0, "Failed to load client state");
    BLOG(6, "Client state contents: " << data);
    callback(ledger::type::Result::LEDGER_ERROR);
    return;
  }

  state_.reset(new ledger::ClientProperties(state));

  // fix timestamp ms to s conversion
  if (std::to_string(state_->reconcile_timestamp).length() > 10) {
    state_->reconcile_timestamp = state_->reconcile_timestamp / 1000;
  }

  // fix timestamp ms to s conversion
  if (std::to_string(state_->boot_timestamp).length() > 10) {
    state_->boot_timestamp = state_->boot_timestamp / 1000;
  }

  callback(ledger::type::Result::LEDGER_OK);
}

bool LegacyBatState::GetRewardsMainEnabled() const {
  return state_->rewards_enabled;
}

double LegacyBatState::GetAutoContributionAmount() const {
  return state_->fee_amount;
}

bool LegacyBatState::GetUserChangedContribution() const {
  return state_->user_changed_fee;
}

bool LegacyBatState::GetAutoContributeEnabled() const {
  return state_->auto_contribute;
}

const std::string& LegacyBatState::GetCardIdAddress() const {
  return state_->wallet_info.address_card_id;
}

uint64_t LegacyBatState::GetReconcileStamp() const {
  return state_->reconcile_timestamp;
}

const std::string& LegacyBatState::GetPaymentId() const {
  return state_->wallet_info.payment_id;
}

const std::vector<uint8_t>& LegacyBatState::GetRecoverySeed() const {
  return state_->wallet_info.key_info_seed;
}

uint64_t LegacyBatState::GetCreationStamp() const {
  return state_->boot_timestamp;
}

bool LegacyBatState::GetInlineTipSetting(const std::string& key) const {
  if (state_->inline_tips.find(key) == state_->inline_tips.end()) {
    // not found, all tips are on by default
    return true;
  } else {
    return state_->inline_tips[key];
  }
}

}  // namespace braveledger_bat_state
