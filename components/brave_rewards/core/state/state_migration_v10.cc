/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v10.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"

namespace brave_rewards::internal {
using endpoints::GetWallet;
using endpoints::RequestFor;

namespace state {

StateMigrationV10::StateMigrationV10(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV10::~StateMigrationV10() = default;

// mojom::WalletStatus::CONNECTED (1),
// mojom::WalletStatus::DISCONNECTED_NOT_VERIFIED (3), and
// mojom::WalletStatus::PENDING (5) have been removed.

// mojom::WalletStatus::NOT_CONNECTED (0) has been renamed to
// mojom::WalletStatus::kNotConnected (0),
// mojom::WalletStatus::VERIFIED (2) has been renamed to
// mojom::WalletStatus::kConnected (2), and
// mojom::WalletStatus::DISCONNECTED_VERIFIED (4) has been renamed to
// mojom::WalletStatus::kLoggedOut (4).

void StateMigrationV10::Migrate(ResultCallback callback) {
  auto uphold_wallet = engine_->uphold()->GetWallet();
  if (!uphold_wallet) {
    engine_->Log(FROM_HERE) << "Uphold wallet is null.";
    return std::move(callback).Run(mojom::Result::OK);
  }

  switch (static_cast<std::underlying_type_t<mojom::WalletStatus>>(
      uphold_wallet->status)) {
    case 0:  // mojom::WalletStatus::NOT_CONNECTED
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case 1:  // mojom::WalletStatus::CONNECTED
      uphold_wallet->status = static_cast<mojom::WalletStatus>(
          !uphold_wallet->token.empty()
              ? 5    // mojom::WalletStatus::PENDING
              : 0);  // mojom::WalletStatus::NOT_CONNECTED
      uphold_wallet->address = "";
      break;
    case 2: {  // mojom::WalletStatus::VERIFIED
      if (uphold_wallet->token.empty() || uphold_wallet->address.empty()) {
        uphold_wallet->status = static_cast<mojom::WalletStatus>(
            !uphold_wallet->token.empty()
                ? 5    // mojom::WalletStatus::PENDING
                : 4);  // mojom::WalletStatus::DISCONNECTED_VERIFIED
        uphold_wallet->address = "";
        break;
      }

      auto wallet_info_endpoint_callback =
          base::BindOnce(&StateMigrationV10::OnGetWallet,
                         weak_factory_.GetWeakPtr(), std::move(callback));

      if (engine_->options().is_testing) {
        return std::move(wallet_info_endpoint_callback)
            .Run(
                base::unexpected(mojom::GetWalletError::kUnexpectedStatusCode));
      } else {
        return RequestFor<GetWallet>(*engine_).Send(
            std::move(wallet_info_endpoint_callback));
      }
    }
    case 3:  // mojom::WalletStatus::DISCONNECTED_NOT_VERIFIED
      uphold_wallet->status = static_cast<mojom::WalletStatus>(
          4);  // mojom::WalletStatus::DISCONNECTED_VERIFIED
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case 4:  // mojom::WalletStatus::DISCONNECTED_VERIFIED
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case 5:  // mojom::WalletStatus::PENDING
      uphold_wallet->status = static_cast<mojom::WalletStatus>(
          !uphold_wallet->token.empty()
              ? 5    // mojom::WalletStatus::PENDING
              : 0);  // mojom::WalletStatus::NOT_CONNECTED
      uphold_wallet->address = "";
      break;
    default:
      NOTREACHED_IN_MIGRATION();
  }

  std::move(callback).Run(engine_->uphold()->SetWallet(std::move(uphold_wallet))
                              ? mojom::Result::OK
                              : mojom::Result::FAILED);
}

void StateMigrationV10::OnGetWallet(ResultCallback callback,
                                    endpoints::GetWallet::Result&& result) {
  auto uphold_wallet = engine_->uphold()->GetWallet();
  if (!uphold_wallet) {
    engine_->LogError(FROM_HERE) << "Uphold wallet is null";
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  DCHECK(uphold_wallet->status ==
         static_cast<mojom::WalletStatus>(2));  // mojom::WalletStatus::VERIFIED
  DCHECK(!uphold_wallet->token.empty());
  DCHECK(!uphold_wallet->address.empty());

  const auto is_semi_verified = [](auto& result) {
    return result.wallet_provider != constant::kWalletUphold || !result.linked;
  };

  // deemed semi-VERIFIED || semi-VERIFIED
  if (!result.has_value() || is_semi_verified(result.value())) {
    uphold_wallet->status =
        static_cast<mojom::WalletStatus>(5);  // mojom::WalletStatus::PENDING
    uphold_wallet->address = "";
  }

  std::move(callback).Run(engine_->uphold()->SetWallet(std::move(uphold_wallet))
                              ? mojom::Result::OK
                              : mojom::Result::FAILED);
}

}  // namespace state
}  // namespace brave_rewards::internal
