/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/get_external_wallet.h"

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

namespace ledger::wallet_provider {

GetExternalWallet::GetExternalWallet(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

GetExternalWallet::~GetExternalWallet() = default;

void GetExternalWallet::Run(ledger::GetExternalWalletCallback callback) const {
  auto wallet = ledger::wallet::MaybeCreateWallet(ledger_, WalletType());
  if (!wallet) {
    return std::move(callback).Run(
        base::unexpected(mojom::GetExternalWalletError::kUnexpected));
  }

  if (wallet->status == mojom::WalletStatus::kConnected ||
      wallet->status == mojom::WalletStatus::kLoggedOut) {
    return ledger_->promotion()->TransferTokens(
        base::BindOnce(&GetExternalWallet::OnTransferTokens,
                       base::Unretained(this), std::move(callback)));
  }

  std::move(callback).Run(std::move(wallet));
}

void GetExternalWallet::OnTransferTokens(
    ledger::GetExternalWalletCallback callback,
    mojom::Result result,
    std::string) const {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to transfer tokens!");
  }

  auto wallet = ledger::wallet::GetWallet(ledger_, WalletType());
  if (!wallet) {
    return std::move(callback).Run(
        base::unexpected(mojom::GetExternalWalletError::kUnexpected));
  }

  std::move(callback).Run(std::move(wallet));
}

}  // namespace ledger::wallet_provider
