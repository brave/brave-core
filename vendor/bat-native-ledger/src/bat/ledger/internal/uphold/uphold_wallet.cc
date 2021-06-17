/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_wallet.h"

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace uphold {

UpholdWallet::UpholdWallet(LedgerImpl* ledger) : ledger_(ledger) {}

UpholdWallet::~UpholdWallet() = default;

void UpholdWallet::Generate(ledger::ResultCallback callback) {
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

  if (status == type::WalletStatus::PENDING ||
      status == type::WalletStatus::VERIFIED) {
    return ledger_->uphold()->GetUser(
        std::bind(&UpholdWallet::OnGetUser, this, _1, _2, callback));
  }

  callback(type::Result::LEDGER_OK);
}

void UpholdWallet::OnGetUser(const type::Result result,
                             const User& user,
                             ledger::ResultCallback callback) {
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

  uphold_wallet->user_name = user.name;
  if (user.status != UserStatus::OK || !user.verified) {
    uphold_wallet->status = type::WalletStatus::PENDING;
    uphold_wallet->address = {};
    uphold_wallet = GenerateLinks(std::move(uphold_wallet));
    if (!ledger_->uphold()->SetWallet(std::move(uphold_wallet))) {
      BLOG(0, "Unable to set the Uphold wallet!");
      return callback(type::Result::LEDGER_ERROR);
    }

    return callback(type::Result::LEDGER_OK);
  }

  if (!ledger_->uphold()->SetWallet(uphold_wallet->Clone())) {
    BLOG(0, "Unable to set the Uphold wallet!");
    return callback(type::Result::LEDGER_ERROR);
  }

  if (uphold_wallet->status == type::WalletStatus::PENDING) {
    return ledger_->uphold()->CreateCard(
        std::bind(&UpholdWallet::OnCreateCard, this, _1, _2, callback));
  }

  callback(type::Result::LEDGER_OK);
}

void UpholdWallet::OnCreateCard(const type::Result result,
                                const std::string& address,
                                ledger::ResultCallback callback) {
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

  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "The Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(uphold_wallet->status == type::WalletStatus::PENDING);
  DCHECK(uphold_wallet->address.empty());
  DCHECK(!uphold_wallet->token.empty());

  ledger_->wallet()->ClaimFunds(address, callback);
}

}  // namespace uphold
}  // namespace ledger
