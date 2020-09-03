/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/wallet/wallet_util.h"

namespace ledger {
namespace wallet {

type::ExternalWalletPtr GetWallet(
    const std::string& wallet_type,
    std::map<std::string, type::ExternalWalletPtr> wallets) {
  for (auto& wallet : wallets) {
    if (wallet.first == wallet_type) {
      return std::move(wallet.second);
    }
  }

  return nullptr;
}

type::ExternalWalletPtr ResetWallet(type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  const auto status = wallet->status;
  wallet = type::ExternalWallet::New();

  if (status != type::WalletStatus::NOT_CONNECTED) {
    if (status == type::WalletStatus::VERIFIED) {
      wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
    } else {
      wallet->status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;
    }
  }

  return wallet;
}

}  // namespace wallet
}  // namespace ledger
