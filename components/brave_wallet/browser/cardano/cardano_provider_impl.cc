/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"

#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

namespace brave_wallet {

CardanoProviderImpl::CardanoProviderImpl(
    HostContentSettingsMap& host_content_settings_map,
    BraveWalletService* brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate)
    : host_content_settings_map_(host_content_settings_map),
      brave_wallet_service_(brave_wallet_service),
      delegate_(std::move(delegate)) {}

CardanoProviderImpl::~CardanoProviderImpl() = default;

void CardanoProviderImpl::Enable(EnableCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run(true);
}

void CardanoProviderImpl::IsEnabled(IsEnabledCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run(true);
}

void CardanoProviderImpl::GetNetworkId(GetNetworkIdCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run(0, std::nullopt);
}

void CardanoProviderImpl::GetUsedAddresses(GetUsedAddressesCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run({"1", "2", "3"}, std::nullopt);
}

void CardanoProviderImpl::GetUnusedAddresses(
    GetUnusedAddressesCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run({"1", "2", "3"}, std::nullopt);
}

void CardanoProviderImpl::GetChangeAddress(GetChangeAddressCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run("1", std::nullopt);
}

void CardanoProviderImpl::GetRewardAddresses(
    GetRewardAddressesCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run({"2"}, std::nullopt);
}

void CardanoProviderImpl::GetBalance(GetBalanceCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run("2", std::nullopt);
}

void CardanoProviderImpl::GetUtxos(const std::optional<std::string>& amount,
                                   mojom::CardanoProviderPaginationPtr paginate,
                                   GetUtxosCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run(std::vector<std::string>({"1", "2"}), std::nullopt);
}

void CardanoProviderImpl::SignTx(const std::string& tx_cbor,
                                 bool partial_sign,
                                 SignTxCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run("signed", std::nullopt);
}

void CardanoProviderImpl::SubmitTx(const std::string& signed_tx_cbor,
                                   SubmitTxCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run("txhash", std::nullopt);
}

void CardanoProviderImpl::SignData(const std::string& address,
                                   const std::string& payload_hex,
                                   SignDataCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run(mojom::CardanoProviderSignatureResult::New("1", "2"),
                          std::nullopt);
}

void CardanoProviderImpl::GetCollateral(const std::string& amount,
                                        GetCollateralCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run(std::vector<std::string>({"1", "2"}), std::nullopt);
}

}  // namespace brave_wallet
