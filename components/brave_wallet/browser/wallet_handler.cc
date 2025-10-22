/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/wallet_handler.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

WalletHandler::WalletHandler(
    mojo::PendingReceiver<mojom::WalletHandler> receiver,
    BraveWalletService* wallet_service)
    : receiver_(this, std::move(receiver)),
      brave_wallet_service_(wallet_service) {}

WalletHandler::~WalletHandler() = default;

// TODO(apaymyshev): this is the only method in WalletHandler. Should be merged
// into BraveWalletService.
void WalletHandler::GetWalletInfo(GetWalletInfoCallback callback) {
  if (!brave_wallet_service_) {
    std::move(callback).Run(nullptr);
    return;
  }

  auto* keyring_service = brave_wallet_service_->keyring_service();

  std::move(callback).Run(mojom::WalletInfo::New(
      keyring_service->IsWalletCreatedSync(), keyring_service->IsLockedSync(),
      keyring_service->IsWalletBackedUpSync(), IsBitcoinEnabled(),
      IsBitcoinImportEnabled(), IsBitcoinLedgerEnabled(), IsZCashEnabled(),
      IsAnkrBalancesEnabled(), IsTransactionSimulationsEnabled(),
      IsZCashShieldedTransactionsEnabled(), IsCardanoEnabled(),
      GetEnabledCoins(), IsCardanoDAppSupportEnabled(), IsPolkadotEnabled()));
}

}  // namespace brave_wallet
