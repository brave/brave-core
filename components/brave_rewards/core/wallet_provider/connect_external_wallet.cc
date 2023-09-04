/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/logging/event_log_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal {
using endpoints::GetWallet;
using endpoints::PostConnect;
using endpoints::RequestFor;
using wallet::GetWalletIf;

namespace wallet_provider {

ConnectExternalWallet::ConnectExternalWallet(RewardsEngineImpl& engine)
    : engine_(engine) {
  // TODO(https://github.com/brave/brave-browser/issues/31698)
  linkage_checker_.Start(FROM_HERE, base::Minutes(1), this,
                         &ConnectExternalWallet::CheckLinkage);
}

ConnectExternalWallet::~ConnectExternalWallet() = default;

void ConnectExternalWallet::Run(
    const base::flat_map<std::string, std::string>& query_parameters,
    ConnectExternalWalletCallback callback) {
  auto wallet = GetWalletIf(
      *engine_, WalletType(),
      {mojom::WalletStatus::kNotConnected, mojom::WalletStatus::kLoggedOut});
  if (!wallet) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  auto oauth_info = ExchangeOAuthInfo(std::move(wallet));
  if (!oauth_info) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  auto code = GetCode(query_parameters, oauth_info->one_time_string);
  if (!code.has_value()) {
    return std::move(callback).Run(base::unexpected(code.error()));
  }

  oauth_info->code = std::move(code.value());

  Authorize(std::move(*oauth_info), std::move(callback));
}

absl::optional<ConnectExternalWallet::OAuthInfo>
ConnectExternalWallet::ExchangeOAuthInfo(
    mojom::ExternalWalletPtr wallet) const {
  DCHECK(wallet);
  if (!wallet) {
    return absl::nullopt;
  }

  OAuthInfo oauth_info;
  // We need to generate a new OTS (and code verifier for bitFlyer) as soon as
  // external wallet connection is triggered.
  oauth_info.one_time_string =
      std::exchange(wallet->one_time_string, util::GenerateRandomHexString());
  oauth_info.code_verifier =
      std::exchange(wallet->code_verifier, util::GeneratePKCECodeVerifier());

  wallet = wallet::GenerateLinks(std::move(wallet));
  if (!wallet) {
    BLOG(0, "Failed to generate links for " << WalletType() << " wallet!");
    return absl::nullopt;
  }

  if (!wallet::SetWallet(*engine_, std::move(wallet))) {
    BLOG(0, "Failed to save " << WalletType() << " wallet!");
    return absl::nullopt;
  }

  return oauth_info;
}

base::expected<std::string, mojom::ConnectExternalWalletError>
ConnectExternalWallet::GetCode(
    const base::flat_map<std::string, std::string>& query_parameters,
    const std::string& current_one_time_string) const {
  if (query_parameters.contains("error_description")) {
    const std::string message = query_parameters.at("error_description");
    BLOG(1, message);
    if (base::Contains(message, "User does not meet minimum requirements")) {
      engine_->database()->SaveEventLog(log::kKYCRequired, WalletType());
      return base::unexpected(mojom::ConnectExternalWalletError::kKYCRequired);
    } else if (base::Contains(message, "not available for user geolocation")) {
      engine_->database()->SaveEventLog(log::kRegionNotSupported, WalletType());
      return base::unexpected(
          mojom::ConnectExternalWalletError::kRegionNotSupported);
    }

    return base::unexpected(mojom::ConnectExternalWalletError::kUnexpected);
  }

  if (!query_parameters.contains("code") ||
      !query_parameters.contains("state")) {
    BLOG(0, "Query parameters should contain both code and state!");
    return base::unexpected(mojom::ConnectExternalWalletError::kUnexpected);
  }

  if (current_one_time_string != query_parameters.at("state")) {
    BLOG(0, "One time string mismatch!");
    return base::unexpected(mojom::ConnectExternalWalletError::kUnexpected);
  }

  return query_parameters.at("code");
}

void ConnectExternalWallet::CheckLinkage() {
  if (!engine_->IsReady()) {
    return linkage_checker_.Reset();
  }

  if (GetWalletIf(
          *engine_, WalletType(),
          {mojom::WalletStatus::kConnected, mojom::WalletStatus::kLoggedOut})) {
    RequestFor<GetWallet>(*engine_).Send(base::BindOnce(
        &ConnectExternalWallet::CheckLinkageCallback, base::Unretained(this)));
  }
}

void ConnectExternalWallet::CheckLinkageCallback(
    endpoints::GetWallet::Result&& result) {
  auto wallet = GetWalletIf(
      *engine_, WalletType(),
      {mojom::WalletStatus::kConnected, mojom::WalletStatus::kLoggedOut});
  if (!wallet) {
    return;
  }

  if (result.has_value()) {
    const auto [wallet_type, linked] = std::move(result.value());
    if (wallet_type == WalletType() && !linked) {
      // {kConnected, kLoggedOut} ==> kNotConnected
      if (wallet::TransitionWallet(*engine_, std::move(wallet),
                                   mojom::WalletStatus::kNotConnected)) {
        engine_->client()->ExternalWalletDisconnected();
      } else {
        BLOG(0, "Failed to transition " << WalletType() << " wallet state!");
      }
    }
  }
}

void ConnectExternalWallet::OnConnect(
    ConnectExternalWalletCallback callback,
    std::string&& token,
    std::string&& address,
    std::string&& country_id,
    endpoints::PostConnect::Result&& result) const {
  auto wallet = GetWalletIf(
      *engine_, WalletType(),
      {mojom::WalletStatus::kNotConnected, mojom::WalletStatus::kLoggedOut});
  if (!wallet) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  DCHECK(!token.empty());
  DCHECK(!address.empty());
  const std::string abbreviated_address = address.substr(0, 5);

  if (const auto connect_external_wallet_result =
          PostConnect::ToConnectExternalWalletResult(result);
      !connect_external_wallet_result.has_value()) {
    BLOG(0, "Failed to connect " << WalletType() << " wallet!");

    if (const auto key = log::GetEventLogKeyForLinkingResult(
            connect_external_wallet_result.error());
        !key.empty()) {
      engine_->database()->SaveEventLog(
          key, WalletType() + std::string("/") + abbreviated_address);
    }

    return std::move(callback).Run(connect_external_wallet_result);
  }

  const auto from_status = wallet->status;
  wallet->token = std::move(token);
  wallet->address = std::move(address);
  // {kNotConnected, kLoggedOut} ==> kConnected
  if (!wallet::TransitionWallet(*engine_, std::move(wallet),
                                mojom::WalletStatus::kConnected)) {
    BLOG(0, "Failed to transition " << WalletType() << " wallet state!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  from_status == mojom::WalletStatus::kNotConnected
      ? engine_->client()->ExternalWalletConnected()
      : engine_->client()->ExternalWalletReconnected();
  engine_->database()->SaveEventLog(
      log::kWalletVerified,
      WalletType() + std::string("/") + abbreviated_address);

  // Update the user's "declared country" based on the information provided by
  // the wallet provider.
  if (!country_id.empty()) {
    engine_->SetState(state::kDeclaredGeo, country_id);
  }

  std::move(callback).Run({});
}

}  // namespace wallet_provider

}  // namespace brave_rewards::internal
