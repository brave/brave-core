/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/get_external_wallet.h"

#include <utility>

#include "brave/components/brave_rewards/core/promotion/promotion.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal::wallet_provider {

GetExternalWallet::GetExternalWallet(RewardsEngineImpl& engine)
    : engine_(engine) {}

GetExternalWallet::~GetExternalWallet() = default;

void GetExternalWallet::Run(GetExternalWalletCallback callback) const {
  auto wallet = wallet::MaybeCreateWallet(*engine_, WalletType());
  if (!wallet) {
    return std::move(callback).Run(
        base::unexpected(mojom::GetExternalWalletError::kUnexpected));
  }

  if (wallet->status == mojom::WalletStatus::kConnected ||
      wallet->status == mojom::WalletStatus::kLoggedOut) {
    return engine_->promotion()->TransferTokens(
        base::BindOnce(&GetExternalWallet::OnTransferTokens,
                       base::Unretained(this), std::move(callback)));
  }

  std::move(callback).Run(std::move(wallet));
}

void GetExternalWallet::OnTransferTokens(GetExternalWalletCallback callback,
                                         mojom::Result result,
                                         std::string) const {
  if (result != mojom::Result::OK) {
    BLOG(0, "Failed to transfer tokens!");
  }

  auto wallet = wallet::GetWallet(*engine_, WalletType());
  if (!wallet) {
    return std::move(callback).Run(
        base::unexpected(mojom::GetExternalWalletError::kUnexpected));
  }

  std::move(callback).Run(std::move(wallet));
}

}  // namespace brave_rewards::internal::wallet_provider
