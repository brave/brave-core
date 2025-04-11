/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep

namespace brave_wallet {

CardanoTestRpcServer::CardanoTestRpcServer(
    CardanoWalletService& cardano_wallet_service)
    : cardano_wallet_service_(cardano_wallet_service) {
  url_loader_factory_.SetInterceptor(base::BindRepeating(
      &CardanoTestRpcServer::RequestInterceptor, base::Unretained(this)));
  cardano_wallet_service.SetUrlLoaderFactoryForTesting(
      url_loader_factory_.GetSafeWeakWrapper());
}

CardanoTestRpcServer::~CardanoTestRpcServer() = default;

void CardanoTestRpcServer::RequestInterceptor(
    const network::ResourceRequest& request) {
  url_loader_factory_.ClearResponses();

  if (auto address = IsAddressUtxoRequest(request)) {
    if (utxos_map_.contains(*address)) {
      base::Value::List items;
      for (const auto& utxo : utxos_map_[*address]) {
        items.Append(utxo.ToValue());
      }
      url_loader_factory_.AddResponse(request.url.spec(),
                                      base::ToString(items));
      return;
    }
    url_loader_factory_.AddResponse(request.url.spec(), "[]");
    return;
  }

  NOTREACHED() << request.url.spec();
}

std::string CardanoTestRpcServer::ExtractApiRequestPath(
    const GURL& request_url) {
  std::string spec = request_url.spec();

  auto mainnet_url_spec =
      cardano_wallet_service_->network_manager()
          .GetNetworkURL(mojom::kCardanoMainnet, mojom::CoinType::ADA)
          .spec();
  auto testnet_url_spec =
      cardano_wallet_service_->network_manager()
          .GetNetworkURL(mojom::kCardanoTestnet, mojom::CoinType::ADA)
          .spec();

  if (spec.starts_with(mainnet_url_spec)) {
    return spec.substr(mainnet_url_spec.size());
  }
  if (spec.starts_with(testnet_url_spec)) {
    return spec.substr(testnet_url_spec.size());
  }

  return spec;
}

std::optional<std::string> CardanoTestRpcServer::IsAddressUtxoRequest(
    const network::ResourceRequest& request) {
  auto parts =
      base::SplitString(ExtractApiRequestPath(request.url), "/",
                        base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (parts.size() == 3 && parts[0] == "addresses" && parts[2] == "utxos") {
    return parts[1];
  }

  return std::nullopt;
}

void CardanoTestRpcServer::SetUpCardanoRpc(
    const std::optional<std::string>& mnemonic,
    std::optional<uint32_t> account_index) {
  auto keyring = std::make_unique<CardanoHDKeyring>(
      *bip39::MnemonicToEntropy(*mnemonic), mojom::KeyringId::kCardanoMainnet);

  auto address_external_0 = keyring->GetAddress(
      0, mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal, 0));
  AddUtxo(address_external_0->address_string, 54321);
  AddUtxo(address_external_0->address_string, 600000);

  auto address_internal_0 = keyring->GetAddress(
      0, mojom::CardanoKeyId(mojom::CardanoKeyRole::kInternal, 0));
  AddUtxo(address_internal_0->address_string, 7000000);
}

std::array<uint8_t, 32> CardanoTestRpcServer::CreateNewTxHash() {
  auto result = next_tx_hash_;
  next_tx_hash_.front()++;
  return result;
}

void CardanoTestRpcServer::AddUtxo(const std::string& address,
                                   uint32_t amount) {
  auto& utxo = utxos_map_[address].emplace_back();
  utxo.tx_hash = HexEncodeLower(CreateNewTxHash());
  utxo.output_index = "13";
  utxo.amount.emplace_back();
  utxo.amount.back().quantity = base::NumberToString(amount);
  utxo.amount.back().unit = "lovelace";
}

}  // namespace brave_wallet
