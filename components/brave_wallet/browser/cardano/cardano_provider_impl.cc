/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"

#include <utility>

namespace brave_wallet {

CardanoProviderImpl::CardanoProviderImpl() = default;

CardanoProviderImpl::~CardanoProviderImpl() = default;

void CardanoProviderImpl::Enable(EnableCallback callback) {
  std::move(callback).Run(true);
}

void CardanoProviderImpl::GetNetworkId(GetNetworkIdCallback callback) {
  std::move(callback).Run(0, std::nullopt);
}

void CardanoProviderImpl::GetUsedAddresses(GetUsedAddressesCallback callback) {
  std::move(callback).Run({"1", "2", "3"}, std::nullopt);
}

void CardanoProviderImpl::GetUnusedAddresses(
    GetUnusedAddressesCallback callback) {
  std::move(callback).Run({"1", "2", "3"}, std::nullopt);
}

void CardanoProviderImpl::GetChangeAddress(GetChangeAddressCallback callback) {
  std::move(callback).Run("1", std::nullopt);
}

void CardanoProviderImpl::GetRewardAddresses(
    GetRewardAddressesCallback callback) {
  std::move(callback).Run({"2"}, std::nullopt);
}

void CardanoProviderImpl::GetBalance(GetBalanceCallback callback) {
  std::move(callback).Run("2", std::nullopt);
}

void CardanoProviderImpl::GetUtxos(const std::optional<std::string>& amount,
                                   mojom::CardanoProviderPaginationPtr paginate,
                                   GetUtxosCallback callback) {
  std::move(callback).Run({"1", "2"}, std::nullopt);
}

void CardanoProviderImpl::SignTx(const std::string& tx_cbor,
                                 bool partial_sign,
                                 SignTxCallback callback) {
  std::move(callback).Run("signed", std::nullopt);
}

void CardanoProviderImpl::SubmitTx(const std::string& signed_tx_cbor,
                                   SubmitTxCallback callback) {
  std::move(callback).Run("txhash", std::nullopt);
}

void CardanoProviderImpl::SignData(const std::string& address,
                                   const std::string& payload_hex,
                                   SignDataCallback callback) {
  std::move(callback).Run(mojom::CardanoProviderSignatureResult::New("1", "2"),
                          std::nullopt);
}

}  // namespace brave_wallet
