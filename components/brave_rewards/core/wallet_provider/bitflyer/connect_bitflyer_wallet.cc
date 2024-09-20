/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/connect_bitflyer_wallet.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/endpoint/bitflyer/bitflyer_server.h"
#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_bitflyer.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "crypto/sha2.h"

namespace brave_rewards::internal {

using endpoints::PostConnectBitflyer;
using endpoints::RequestFor;
using mojom::ConnectExternalWalletResult;
using wallet_provider::ConnectExternalWallet;

namespace bitflyer {

ConnectBitFlyerWallet::ConnectBitFlyerWallet(RewardsEngine& engine)
    : ConnectExternalWallet(engine), bitflyer_server_(engine) {}

ConnectBitFlyerWallet::~ConnectBitFlyerWallet() = default;

const char* ConnectBitFlyerWallet::WalletType() const {
  return constant::kWalletBitflyer;
}

std::string ConnectBitFlyerWallet::GetOAuthLoginURL() const {
  auto& config = engine_->Get<EnvironmentConfig>();

  auto url = config.bitflyer_url().Resolve("/ex/OAuth/authorize");

  url = AppendOrReplaceQueryParameters(
      url, {{"client_id", config.bitflyer_client_id()},
            {"scope", "assets create_deposit_id withdraw_to_deposit_id"},
            {"redirect_uri", "rewards://bitflyer/authorization"},
            {"state", oauth_info_.one_time_string},
            {"response_type", "code"},
            {"code_challenge_method", "S256"},
            {"code_challenge",
             util::GeneratePKCECodeChallenge(oauth_info_.code_verifier)}});

  return url.spec();
}

void ConnectBitFlyerWallet::Authorize(ConnectExternalWalletCallback callback) {
  DCHECK(!oauth_info_.code.empty());
  DCHECK(!oauth_info_.code_verifier.empty());

  const auto rewards_wallet = engine_->wallet()->GetWallet();
  if (!rewards_wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet is null";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(rewards_wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  bitflyer_server_.post_oauth().Request(
      external_account_id, oauth_info_.code, oauth_info_.code_verifier,
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
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Couldn't get token";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (token.empty()) {
    engine_->LogError(FROM_HERE) << "Token is empty";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (address.empty()) {
    engine_->LogError(FROM_HERE) << "Address is empty";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (linking_info.empty()) {
    engine_->LogError(FROM_HERE) << "Linking info is empty";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  auto on_connect =
      base::BindOnce(&ConnectBitFlyerWallet::OnConnect, base::Unretained(this),
                     std::move(callback), std::move(token), std::move(address));

  RequestFor<PostConnectBitflyer>(*engine_, std::move(linking_info))
      .Send(std::move(on_connect));
}

}  // namespace bitflyer

}  // namespace brave_rewards::internal
