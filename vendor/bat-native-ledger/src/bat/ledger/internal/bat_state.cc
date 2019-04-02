/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <utility>

#include "bat/ledger/internal/bat_state.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"

namespace braveledger_bat_state {

BatState::BatState(bat_ledger::LedgerImpl* ledger) :
      ledger_(ledger),
      state_(new braveledger_bat_helper::CLIENT_STATE_ST()) {
}

BatState::~BatState() {
}

bool BatState::LoadState(const std::string& data) {
  braveledger_bat_helper::CLIENT_STATE_ST state;
  if (!braveledger_bat_helper::loadFromJson(&state, data.c_str())) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Failed to load client state: " << data;
    return false;
  }

  state_.reset(new braveledger_bat_helper::CLIENT_STATE_ST(state));

  bool stateChanged = false;

  // fix timestamp ms to s conversion
  if (std::to_string(state_->reconcileStamp_).length() > 10) {
    state_->reconcileStamp_ = state_->reconcileStamp_ / 1000;
    stateChanged = true;
  }

  // fix timestamp ms to s conversion
  if (std::to_string(state_->bootStamp_).length() > 10) {
    state_->bootStamp_ = state_->bootStamp_ / 1000;
    stateChanged = true;
  }

  if (stateChanged) {
    SaveState();
  }

  return true;
}

void BatState::SaveState() {
  std::string data;
  braveledger_bat_helper::saveToJsonString(*state_, &data);
  ledger_->SaveLedgerState(data);
}

void BatState::AddReconcile(const std::string& viewing_id,
      const braveledger_bat_helper::CURRENT_RECONCILE& reconcile) {
  state_->current_reconciles_.insert(std::make_pair(viewing_id, reconcile));
  SaveState();
}

bool BatState::UpdateReconcile(
    const braveledger_bat_helper::CURRENT_RECONCILE& reconcile) {
  if (state_->current_reconciles_.count(reconcile.viewingId_) == 0) {
    return false;
  }

  state_->current_reconciles_[reconcile.viewingId_] = reconcile;
  SaveState();
  return true;
}

braveledger_bat_helper::CURRENT_RECONCILE BatState::GetReconcileById(
    const std::string& viewingId) const {
  if (state_->current_reconciles_.count(viewingId) == 0) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Could not find any reconcile tasks with the id " << viewingId;
    return braveledger_bat_helper::CURRENT_RECONCILE();
  }

  return state_->current_reconciles_[viewingId];
}

bool BatState::ReconcileExists(const std::string& viewingId) const {
  return state_->current_reconciles_.count(viewingId) > 0;
}

void BatState::RemoveReconcileById(const std::string& viewingId) {
  braveledger_bat_helper::CurrentReconciles::iterator it =
      state_->current_reconciles_.find(viewingId);
  if (it != state_->current_reconciles_.end()) {
    state_->current_reconciles_.erase(it);
    SaveState();
  }
}

void BatState::SetRewardsMainEnabled(bool enabled) {
  state_->rewards_enabled_ = enabled;
  SaveState();
}

bool BatState::GetRewardsMainEnabled() const {
  return state_->rewards_enabled_;
}

void BatState::SetContributionAmount(double amount) {
  braveledger_bat_helper::WALLET_PROPERTIES_ST properties =
      GetWalletProperties();
  auto hasAmount = std::find(properties.parameters_choices_.begin(),
                             properties.parameters_choices_.end(),
                             amount);

  if (hasAmount == properties.parameters_choices_.end()) {
    // amount is missing in the list
    properties.parameters_choices_.push_back(amount);
    std::sort(properties.parameters_choices_.begin(),
              properties.parameters_choices_.end());
    ledger_->OnWalletProperties(ledger::Result::LEDGER_OK, properties);
    state_->walletProperties_ = properties;
  }

  state_->fee_amount_ = amount;
  SaveState();
}

double BatState::GetContributionAmount() const {
  return state_->fee_amount_;
}

void BatState::SetUserChangedContribution() {
  state_->user_changed_fee_ = true;
  SaveState();
}

bool BatState::GetUserChangedContribution() const {
  return state_->user_changed_fee_;
}

void BatState::SetAutoContribute(bool enabled) {
  state_->auto_contribute_ = enabled;
  SaveState();
}

bool BatState::GetAutoContribute() const {
  return state_->auto_contribute_;
}

const std::string& BatState::GetBATAddress() const {
  return state_->walletInfo_.addressBAT_;
}

const std::string& BatState::GetBTCAddress() const {
  return state_->walletInfo_.addressBTC_;
}

const std::string& BatState::GetETHAddress() const {
  return state_->walletInfo_.addressETH_;
}

const std::string& BatState::GetLTCAddress() const {
  return state_->walletInfo_.addressLTC_;
}

uint64_t BatState::GetReconcileStamp() const {
  return state_->reconcileStamp_;
}

void BatState::ResetReconcileStamp() {
  if (ledger::reconcile_time > 0) {
    state_->reconcileStamp_ = braveledger_bat_helper::currentTime() +
                                ledger::reconcile_time * 60;
  } else {
    state_->reconcileStamp_ = braveledger_bat_helper::currentTime() +
                                braveledger_ledger::_reconcile_default_interval;
  }
  SaveState();
}

uint64_t BatState::GetLastGrantLoadTimestamp() const {
  return state_->last_grant_fetch_stamp_;
}

void BatState::SetLastGrantLoadTimestamp(uint64_t stamp) {
  state_->last_grant_fetch_stamp_ = stamp;
  SaveState();
}

bool BatState::IsWalletCreated() const {
  return state_->bootStamp_ != 0u;
}

double BatState::GetBalance() const {
  return state_->walletProperties_.balance_;
}

const std::string& BatState::GetPaymentId() const {
  return state_->walletInfo_.paymentId_;
}

void BatState::SetPaymentId(const std::string& payment_id) {
  state_->walletInfo_.paymentId_ = payment_id;
  SaveState();
}

const braveledger_bat_helper::Grants& BatState::GetGrants() const {
  return state_->grants_;
}

void BatState::SetGrants(braveledger_bat_helper::Grants grants) {
  state_->grants_ = grants;
  SaveState();
}

const std::string& BatState::GetPersonaId() const {
  return state_->personaId_;
}

void BatState::SetPersonaId(const std::string& persona_id) {
  state_->personaId_ = persona_id;
  SaveState();
}

const std::string& BatState::GetUserId() const {
  return state_->userId_;
}

void BatState::SetUserId(const std::string& user_id) {
  state_->userId_ = user_id;
  SaveState();
}

const std::string& BatState::GetRegistrarVK() const {
  return state_->registrarVK_;
}

void BatState::SetRegistrarVK(const std::string& registrar_vk) {
  state_->registrarVK_ = registrar_vk;
  SaveState();
}

const std::string& BatState::GetPreFlight() const {
  return state_->preFlight_;
}

void BatState::SetPreFlight(const std::string& pre_flight) {
  state_->preFlight_ = pre_flight;
  SaveState();
}

const braveledger_bat_helper::WALLET_INFO_ST& BatState::GetWalletInfo() const {
  return state_->walletInfo_;
}

void BatState::SetWalletInfo(
    const braveledger_bat_helper::WALLET_INFO_ST& wallet_info) {
  state_->walletInfo_ = wallet_info;
  SaveState();
}

const braveledger_bat_helper::WALLET_PROPERTIES_ST&
BatState::GetWalletProperties() const {
  return state_->walletProperties_;
}

void BatState::SetWalletProperties(
    braveledger_bat_helper::WALLET_PROPERTIES_ST* properties) {
  double amount = GetContributionAmount();
  double new_amount = properties->fee_amount_;
  bool amount_changed = GetUserChangedContribution();
  if (amount_changed) {
    auto hasAmount = std::find(properties->parameters_choices_.begin(),
                               properties->parameters_choices_.end(),
                               amount);

    if (hasAmount == properties->parameters_choices_.end()) {
      // amount is missing in the list
      properties->parameters_choices_.push_back(amount);
      std::sort(properties->parameters_choices_.begin(),
                properties->parameters_choices_.end());
    }
  }

  state_->walletProperties_ = *properties;

  if (!amount_changed && amount != new_amount) {
    SetContributionAmount(new_amount);
  }

  SaveState();
}

unsigned int BatState::GetDays() const {
  return state_->days_;
}

void BatState::SetDays(unsigned int days) {
  state_->days_ = days;
  SaveState();
}

const braveledger_bat_helper::Transactions& BatState::GetTransactions() const {
  return state_->transactions_;
}

void BatState::SetTransactions(
    const braveledger_bat_helper::Transactions& transactions) {
  state_->transactions_ = transactions;
  SaveState();
}

const braveledger_bat_helper::Ballots& BatState::GetBallots() const {
  return state_->ballots_;
}

void BatState::SetBallots(const braveledger_bat_helper::Ballots& ballots) {
  state_->ballots_ = ballots;
  SaveState();
}

const braveledger_bat_helper::BatchVotes& BatState::GetBatch() const {
  return state_->batch_;
}

void BatState::SetBatch(const braveledger_bat_helper::BatchVotes& votes) {
  state_->batch_ = votes;
  SaveState();
}

const std::string& BatState::GetCurrency() const {
  return state_->fee_currency_;
}

void BatState::SetCurrency(const std::string &currency) {
  state_->fee_currency_ = currency;
  SaveState();
}

uint64_t BatState::GetBootStamp() const {
  return state_->bootStamp_;
}

void BatState::SetBootStamp(uint64_t stamp) {
  state_->bootStamp_ = stamp;
  SaveState();
}

const std::string& BatState::GetMasterUserToken() const {
  return state_->masterUserToken_;
}

void BatState::SetMasterUserToken(const std::string &token) {
  state_->masterUserToken_ = token;
  SaveState();
}

bool BatState::AddReconcileStep(const std::string& viewing_id,
                                ledger::ContributionRetry step,
                                int level) {
  braveledger_bat_helper::CURRENT_RECONCILE reconcile =
      GetReconcileById(viewing_id);

  if (reconcile.viewingId_.empty()) {
    return false;
  }

  // don't save step when you are already in the same step
  if (reconcile.retry_step_ == step && level == -1) {
    return true;
  }

  reconcile.retry_step_ = step;
  reconcile.retry_level_ = level;

  return UpdateReconcile(reconcile);
}

const braveledger_bat_helper::CurrentReconciles&
BatState::GetCurrentReconciles() const {
  return state_->current_reconciles_;
}

double BatState::GetDefaultContributionAmount() {
  return state_->walletProperties_.fee_amount_;
}

void BatState::SetAddress(std::map<std::string, std::string> addresses) {
  for (auto& address : addresses) {
    if (address.first == "BAT") {
      state_->walletInfo_.addressBAT_ = address.second;
    }

    if (address.first == "BTC") {
      state_->walletInfo_.addressBTC_ = address.second;
    }

    if (address.first == "CARD_ID") {
      state_->walletInfo_.addressCARD_ID_ = address.second;
    }

    if (address.first == "ETH") {
      state_->walletInfo_.addressETH_ = address.second;
    }

    if (address.first == "LTC") {
      state_->walletInfo_.addressLTC_ = address.second;
    }
  }
  SaveState();
}

void BatState::ResetState() {
  state_.reset(new braveledger_bat_helper::CLIENT_STATE_ST());
}

}  // namespace braveledger_bat_state
