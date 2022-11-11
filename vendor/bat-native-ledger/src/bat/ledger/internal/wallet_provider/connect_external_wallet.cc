/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/connect_external_wallet.h"

#include <utility>

#include "base/containers/contains.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/logging/event_log_util.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

using ledger::endpoints::PostConnect;
using ledger::wallet::GetWalletIf;

namespace ledger::wallet_provider {

ConnectExternalWallet::ConnectExternalWallet(LedgerImpl* ledger)
    : ledger_(ledger) {
  DCHECK(ledger_);
}

ConnectExternalWallet::~ConnectExternalWallet() = default;

void ConnectExternalWallet::Run(
    const base::flat_map<std::string, std::string>& query_parameters,
    ledger::ConnectExternalWalletCallback callback) const {
  auto wallet = GetWalletIf(
      ledger_, WalletType(),
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
    ledger::mojom::ExternalWalletPtr wallet) const {
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

  wallet = ledger::wallet::GenerateLinks(std::move(wallet));
  if (!wallet) {
    BLOG(0, "Failed to generate links for " << WalletType() << " wallet!");
    return absl::nullopt;
  }

  if (!ledger::wallet::SetWallet(ledger_, std::move(wallet))) {
    BLOG(0, "Failed to save " << WalletType() << " wallet!");
    return absl::nullopt;
  }

  return oauth_info;
}

base::expected<std::string, ledger::mojom::ConnectExternalWalletError>
ConnectExternalWallet::GetCode(
    const base::flat_map<std::string, std::string>& query_parameters,
    const std::string& current_one_time_string) const {
  if (query_parameters.contains("error_description")) {
    const std::string message = query_parameters.at("error_description");
    BLOG(1, message);
    if (base::Contains(message, "User does not meet minimum requirements")) {
      ledger_->database()->SaveEventLog(log::kKYCRequired, WalletType());
      return base::unexpected(mojom::ConnectExternalWalletError::kKYCRequired);
    } else if (base::Contains(message, "not available for user geolocation")) {
      ledger_->database()->SaveEventLog(log::kRegionNotSupported, WalletType());
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

void ConnectExternalWallet::OnConnect(
    ledger::ConnectExternalWalletCallback callback,
    std::string&& token,
    std::string&& address,
    endpoints::PostConnect::Result&& result) const {
  auto wallet = GetWalletIf(
      ledger_, WalletType(),
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
      ledger_->database()->SaveEventLog(
          key, WalletType() + std::string("/") + abbreviated_address);
    }

    return std::move(callback).Run(connect_external_wallet_result);
  }

  const auto from_status = wallet->status;
  wallet->token = std::move(token);
  wallet->address = std::move(address);
  // {kNotConnected, kLoggedOut} ==> kConnected
  if (!ledger::wallet::TransitionWallet(ledger_, std::move(wallet),
                                        mojom::WalletStatus::kConnected)) {
    BLOG(0, "Failed to transition " << WalletType() << " wallet state!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  from_status == mojom::WalletStatus::kNotConnected
      ? ledger_->ledger_client()->ExternalWalletConnected()
      : ledger_->ledger_client()->ExternalWalletReconnected();
  ledger_->database()->SaveEventLog(
      log::kWalletVerified,
      WalletType() + std::string("/") + abbreviated_address);
  std::move(callback).Run({});
}

}  // namespace ledger::wallet_provider
