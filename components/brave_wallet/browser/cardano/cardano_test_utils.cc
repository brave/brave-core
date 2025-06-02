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
#include <utility>

#include "base/containers/extend.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_blockfrost_api.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "net/http/http_status_code.h"
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
    if (fail_address_utxo_request_) {
      url_loader_factory_.AddResponse(request.url.spec(), "Bad request",
                                      net::HTTP_BAD_REQUEST);
      return;
    }

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

  if (IsLatestEpochParametersRequest(request)) {
    if (fail_latest_epoch_parameters_request_) {
      url_loader_factory_.AddResponse(request.url.spec(), "Bad request",
                                      net::HTTP_BAD_REQUEST);
      return;
    }

    cardano_rpc::blockfrost_api::EpochParameters params;
    params.min_fee_a = "44";
    params.min_fee_b = "155381";
    url_loader_factory_.AddResponse(
        request.url.spec(), *base::WriteJsonWithOptions(params.ToValue(), 0));
    return;
  }

  if (IsLatestBlockRequest(request)) {
    if (fail_latest_block_request_) {
      url_loader_factory_.AddResponse(request.url.spec(), "Bad request",
                                      net::HTTP_BAD_REQUEST);
      return;
    }

    cardano_rpc::blockfrost_api::Block latest_block;
    latest_block.height = "11854454";
    latest_block.slot = "155479747";
    latest_block.epoch = "557";
    url_loader_factory_.AddResponse(
        request.url.spec(),
        *base::WriteJsonWithOptions(latest_block.ToValue(), 0));
    return;
  }

  if (IsTxSubmitRequest(request)) {
    auto request_bytes = request.request_body->elements()
                             ->at(0)
                             .As<network::DataElementBytes>()
                             .bytes();

    captured_raw_tx_ = base::HexEncode(request_bytes);

    if (fail_next_transaction_submission_) {
      fail_next_transaction_submission_ = false;
      url_loader_factory_.AddResponse(request.url.spec(), "Bad request",
                                      net::HTTP_BAD_REQUEST);
      return;
    }

    url_loader_factory_.AddResponse(
        request.url.spec(), base::StrCat({"\"", kMockCardanoTxid, "\""}));
    mempool_transactions_.push_back(kMockCardanoTxid);
    return;
  }

  if (auto txid = IsGetTransactionRequest(request)) {
    if (base::Contains(confirmed_transactions_, *txid)) {
      cardano_rpc::blockfrost_api::Transaction tx;
      tx.hash = *txid;
      url_loader_factory_.AddResponse(
          request.url.spec(), *base::WriteJsonWithOptions(tx.ToValue(), 0));
      return;
    }
    url_loader_factory_.AddResponse(request.url.spec(), "Not found",
                                    net::HTTP_NOT_FOUND);
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

bool CardanoTestRpcServer::IsLatestEpochParametersRequest(
    const network::ResourceRequest& request) {
  return ExtractApiRequestPath(request.url) == "epochs/latest/parameters";
}

bool CardanoTestRpcServer::IsLatestBlockRequest(
    const network::ResourceRequest& request) {
  return ExtractApiRequestPath(request.url) == "blocks/latest";
}

bool CardanoTestRpcServer::IsTxSubmitRequest(
    const network::ResourceRequest& request) {
  return request.method == net::HttpRequestHeaders::kPostMethod &&
         ExtractApiRequestPath(request.url) == "tx/submit";
}

std::optional<std::string> CardanoTestRpcServer::IsGetTransactionRequest(
    const network::ResourceRequest& request) {
  auto parts =
      base::SplitString(ExtractApiRequestPath(request.url), "/",
                        base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (parts.size() == 2 && parts[0] == "txs") {
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

void CardanoTestRpcServer::FailNextTransactionSubmission() {
  fail_next_transaction_submission_ = true;
}

void CardanoTestRpcServer::ConfirmAllTransactions() {
  base::Extend(confirmed_transactions_, std::move(mempool_transactions_));
}

void CardanoTestRpcServer::AddConfirmedTransaction(const std::string& txid) {
  confirmed_transactions_.push_back(txid);
}

scoped_refptr<network::SharedURLLoaderFactory>
CardanoTestRpcServer::GetURLLoaderFactory() {
  return shared_url_loader_factory_;
}

}  // namespace brave_wallet
