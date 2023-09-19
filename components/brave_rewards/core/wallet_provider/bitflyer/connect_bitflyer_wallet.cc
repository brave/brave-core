/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/connect_bitflyer_wallet.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/endpoint/bitflyer/bitflyer_server.h"
#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_bitflyer.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "crypto/sha2.h"

namespace brave_rewards::internal {

using endpoints::PostConnectBitflyer;
using endpoints::RequestFor;
using wallet_provider::ConnectExternalWallet;

namespace bitflyer {

ConnectBitFlyerWallet::ConnectBitFlyerWallet(RewardsEngineImpl& engine)
    : ConnectExternalWallet(engine), bitflyer_server_(engine) {}

ConnectBitFlyerWallet::~ConnectBitFlyerWallet() = default;

const char* ConnectBitFlyerWallet::WalletType() const {
  return constant::kWalletBitflyer;
}

void ConnectBitFlyerWallet::Authorize(OAuthInfo&& oauth_info,
                                      ConnectExternalWalletCallback callback) {
  DCHECK(!oauth_info.code.empty());
  DCHECK(!oauth_info.code_verifier.empty());

  const auto rewards_wallet = engine_->wallet()->GetWallet();
  if (!rewards_wallet) {
    BLOG(0, "Rewards wallet is null!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(rewards_wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  bitflyer_server_.post_oauth().Request(
      external_account_id, std::move(oauth_info.code),
      std::move(oauth_info.code_verifier),
      base::BindOnce(&ConnectBitFlyerWallet::OnAuthorize,
                     base::Unretained(this), std::move(callback)));
}

void ConnectBitFlyerWallet::OnAuthorize(ConnectExternalWalletCallback callback,
                                        mojom::Result result,
                                        std::string&& token,
                                        std::string&& address,
                                        std::string&& linking_info) const {
  if (!engine_->bitflyer()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                         mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result != mojom::Result::OK) {
    BLOG(0, "Couldn't get token");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (token.empty()) {
    BLOG(0, "Token is empty");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (address.empty()) {
    BLOG(0, "Address is empty");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (linking_info.empty()) {
    BLOG(0, "Linking info is empty");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  auto on_connect =
      base::BindOnce(&ConnectBitFlyerWallet::OnConnect, base::Unretained(this),
                     std::move(callback), std::move(token), std::move(address));

  RequestFor<PostConnectBitflyer>(*engine_, std::move(linking_info))
      .Send(std::move(on_connect));
}

}  // namespace bitflyer

}  // namespace brave_rewards::internal
