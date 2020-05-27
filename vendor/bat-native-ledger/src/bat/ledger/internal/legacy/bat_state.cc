/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <utility>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/legacy/bat_state.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/client_state.h"

using std::placeholders::_1;

namespace braveledger_bat_state {

LegacyBatState::LegacyBatState(bat_ledger::LedgerImpl* ledger) :
      ledger_(ledger),
      state_(new ledger::ClientProperties()) {
}

LegacyBatState::~LegacyBatState() {
}

bool LegacyBatState::LoadState(const std::string& data) {
  ledger::ClientProperties state;
  const ledger::ClientState client_state;
  if (!client_state.FromJson(data, &state)) {
    BLOG(0, "Failed to load client state: " << data);
    return false;
  }

  state_.reset(new ledger::ClientProperties(state));

  bool stateChanged = false;

  // fix timestamp ms to s conversion
  if (std::to_string(state_->reconcile_timestamp).length() > 10) {
    state_->reconcile_timestamp = state_->reconcile_timestamp / 1000;
    stateChanged = true;
  }

  // fix timestamp ms to s conversion
  if (std::to_string(state_->boot_timestamp).length() > 10) {
    state_->boot_timestamp = state_->boot_timestamp / 1000;
    stateChanged = true;
  }

  if (stateChanged) {
    SaveState();
  }

  return true;
}

void LegacyBatState::SaveState() {
  const ledger::ClientState client_state;
  const std::string data = client_state.ToJson(*state_);

  auto save_callback = std::bind(&LegacyBatState::OnSaveState,
      this,
      _1);

  ledger_->SaveLedgerState(data, save_callback);
}

void LegacyBatState::OnSaveState(const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Ledger state was not save successfully");
    return;
  }
}

void LegacyBatState::SetRewardsMainEnabled(bool enabled) {
  state_->rewards_enabled = enabled;
  SaveState();
}

bool LegacyBatState::GetRewardsMainEnabled() const {
  return state_->rewards_enabled;
}

void LegacyBatState::SetContributionAmount(double amount) {
  ledger::WalletProperties properties = GetWalletProperties();
  auto hasAmount = std::find(properties.parameters_choices.begin(),
                             properties.parameters_choices.end(),
                             amount);

  if (hasAmount == properties.parameters_choices.end()) {
    // amount is missing in the list
    properties.parameters_choices.push_back(amount);
    std::sort(properties.parameters_choices.begin(),
              properties.parameters_choices.end());
    ledger_->OnWalletProperties(ledger::Result::LEDGER_OK, properties);
    state_->wallet = properties;
  }

  state_->fee_amount = amount;
  SaveState();
}

double LegacyBatState::GetContributionAmount() const {
  return state_->fee_amount;
}

void LegacyBatState::SetUserChangedContribution() {
  state_->user_changed_fee = true;
  SaveState();
}

bool LegacyBatState::GetUserChangedContribution() const {
  return state_->user_changed_fee;
}

void LegacyBatState::SetAutoContribute(bool enabled) {
  state_->auto_contribute = enabled;
  SaveState();
}

bool LegacyBatState::GetAutoContribute() const {
  return state_->auto_contribute;
}

const std::string& LegacyBatState::GetCardIdAddress() const {
  return state_->wallet_info.address_card_id;
}

uint64_t LegacyBatState::GetReconcileStamp() const {
  return state_->reconcile_timestamp;
}

void LegacyBatState::ResetReconcileStamp() {
  if (ledger::reconcile_time > 0) {
    state_->reconcile_timestamp =
        braveledger_time_util::GetCurrentTimeStamp() +
        ledger::reconcile_time * 60;
  } else {
    state_->reconcile_timestamp = braveledger_time_util::GetCurrentTimeStamp() +
        braveledger_ledger::_reconcile_default_interval;
  }
  SaveState();
}

bool LegacyBatState::IsWalletCreated() const {
  return state_->boot_timestamp != 0u;
}

const std::string& LegacyBatState::GetPaymentId() const {
  return state_->wallet_info.payment_id;
}

const ledger::WalletInfoProperties& LegacyBatState::GetWalletInfo() const {
  return state_->wallet_info;
}

void LegacyBatState::SetWalletInfo(
    const ledger::WalletInfoProperties& wallet_info) {
  state_->wallet_info = wallet_info;
  SaveState();
}

const ledger::WalletProperties& LegacyBatState::GetWalletProperties() const {
  return state_->wallet;
}

void LegacyBatState::SetWalletProperties(
    ledger::WalletProperties* properties) {
  double amount = GetContributionAmount();
  double new_amount = properties->fee_amount;
  bool amount_changed = GetUserChangedContribution();
  if (amount_changed) {
    auto hasAmount = std::find(properties->parameters_choices.begin(),
                               properties->parameters_choices.end(),
                               amount);

    if (hasAmount == properties->parameters_choices.end()) {
      // amount is missing in the list
      properties->parameters_choices.push_back(amount);
      std::sort(properties->parameters_choices.begin(),
                properties->parameters_choices.end());
    }
  }

  state_->wallet = *properties;

  if (!amount_changed && amount != new_amount) {
    SetContributionAmount(new_amount);
  }

  SaveState();
}

uint64_t LegacyBatState::GetBootStamp() const {
  return state_->boot_timestamp;
}

void LegacyBatState::SetBootStamp(uint64_t stamp) {
  state_->boot_timestamp = stamp;
  SaveState();
}

double LegacyBatState::GetDefaultContributionAmount() {
  return state_->wallet.fee_amount;
}

void LegacyBatState::SetInlineTipSetting(const std::string& key, bool enabled) {
  state_->inline_tips[key] = enabled;
  SaveState();
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
