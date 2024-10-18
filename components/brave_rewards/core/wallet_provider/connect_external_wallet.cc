/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/logging/event_log_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal {

using endpoints::PostConnect;
using mojom::ConnectExternalWalletResult;
using wallet::GetWalletIf;

namespace wallet_provider {

ConnectExternalWallet::ConnectExternalWallet(RewardsEngine& engine)
    : engine_(engine) {}

ConnectExternalWallet::~ConnectExternalWallet() = default;

std::string ConnectExternalWallet::GenerateLoginURL() {
  oauth_info_.one_time_string = engine_->options().is_testing
                                    ? "123456789"
                                    : util::GenerateRandomHexString();
  oauth_info_.code_verifier = util::GeneratePKCECodeVerifier();
  return GetOAuthLoginURL();
}

void ConnectExternalWallet::Run(
    const base::flat_map<std::string, std::string>& query_parameters,
    ConnectExternalWalletCallback callback) {
  if (oauth_info_.one_time_string.empty()) {
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  auto code = GetCode(query_parameters, oauth_info_.one_time_string);
  if (!code.has_value()) {
    return std::move(callback).Run(code.error());
  }

  oauth_info_.code = std::move(code.value());

  Authorize(std::move(callback));
}

base::expected<std::string, ConnectExternalWalletResult>
ConnectExternalWallet::GetCode(
    const base::flat_map<std::string, std::string>& query_parameters,
    const std::string& current_one_time_string) const {
  if (query_parameters.contains("error_description")) {
    const std::string message = query_parameters.at("error_description");
    engine_->Log(FROM_HERE) << message;
    if (base::Contains(message, "User does not meet minimum requirements")) {
      engine_->database()->SaveEventLog(log::kKYCRequired, WalletType());
      return base::unexpected(ConnectExternalWalletResult::kKYCRequired);
    } else if (base::Contains(message, "not available for user geolocation")) {
      engine_->database()->SaveEventLog(log::kRegionNotSupported, WalletType());
      return base::unexpected(ConnectExternalWalletResult::kRegionNotSupported);
    }

    return base::unexpected(ConnectExternalWalletResult::kUnexpected);
  }

  if (!query_parameters.contains("code") ||
      !query_parameters.contains("state")) {
    engine_->LogError(FROM_HERE)
        << "Query parameters should contain both code and state";
    return base::unexpected(ConnectExternalWalletResult::kUnexpected);
  }

  if (current_one_time_string != query_parameters.at("state")) {
    engine_->LogError(FROM_HERE) << "One time string mismatch";
    return base::unexpected(ConnectExternalWalletResult::kUnexpected);
  }

  return query_parameters.at("code");
}

void ConnectExternalWallet::OnConnect(
    ConnectExternalWalletCallback callback,
    std::string&& token,
    std::string&& address,
    endpoints::PostConnect::Result&& result) const {
  auto wallet = GetWalletIf(
      *engine_, WalletType(),
      {mojom::WalletStatus::kNotConnected, mojom::WalletStatus::kLoggedOut});
  if (!wallet) {
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  DCHECK(!token.empty());
  DCHECK(!address.empty());
  const std::string abbreviated_address = address.substr(0, 5);

  if (const auto connect_external_wallet_result =
          PostConnect::ToConnectExternalWalletResult(result);
      connect_external_wallet_result != ConnectExternalWalletResult::kSuccess) {
    engine_->LogError(FROM_HERE)
        << "Failed to connect " << WalletType() << " wallet";

    if (const auto key =
            log::GetEventLogKeyForLinkingResult(connect_external_wallet_result);
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
    engine_->LogError(FROM_HERE)
        << "Failed to transition " << WalletType() << " wallet state";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  // Set the "active" wallet type.
  engine_->Get<Prefs>().SetString(prefs::kExternalWalletType, WalletType());

  from_status == mojom::WalletStatus::kNotConnected
      ? engine_->client()->ExternalWalletConnected()
      : engine_->client()->ExternalWalletReconnected();
  engine_->database()->SaveEventLog(
      log::kWalletVerified,
      WalletType() + std::string("/") + abbreviated_address);

  // Update the user's "declared country" based on the information provided by
  // the linking endpoint.
  CHECK(result.has_value() && !result.value().empty());
  engine_->Get<Prefs>().SetString(prefs::kDeclaredGeo, result.value());

  std::move(callback).Run(ConnectExternalWalletResult::kSuccess);
}

}  // namespace wallet_provider

}  // namespace brave_rewards::internal
