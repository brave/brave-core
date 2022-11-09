/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_create.h"

#include <utility>

#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/endpoints/patch_wallets/patch_wallets.h"
#include "bat/ledger/internal/endpoints/post_wallets/post_wallets.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

using ledger::endpoints::PatchWallets;
using ledger::endpoints::PostWallets;
using ledger::endpoints::RequestFor;

namespace ledger::wallet {

namespace {

mojom::CreateRewardsWalletResult MapEndpointError(PostWallets::Error error) {
  switch (error) {
    case PostWallets::Error::kWalletGenerationDisabled:
      return mojom::CreateRewardsWalletResult::kWalletGenerationDisabled;
    default:
      return mojom::CreateRewardsWalletResult::kUnexpected;
  }
}

mojom::CreateRewardsWalletResult MapEndpointError(PatchWallets::Error error) {
  switch (error) {
    case PatchWallets::Error::kGeoCountryAlreadyDeclared:
      return mojom::CreateRewardsWalletResult::kGeoCountryAlreadyDeclared;
    default:
      return mojom::CreateRewardsWalletResult::kUnexpected;
  }
}

}  // namespace

WalletCreate::WalletCreate(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

void WalletCreate::CreateWallet(absl::optional<std::string>&& geo_country,
                                CreateRewardsWalletCallback callback) {
  bool corrupted = false;
  auto wallet = ledger_->wallet()->GetWallet(&corrupted);

  if (corrupted) {
    DCHECK(!wallet);
    BLOG(0, "Rewards wallet data is corrupted - generating a new wallet!");
    ledger_->database()->SaveEventLog(log::kWalletCorrupted, "");
  }

  if (!wallet) {
    wallet = mojom::RewardsWallet::New();
    wallet->recovery_seed = util::Security::GenerateSeed();

    if (!ledger_->wallet()->SetWallet(std::move(wallet))) {
      BLOG(0, "Failed to set Rewards wallet!");
      return std::move(callback).Run(
          mojom::CreateRewardsWalletResult::kUnexpected);
    }
  } else if (!wallet->payment_id.empty()) {
    if (geo_country) {
      DCHECK(!geo_country->empty());
      auto on_update_wallet = base::BindOnce(
          &WalletCreate::OnResult<PatchWallets::Result>, base::Unretained(this),
          std::move(callback), geo_country);

      return RequestFor<PatchWallets>(ledger_, std::move(*geo_country))
          .Send(std::move(on_update_wallet));
    } else {
      BLOG(1, "Rewards wallet already exists.");
      return std::move(callback).Run(
          mojom::CreateRewardsWalletResult::kSuccess);
    }
  }

  auto on_create_wallet =
      base::BindOnce(&WalletCreate::OnResult<PostWallets::Result>,
                     base::Unretained(this), std::move(callback), geo_country);

  RequestFor<PostWallets>(ledger_, std::move(geo_country))
      .Send(std::move(on_create_wallet));
}

template <typename>
inline constexpr bool dependent_false_v = false;

template <typename Result>
void WalletCreate::OnResult(CreateRewardsWalletCallback callback,
                            absl::optional<std::string>&& geo_country,
                            Result&& result) {
  if (!result.has_value()) {
    if constexpr (std::is_same_v<Result, PostWallets::Result>) {
      BLOG(0, "Failed to create Rewards wallet!");
    } else if constexpr (std::is_same_v<Result, PatchWallets::Result>) {
      BLOG(0, "Failed to update Rewards wallet!");
    } else {
      static_assert(dependent_false_v<Result>,
                    "Result must be either "
                    "PostWallets::Result, or "
                    "PatchWallets::Result!");
    }

    return std::move(callback).Run(MapEndpointError(result.error()));
  }

  auto wallet = ledger_->wallet()->GetWallet();
  DCHECK(wallet);
  if constexpr (std::is_same_v<Result, PostWallets::Result>) {
    DCHECK(!result.value().empty());
    wallet->payment_id = std::move(result.value());
  }

  if (!ledger_->wallet()->SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to set Rewards wallet!");
    return std::move(callback).Run(
        mojom::CreateRewardsWalletResult::kUnexpected);
  }

  if constexpr (std::is_same_v<Result, PostWallets::Result>) {
    ledger_->state()->ResetReconcileStamp();
    if (!ledger::is_testing) {
      ledger_->state()->SetEmptyBalanceChecked(true);
      ledger_->state()->SetPromotionCorruptedMigrated(true);
    }
    ledger_->state()->SetCreationStamp(util::GetCurrentTimeStamp());
  }

  std::move(callback).Run(mojom::CreateRewardsWalletResult::kSuccess);
}

}  // namespace ledger::wallet
