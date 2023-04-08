/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"

#include "base/command_line.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("bitcoin_rpc", R"(
      semantics {
        sender: "Bitcoin RPC"
        description:
          "This service is used to communicate with Bitcoin nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Bitcoin JSON RPC response bodies."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

const GURL MakeGetChainHeightUrl(const GURL& base_url) {
  GURL::Replacements replacements;
  const std::string path =
      base_url.path() + base::JoinString({"blocks", "tip", "height"}, "/");
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeAddressHistoryUrl(const GURL& base_url,
                                 const std::string& address,
                                 const std::string& last_seen_txid) {
  GURL::Replacements replacements;
  std::string path =
      base_url.path() +
      base::JoinString({"address", address, "txs", "chain"}, "/");
  if (!last_seen_txid.empty()) {
    path += "/" + last_seen_txid;
  }
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakePostTransactionUrl(const GURL& base_url) {
  GURL::Replacements replacements;
  const std::string path = base_url.path() + "tx";
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

std::string BitcoinRpcHost() {
  return base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
      brave_wallet::switches::kBitcoinRpcHost);
}

GURL BaseRpcUrl(const std::string& network_id) {
  CHECK(brave_wallet::IsBitcoinNetwork(network_id));

  std::string host = BitcoinRpcHost();
  std::string path = network_id == brave_wallet::mojom::kBitcoinMainnet
                         ? "/api/"
                         : "/testnet/api/";

  return GURL("https://" + host + path);
}

absl::optional<std::string> ConvertPlainIntToJsonArray(
    const std::string& json) {
  return "[" + json + "]";
}

absl::optional<std::string> ConvertPlainStringToJsonArray(
    const std::string& json) {
  return "[\"" + json + "\"]";
}

}  // namespace

namespace brave_wallet {

BitcoinRpc::BitcoinRpc(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(new APIRequestHelper(GetNetworkTrafficAnnotationTag(),
                                               url_loader_factory)) {}

BitcoinRpc::~BitcoinRpc() = default;

void BitcoinRpc::GetChainHeight(const std::string& network_id,
                                GetChainHeightCallback callback) {
  GURL network_url = BaseRpcUrl(network_id);

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnGetChainHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  // Response comes as a plain integer which is not accepted by json sanitizer.
  // Wrap response into a json array.
  auto conversion_callback = base::BindOnce(&ConvertPlainIntToJsonArray);
  RequestInternal(true, MakeGetChainHeightUrl(network_url),
                  std::move(internal_callback), std::move(conversion_callback));
}

void BitcoinRpc::OnGetChainHeight(GetChainHeightCallback callback,
                                  APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(base::unexpected("Unexpected HTTP result code"));
    return;
  }

  auto* list = api_request_result.value_body().GetIfList();
  if (!list) {
    std::move(callback).Run(base::unexpected("Json response is not an arary"));
    return;
  }

  if (list->size() != 1 || !list->front().is_int()) {
    std::move(callback).Run(
        base::unexpected("Json response value is not an int"));
    return;
  }

  std::move(callback).Run(base::ok(list->front().GetInt()));
}

void BitcoinRpc::GetAddressHistory(const std::string& network_id,
                                   const std::string& address,
                                   const uint32_t max_block_height,
                                   const std::string& last_seen_txid,
                                   GetAddressHistoryCallback callback) {
  GURL network_url = BaseRpcUrl(network_id);

  auto internal_callback = base::BindOnce(
      &BitcoinRpc::OnGetAddressHistory, weak_ptr_factory_.GetWeakPtr(),
      max_block_height, std::move(callback));
  RequestInternal(true,
                  MakeAddressHistoryUrl(network_url, address, last_seen_txid),
                  std::move(internal_callback));
}

void BitcoinRpc::OnGetAddressHistory(const uint32_t max_block_height,
                                     GetAddressHistoryCallback callback,
                                     APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(base::unexpected("Unexpected HTTP result code"));
    return;
  }

  auto* items = api_request_result.value_body().GetIfList();
  if (!items) {
    std::move(callback).Run(base::unexpected("Json response is not an arary"));
    return;
  }

  std::vector<Transaction> result;

  for (auto& item : *items) {
    auto transaction = Transaction::FromRpcValue(item);
    if (!transaction) {
      std::move(callback).Run(base::unexpected("Invalid transaction dict"));
      return;
    }
    if (transaction->block_height > max_block_height) {
      continue;
    }

    result.push_back(std::move(*transaction));
  }

  std::move(callback).Run(base::ok(result));
}

void BitcoinRpc::PostTransaction(const std::string& network_id,
                                 const std::vector<uint8_t>& transaction,
                                 PostTransactionCallback callback) {
  GURL network_url = BaseRpcUrl(network_id);
  auto payload = base::HexEncode(transaction);

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnPostTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&ConvertPlainStringToJsonArray);
  api_request_helper_->Request("POST", MakePostTransactionUrl(network_url),
                               payload, "", true, std::move(internal_callback),
                               {}, -1u, std::move(conversion_callback));
}

void BitcoinRpc::OnPostTransaction(PostTransactionCallback callback,
                                   APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(base::unexpected("Unexpected HTTP result code"));
    return;
  }

  auto* list = api_request_result.value_body().GetIfList();
  if (!list) {
    std::move(callback).Run(base::unexpected("Json response is not an arary"));
    return;
  }

  if (list->size() != 1 || !list->front().is_string()) {
    std::move(callback).Run(base::unexpected("Invalid txid string"));
    return;
  }

  auto txid = list->front().GetString();
  std::move(callback).Run(base::ok(txid));
}

void BitcoinRpc::RequestInternal(
    bool auto_retry_on_network_change,
    const GURL& request_url,
    RequestIntermediateCallback callback,
    APIRequestHelper::ResponseConversionCallback conversion_callback) {
  DCHECK(request_url.is_valid());

  api_request_helper_->Request(
      "GET", request_url, "", "", auto_retry_on_network_change,
      std::move(callback), {}, -1u, std::move(conversion_callback));
}

}  // namespace brave_wallet
