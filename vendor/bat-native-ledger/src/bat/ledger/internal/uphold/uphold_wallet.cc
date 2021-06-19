/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_wallet.h"

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace uphold {

UpholdWallet::UpholdWallet(LedgerImpl* ledger)
    : ledger_{ledger},
      promotion_server_{std::make_unique<endpoint::PromotionServer>(ledger)} {}

UpholdWallet::~UpholdWallet() = default;

void UpholdWallet::Generate(ledger::ResultCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    uphold_wallet = type::ExternalWallet::New();
    uphold_wallet->type = constant::kWalletUphold;
    uphold_wallet->status = type::WalletStatus::NOT_CONNECTED;
    uphold_wallet = GenerateLinks(std::move(uphold_wallet));
  }

  if (uphold_wallet->one_time_string.empty()) {
    uphold_wallet->one_time_string = util::GenerateRandomHexString();
  }

  const auto status = uphold_wallet->status;
  if (!ledger_->uphold()->SetWallet(std::move(uphold_wallet))) {
    BLOG(0, "Unable to set the Uphold wallet!");
    return callback(type::Result::LEDGER_ERROR);
  }

  if (status != type::WalletStatus::PENDING &&
      status != type::WalletStatus::VERIFIED) {
    return callback(type::Result::LEDGER_OK);
  }

  ledger_->uphold()->GetUser(
      std::bind(&UpholdWallet::OnGetUser, this, _1, _2, callback));
}

void UpholdWallet::OnGetUser(const type::Result result,
                             const User& user,
                             ledger::ResultCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "The Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(uphold_wallet->status == type::WalletStatus::PENDING ||
         uphold_wallet->status == type::WalletStatus::VERIFIED);
  DCHECK(!uphold_wallet->token.empty());
  DCHECK(uphold_wallet->status == type::WalletStatus::PENDING
             ? uphold_wallet->address.empty()
             : !uphold_wallet->address.empty());

  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    ledger_->uphold()->DisconnectWallet();
    // status == type::WalletStatus::NOT_CONNECTED ||
    // status == type::WalletStatus::DISCONNECTED_VERIFIED
    return callback(type::Result::EXPIRED_TOKEN);
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get the user object from Uphold!");
    return callback(result);
  }

  if (user.bat_not_allowed) {
    BLOG(0, "BAT is not allowed for the user!");
    ledger_->uphold()->DisconnectWallet();
    // status == type::WalletStatus::NOT_CONNECTED ||
    // status == type::WalletStatus::DISCONNECTED_VERIFIED
    return callback(type::Result::BAT_NOT_ALLOWED);
  }

  uphold_wallet->user_name = user.name;
  if (!ledger_->uphold()->SetWallet(uphold_wallet->Clone())) {
    BLOG(0, "Unable to set the Uphold wallet!");
    return callback(type::Result::LEDGER_ERROR);
  }

  if (user.status != UserStatus::OK || !user.verified) {
    if (uphold_wallet->status == type::WalletStatus::VERIFIED) {
      ledger_->uphold()->DisconnectWallet();
    }

    return callback(type::Result::LEDGER_OK);
  }

  if (uphold_wallet->status == type::WalletStatus::VERIFIED) {
    return ledger_->promotion()->TransferTokens(
        std::bind(&UpholdWallet::OnTransferTokens, this, _1, _2, callback));
  }

  ledger_->uphold()->CreateCard(
      std::bind(&UpholdWallet::OnCreateCard, this, _1, _2, callback));
}

void UpholdWallet::OnCreateCard(const type::Result result,
                                const std::string& address,
                                ledger::ResultCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "The Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(uphold_wallet->status == type::WalletStatus::PENDING);
  DCHECK(!uphold_wallet->token.empty());
  DCHECK(uphold_wallet->address.empty());

  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    ledger_->uphold()->DisconnectWallet();
    // status == type::WalletStatus::NOT_CONNECTED
    // Theoretically, calling DisconnectWallet() could result in
    // DISCONNECTED_VERIFIED, but only in case the status was VERIFIED (which we
    // know it wasn't - we create the card only in PENDING).
    return callback(type::Result::EXPIRED_TOKEN);
  }

  if (result != type::Result::LEDGER_OK) {
    return callback(result);
  }

  if (address.empty()) {
    BLOG(0, "Card address is empty!");
    return callback(type::Result::LEDGER_ERROR);
  }

  GetAnonFunds(std::bind(&UpholdWallet::OnGetAnonFunds, this, _1, _2, address,
                         callback));
}

void UpholdWallet::GetAnonFunds(
    endpoint::promotion::GetWalletBalanceCallback callback) const {
  // if we don't have user funds in anon card anymore
  // we can skip balance server ping
  if (!ledger_->state()->GetFetchOldBalanceEnabled()) {
    return callback(type::Result::LEDGER_OK, type::Balance::New());
  }

  const auto brave_wallet = ledger_->wallet()->GetWallet();
  if (!brave_wallet) {
    BLOG(1, "The Brave wallet is null!");
    ledger_->state()->SetFetchOldBalanceEnabled(false);
    return callback(type::Result::LEDGER_OK, type::Balance::New());
  }

  if (brave_wallet->payment_id.empty()) {
    BLOG(0, "Payment ID is empty!");
    return callback(type::Result::LEDGER_ERROR, nullptr);
  }

  promotion_server_->get_wallet_balance()->Request(callback);
}

void UpholdWallet::OnGetAnonFunds(const type::Result result,
                                  type::BalancePtr balance,
                                  const std::string& address,
                                  ledger::ResultCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "The Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(uphold_wallet->status == type::WalletStatus::PENDING);
  DCHECK(!uphold_wallet->token.empty());
  DCHECK(uphold_wallet->address.empty());
  DCHECK(!address.empty());

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get anonymous funds!");
    return callback(result);
  }

  if (!balance) {
    BLOG(0, "Balance is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  if (balance->user_funds == 0.0) {  // == floating-point comparison!
    ledger_->state()->SetFetchOldBalanceEnabled(false);
  }

  LinkWallet(balance->user_funds, address,
             std::bind(&UpholdWallet::OnLinkWallet, this, _1, _2, callback));
}

void UpholdWallet::LinkWallet(
    const double user_funds,
    const std::string& address,
    ledger::endpoint::promotion::PostClaimUpholdCallback callback) const {
  promotion_server_->post_claim_uphold()->Request(user_funds, address,
                                                  callback);
}

void UpholdWallet::OnLinkWallet(const type::Result result,
                                const std::string& address,
                                ledger::ResultCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "The Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(uphold_wallet->status == type::WalletStatus::PENDING);
  DCHECK(!uphold_wallet->token.empty());
  DCHECK(uphold_wallet->address.empty());
  DCHECK(!address.empty());

  if (result == type::Result::ALREADY_EXISTS) {
    ledger_->database()->SaveEventLog(
        log::kDeviceLimitReached,
        std::string{constant::kWalletUphold} + "/" + address.substr(0, 5));

    ledger_->ledger_client()->ShowNotification("wallet_device_limit_reached",
                                               {}, [](type::Result) {});

    return callback(type::Result::ALREADY_EXISTS);
  }

  if (result != type::Result::LEDGER_OK) {
    return callback(result); // used to be callback(type::Result::CONTINUE);
  }

  uphold_wallet->status = type::WalletStatus::VERIFIED;
  uphold_wallet->address = address;
  uphold_wallet = GenerateLinks(std::move(uphold_wallet));
  ledger_->uphold()->SetWallet(std::move(uphold_wallet));

  ledger_->promotion()->TransferTokens(
      std::bind(&UpholdWallet::OnTransferTokens, this, _1, _2, callback));
}

void UpholdWallet::OnTransferTokens(const type::Result result,
                                    const std::string& drain_id,
                                    ledger::ResultCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "The Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(uphold_wallet->status == type::WalletStatus::VERIFIED);
  DCHECK(!uphold_wallet->token.empty());
  DCHECK(!uphold_wallet->address.empty());

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Transferring tokens failed!");
    return callback(type::Result::CONTINUE);
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace uphold
}  // namespace ledger
