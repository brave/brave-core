/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/uphold/uphold_wallet.h"
#include "bat/ledger/internal/state_keys.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

UpholdWallet::UpholdWallet(bat_ledger::LedgerImpl* ledger, Uphold* uphold) :
    ledger_(ledger),
    uphold_(uphold) {
}

UpholdWallet::~UpholdWallet() {
}

void UpholdWallet::Generate(
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletCallback callback) {
  ledger::ExternalWalletPtr wallet;
  if (wallets.size() == 0) {
    wallet = ledger::ExternalWallet::New();
    wallet->status = ledger::WalletStatus::NOT_CONNECTED;
  } else {
    wallet = GetWallet(std::move(wallets));

    if (!wallet) {
      wallet = ledger::ExternalWallet::New();
      wallet->status = ledger::WalletStatus::NOT_CONNECTED;
    }
  }

  wallet->type = ledger::kWalletUphold;

  if (wallet->one_time_string.empty()) {
    wallet->one_time_string = GenerateRandomString(ledger::is_testing);
  }

  if (wallet->token.empty() &&
      (wallet->status == ledger::WalletStatus::PENDING ||
       wallet->status == ledger::WalletStatus::CONNECTED)) {
    wallet->status = ledger::WalletStatus::NOT_CONNECTED;
  }

  wallet = GenerateLinks(std::move(wallet));

  if (wallet->status == ledger::WalletStatus::CONNECTED ||
      wallet->status == ledger::WalletStatus::VERIFIED ||
      wallet->status == ledger::WalletStatus::PENDING) {
    const auto user_callback = std::bind(&UpholdWallet::OnGenerate,
                                         this,
                                         _1,
                                         _2,
                                         *wallet,
                                         callback);
    uphold_->GetUser(std::move(wallet), user_callback);
    return;
  }

  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet->Clone());
  callback(ledger::Result::LEDGER_OK, std::move(wallet));
}

void UpholdWallet::OnGenerate(
    const ledger::Result result,
    const User& user,
    const ledger::ExternalWallet& wallet,
    ledger::ExternalWalletCallback callback) {
  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  if (result == ledger::Result::EXPIRED_TOKEN) {
    uphold_->DisconnectWallet();
    callback(result, std::move(wallet_ptr));
    return;
  }

  if (user.bat_not_allowed) {
    callback(ledger::Result::BAT_NOT_ALLOWED, std::move(wallet_ptr));
    return;
  }

  if (!wallet_ptr || result != ledger::Result::LEDGER_OK) {
    callback(result, std::move(wallet_ptr));
    return;
  }

  wallet_ptr->user_name = user.name;

  if (user.status != UserStatus::OK) {
    wallet_ptr->status = ledger::WalletStatus::PENDING;
  }

  wallet_ptr = SetStatus(user, std::move(wallet_ptr));

  if (wallet_ptr->address.empty()) {
    auto card_callback = std::bind(
      &UpholdWallet::OnCreateCard,
      this,
      *wallet_ptr,
      callback,
      _1,
      _2);
    uphold_->CreateCard(std::move(wallet_ptr), card_callback);
    return;
  }

  if (user.verified) {
    ledger_->TransferTokens(wallet_ptr->Clone(), [](const ledger::Result){});
    uphold_->TransferAnonToExternalWallet(std::move(wallet_ptr), callback);
    return;
  }

  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet_ptr->Clone());
  callback(ledger::Result::LEDGER_OK, std::move(wallet_ptr));
}

void UpholdWallet::OnCreateCard(
    const ledger::ExternalWallet& wallet,
    ledger::ExternalWalletCallback callback,
    const ledger::Result result,
    const std::string& address) {
  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  if (result != ledger::Result::LEDGER_OK) {
    callback(result, std::move(wallet_ptr));
    return;
  }

  wallet_ptr->address = address;
  wallet_ptr = GenerateLinks(std::move(wallet_ptr));

  if (wallet_ptr->status == ledger::WalletStatus::VERIFIED) {
    ledger_->TransferTokens(wallet_ptr->Clone(), [](const ledger::Result){});
    uphold_->TransferAnonToExternalWallet(std::move(wallet_ptr), callback);
    return;
  }

  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet_ptr->Clone());
  callback(ledger::Result::LEDGER_OK, std::move(wallet_ptr));
}

ledger::ExternalWalletPtr UpholdWallet::SetStatus(
    const User& user,
    ledger::ExternalWalletPtr wallet) {
  switch (static_cast<int>(wallet->status)) {
    case static_cast<int>(ledger::WalletStatus::CONNECTED): {
      if (!user.verified) {
        break;
      }

      wallet->status = ledger::WalletStatus::VERIFIED;

      ledger_->ShowNotification(
         "wallet_new_verified",
         [](ledger::Result _){},
         {"Uphold"});
      break;
    }
    case static_cast<int>(ledger::WalletStatus::VERIFIED): {
      if (user.verified) {
        break;
      }

      wallet->status = ledger::WalletStatus::CONNECTED;
      break;
    }
    case static_cast<int>(ledger::WalletStatus::PENDING): {
      if (user.status != UserStatus::OK) {
        break;
      }

      wallet->status = ledger::WalletStatus::CONNECTED;
      break;
    }
  }

  return wallet;
}

}  // namespace braveledger_uphold
