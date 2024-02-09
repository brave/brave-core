/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"

#include <map>
#include <optional>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

namespace {

std::optional<std::string> IsAddressStatsRequest(
    const network::ResourceRequest& request) {
  auto parts =
      base::SplitStringPiece(request.url.path_piece(), "/",
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (parts.size() == 2 && parts[0] == "address") {
    return std::string(parts[1]);
  }

  return std::nullopt;
}

std::optional<std::string> IsAddressUtxoRequest(
    const network::ResourceRequest& request) {
  auto parts =
      base::SplitStringPiece(request.url.path_piece(), "/",
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (parts.size() == 3 && parts[0] == "address" && parts[2] == "utxo") {
    return std::string(parts[1]);
  }

  return std::nullopt;
}

}  // namespace

BitcoinTestRpcServer::BitcoinTestRpcServer(KeyringService* keyring_service,
                                           PrefService* prefs)
    : keyring_service_(keyring_service), prefs_(prefs) {
  shared_url_loader_factory_ =
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &url_loader_factory_);

  url_loader_factory_.SetInterceptor(base::BindRepeating(
      &BitcoinTestRpcServer::RequestInterceptor, base::Unretained(this)));
}
BitcoinTestRpcServer::~BitcoinTestRpcServer() = default;

scoped_refptr<network::SharedURLLoaderFactory>
BitcoinTestRpcServer::GetURLLoaderFactory() {
  return shared_url_loader_factory_;
}

bitcoin_rpc::AddressStats BitcoinTestRpcServer::EmptyAddressStats(
    const std::string& address) {
  bitcoin_rpc::AddressStats stats;
  stats.address = address;
  stats.chain_stats.tx_count = "0";
  stats.chain_stats.funded_txo_sum = "0";
  stats.chain_stats.spent_txo_sum = "0";
  stats.mempool_stats.tx_count = "0";
  stats.mempool_stats.funded_txo_sum = "0";
  stats.mempool_stats.spent_txo_sum = "0";
  return stats;
}

bitcoin_rpc::AddressStats BitcoinTestRpcServer::TransactedAddressStats(
    const std::string& address) {
  bitcoin_rpc::AddressStats stats = EmptyAddressStats(address);
  stats.chain_stats.tx_count = "1";
  return stats;
}

bitcoin_rpc::AddressStats BitcoinTestRpcServer::MempoolAddressStats(
    const std::string& address,
    uint64_t funded,
    uint64_t spent) {
  bitcoin_rpc::AddressStats stats = EmptyAddressStats(address);
  stats.mempool_stats.tx_count = "1";
  stats.mempool_stats.funded_txo_sum = base::NumberToString(funded);
  stats.mempool_stats.spent_txo_sum = base::NumberToString(spent);
  return stats;
}

void BitcoinTestRpcServer::RequestInterceptor(
    const network::ResourceRequest& request) {
  url_loader_factory_.ClearResponses();

  if (request.method == net::HttpRequestHeaders::kPostMethod &&
      request.url.path_piece() == "/tx") {
    auto request_string(request.request_body->elements()
                            ->at(0)
                            .As<network::DataElementBytes>()
                            .AsStringPiece());
    captured_raw_tx_ = request_string;
    url_loader_factory_.AddResponse(request.url.spec(), kMockBtcTxid3);
    return;
  }

  if (request.url.path_piece() == "/blocks/tip/height") {
    url_loader_factory_.AddResponse(request.url.spec(),
                                    base::ToString(mainnet_height_));
    return;
  }

  if (request.url.path_piece() == "/fee-estimates") {
    url_loader_factory_.AddResponse(request.url.spec(),
                                    base::ToString(fee_estimates_));
    return;
  }

  if (auto address = IsAddressStatsRequest(request)) {
    if (address_stats_map_.contains(*address)) {
      url_loader_factory_.AddResponse(
          request.url.spec(),
          base::ToString(address_stats_map_[*address].ToValue()));
      return;
    }

    if (account_id_) {
      auto addresses = keyring_service_->GetBitcoinAddresses(account_id_);
      auto bitcoin_acc_info =
          keyring_service_->GetBitcoinAccountInfo(account_id_);
      ASSERT_TRUE(bitcoin_acc_info);

      for (const auto& item : *addresses) {
        // Assume next change and receive addresses are not transacted.
        if (item == bitcoin_acc_info->next_change_address ||
            item == bitcoin_acc_info->next_receive_address) {
          continue;
        }

        if (item->address_string == *address) {
          url_loader_factory_.AddResponse(
              request.url.spec(),
              base::ToString(TransactedAddressStats(*address).ToValue()));
          return;
        }
      }
    }

    url_loader_factory_.AddResponse(
        request.url.spec(),
        base::ToString(EmptyAddressStats(*address).ToValue()));
    return;
  }

  if (auto address = IsAddressUtxoRequest(request)) {
    if (utxos_map_.contains(*address)) {
      base::Value::List items;
      for (auto& utxo : utxos_map_[*address]) {
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

void BitcoinTestRpcServer::SetUpBitcoinRpc(
    const mojom::AccountIdPtr& account_id) {
  auto btc_mainnet =
      GetKnownChain(prefs_, mojom::kBitcoinMainnet, mojom::CoinType::BTC);
  btc_mainnet->rpc_endpoints[0] = GURL(mainnet_rpc_url_);
  AddCustomNetwork(prefs_, *btc_mainnet);
  auto btc_testnet =
      GetKnownChain(prefs_, mojom::kBitcoinTestnet, mojom::CoinType::BTC);
  btc_testnet->rpc_endpoints[0] = GURL(testnet_rpc_url_);
  AddCustomNetwork(prefs_, *btc_testnet);

  address_0_.clear();
  address_6_.clear();
  address_stats_map_.clear();
  utxos_map_.clear();

  account_id_ = account_id.Clone();

  if (account_id_) {
    address_0_ =
        keyring_service_
            ->GetBitcoinAddress(account_id_, mojom::BitcoinKeyId::New(0, 0))
            ->address_string;
    auto& stats_0 = address_stats_map_[address_0_];
    stats_0.address = address_0_;
    stats_0.chain_stats.funded_txo_sum = "10000";
    stats_0.chain_stats.spent_txo_sum = "5000";
    stats_0.chain_stats.tx_count = "1";
    stats_0.mempool_stats.funded_txo_sum = "8888";
    stats_0.mempool_stats.spent_txo_sum = "2222";
    stats_0.mempool_stats.tx_count = "1";

    address_6_ =
        keyring_service_
            ->GetBitcoinAddress(account_id_, mojom::BitcoinKeyId::New(1, 0))
            ->address_string;
    auto& stats_6 = address_stats_map_[address_6_];
    stats_6.address = address_6_;
    stats_6.chain_stats.funded_txo_sum = "100000";
    stats_6.chain_stats.spent_txo_sum = "50000";
    stats_6.chain_stats.tx_count = "1";
    stats_6.mempool_stats.funded_txo_sum = "88888";
    stats_6.mempool_stats.spent_txo_sum = "22222";
    stats_6.mempool_stats.tx_count = "1";

    auto& utxos_0 = utxos_map_[address_0_];
    utxos_0.emplace_back();
    utxos_0.back().txid = kMockBtcTxid1;
    utxos_0.back().vout = "1";
    utxos_0.back().value = "5000";
    utxos_0.back().status.confirmed = true;
    auto& utxos_6 = utxos_map_[address_6_];
    utxos_6.emplace_back();
    utxos_6.back().txid = kMockBtcTxid2;
    utxos_6.back().vout = "7";
    utxos_6.back().value = "50000";
    utxos_6.back().status.confirmed = true;
  }

  fee_estimates_ = ParseJson(
      R"(
        {
          "1": 28.322,
          "2": 28.322,
          "3": 25.838,
          "4": 23.456,  // This is used as default target block fee.
          "5": 23.219,
          "6": 23.219,
          "7": 23.219,
          "8": 23.219,
          "9": 23.219,
          "10": 23.219,
          "11": 23.219,
          "12": 23.219,
          "13": 16.53,
          "14": 16.53,
          "15": 16.53,
          "16": 16.53,
          "17": 16.53,
          "18": 16.53,
          "19": 16.53,
          "20": 16.53,
          "21": 16.53,
          "22": 16.53,
          "23": 16.53,
          "24": 16.53,
          "25": 15.069,
          "144": 12.992,
          "504": 12.361,
          "1008": 1.93
        }
        )");
}

void BitcoinTestRpcServer::AddTransactedAddress(const std::string& address) {
  address_stats_map()[address] = TransactedAddressStats(address);
}

void BitcoinTestRpcServer::AddMempoolBalance(const std::string& address,
                                             uint64_t funded,
                                             uint64_t spent) {
  address_stats_map()[address] = MempoolAddressStats(address, funded, spent);
}

}  // namespace brave_wallet
