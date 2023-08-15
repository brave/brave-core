/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet/wallet_create.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/security_util.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/endpoints/brave/patch_wallets.h"
#include "brave/components/brave_rewards/core/endpoints/brave/post_wallets.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"

namespace brave_rewards::internal {

using endpoints::PatchWallets;
using endpoints::PostWallets;
using endpoints::RequestFor;

namespace wallet {

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

WalletCreate::WalletCreate(RewardsEngineImpl& engine) : engine_(engine) {}

void WalletCreate::CreateWallet(absl::optional<std::string>&& geo_country,
                                CreateRewardsWalletCallback callback) {
  bool corrupted = false;
  auto wallet = engine_->wallet()->GetWallet(&corrupted);

  if (corrupted) {
    DCHECK(!wallet);
    BLOG(0, "Rewards wallet data is corrupted - generating a new wallet!");
    engine_->database()->SaveEventLog(log::kWalletCorrupted, "");
  }

  if (!wallet) {
    wallet = mojom::RewardsWallet::New();
    wallet->recovery_seed = util::Security::GenerateSeed();

    if (!engine_->wallet()->SetWallet(std::move(wallet))) {
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

      return RequestFor<PatchWallets>(*engine_, std::move(*geo_country))
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

  RequestFor<PostWallets>(*engine_, std::move(geo_country))
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

  auto wallet = engine_->wallet()->GetWallet();
  DCHECK(wallet);
  if constexpr (std::is_same_v<Result, PostWallets::Result>) {
    DCHECK(!result.value().empty());
    wallet->payment_id = std::move(result.value());
  }

  if (!engine_->wallet()->SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to set Rewards wallet!");
    return std::move(callback).Run(
        mojom::CreateRewardsWalletResult::kUnexpected);
  }

  if constexpr (std::is_same_v<Result, PostWallets::Result>) {
    engine_->state()->ResetReconcileStamp();
    if (!is_testing) {
      engine_->state()->SetEmptyBalanceChecked(true);
      engine_->state()->SetPromotionCorruptedMigrated(true);
    }
    engine_->state()->SetCreationStamp(util::GetCurrentTimeStamp());
  }

  std::move(callback).Run(mojom::CreateRewardsWalletResult::kSuccess);
}

}  // namespace wallet

}  // namespace brave_rewards::internal
