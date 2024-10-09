/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"

#include <optional>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/bitcoin_rpc_responses.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/json/rs/src/lib.rs.h"
#include "components/grit/brave_components_strings.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

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

bool IsAsciiAlphaNumeric(const std::string& str) {
  return base::ranges::all_of(str, &base::IsAsciiAlphaNumeric<char>);
}

bool UrlPathEndsWithSlash(const GURL& base_url) {
  auto path_piece = base_url.path_piece();
  return !path_piece.empty() && path_piece.back() == '/';
}

const GURL MakeGetChainHeightUrl(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  const std::string path = base::StrCat({base_url.path(), "blocks/tip/height"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetFeeEstimatesUrl(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  const std::string path = base::StrCat({base_url.path(), "fee-estimates"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetTransactionUrl(const GURL& base_url,
                                 const std::string& txid) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }
  if (!IsAsciiAlphaNumeric(txid)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(), base::JoinString({"tx", txid}, "/")});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetTransactionHexUrl(const GURL& base_url,
                                    const std::string& txid) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }
  if (!IsAsciiAlphaNumeric(txid)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path = base::StrCat(
      {base_url.path(), base::JoinString({"tx", txid, "hex"}, "/")});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeAddressStatsUrl(const GURL& base_url,
                               const std::string& address) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }
  if (!IsAsciiAlphaNumeric(address)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path = base::StrCat(
      {base_url.path(), base::JoinString({"address", address}, "/")});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeUtxoListUrl(const GURL& base_url, const std::string& address) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }
  if (!IsAsciiAlphaNumeric(address)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path = base::StrCat(
      {base_url.path(), base::JoinString({"address", address, "utxo"}, "/")});

  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakePostTransactionUrl(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  const std::string path = base::StrCat({base_url.path(), "tx"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

GURL EndpointHost(const GURL& request_url) {
  DCHECK(request_url.is_valid());
  return request_url.GetWithEmptyPath();
}

bool ShouldThrottleEndpoint(const GURL& endpoint_host) {
  // Don't throttle requests if host matches brave proxy.
  return !brave_wallet::IsEndpointUsingBraveWalletProxy(endpoint_host);
}

std::optional<std::string> ConvertPlainStringToJsonArray(
    const std::string& json) {
  return base::StrCat({"[\"", json, "\"]"});
}

template <class TCallback>
void ReplyWithInvalidJsonError(TCallback callback) {
  std::move(callback).Run(
      base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
}

template <class TCallback>
void ReplyWithInternalError(TCallback callback) {
  std::move(callback).Run(
      base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
}

}  // namespace

namespace brave_wallet::bitcoin_rpc {

struct QueuedRequestData {
  GURL request_url;
  BitcoinRpc::RequestIntermediateCallback callback;
  BitcoinRpc::ResponseConversionCallback conversion_callback;
};

struct EndpointQueue {
  uint32_t active_requests = 0;
  base::circular_deque<QueuedRequestData> requests_queue;
};

BitcoinRpc::BitcoinRpc(
    NetworkManager* network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : network_manager_(network_manager),
      api_request_helper_(new APIRequestHelper(GetNetworkTrafficAnnotationTag(),
                                               url_loader_factory)) {}

BitcoinRpc::~BitcoinRpc() = default;

void BitcoinRpc::GetChainHeight(const std::string& chain_id,
                                GetChainHeightCallback callback) {
  GURL request_url = MakeGetChainHeightUrl(GetNetworkURL(chain_id));
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnGetChainHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  // Response comes as a plain integer which is not accepted by json sanitizer.
  // Wrap response into a json string array.
  auto conversion_callback = base::BindOnce(&ConvertPlainStringToJsonArray);
  RequestInternal(request_url, std::move(internal_callback),
                  std::move(conversion_callback));
}

void BitcoinRpc::OnGetChainHeight(GetChainHeightCallback callback,
                                  APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto* list = api_request_result.value_body().GetIfList();
  if (!list) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  uint32_t height = 0;
  if (list->size() != 1 || !list->front().is_string() ||
      !base::StringToUint(list->front().GetString(), &height)) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(height));
}

void BitcoinRpc::GetFeeEstimates(const std::string& chain_id,
                                 GetFeeEstimatesCallback callback) {
  GURL request_url = MakeGetFeeEstimatesUrl(GetNetworkURL(chain_id));
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnGetFeeEstimates,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(request_url, std::move(internal_callback));
}

void BitcoinRpc::OnGetFeeEstimates(GetFeeEstimatesCallback callback,
                                   APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto* dict = api_request_result.value_body().GetIfDict();
  if (!dict || dict->empty()) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::map<uint32_t, double> estimates;

  for (auto&& item : *dict) {
    uint32_t blocks = 0;
    if (!base::StringToUint(item.first, &blocks) ||
        !item.second.GetIfDouble()) {
      return ReplyWithInvalidJsonError(std::move(callback));
    }
    estimates[blocks] = item.second.GetDouble();
  }

  std::move(callback).Run(base::ok(std::move(estimates)));
}

void BitcoinRpc::GetTransaction(const std::string& chain_id,
                                const std::string& txid,
                                GetTransactionCallback callback) {
  GURL request_url = MakeGetTransactionUrl(GetNetworkURL(chain_id), txid);
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnGetTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(request_url, std::move(internal_callback));
}

void BitcoinRpc::OnGetTransaction(GetTransactionCallback callback,
                                  APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto transaction = Transaction::FromValue(api_request_result.value_body());
  if (!transaction) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(std::move(*transaction)));
}

void BitcoinRpc::GetTransactionRaw(const std::string& chain_id,
                                   const std::string& txid,
                                   GetTransactionRawCallback callback) {
  GURL request_url = MakeGetTransactionHexUrl(GetNetworkURL(chain_id), txid);
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnGetTransactionRaw,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(request_url, std::move(internal_callback),
                  base::BindOnce(&ConvertPlainStringToJsonArray));
}

void BitcoinRpc::OnGetTransactionRaw(GetTransactionRawCallback callback,
                                     APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto* list = api_request_result.value_body().GetIfList();
  if (!list) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::vector<uint8_t> transaction_raw_bytes;
  if (list->size() != 1 || !list->front().is_string() ||
      !base::HexStringToBytes(list->front().GetString(),
                              &transaction_raw_bytes)) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(transaction_raw_bytes));
}

void BitcoinRpc::GetAddressStats(const std::string& chain_id,
                                 const std::string& address,
                                 GetAddressStatsCallback callback) {
  GURL request_url = MakeAddressStatsUrl(GetNetworkURL(chain_id), address);
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnGetAddressStats,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString, "");
  RequestInternal(request_url, std::move(internal_callback),
                  std::move(conversion_callback));
}

void BitcoinRpc::OnGetAddressStats(GetAddressStatsCallback callback,
                                   APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto result = AddressStats::FromValue(api_request_result.value_body());
  if (!result) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(std::move(*result)));
}

void BitcoinRpc::GetUtxoList(const std::string& chain_id,
                             const std::string& address,
                             GetUtxoListCallback callback) {
  GURL request_url = MakeUtxoListUrl(GetNetworkURL(chain_id), address);
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnGetUtxoList, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback), address);
  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString, "");
  RequestInternal(request_url, std::move(internal_callback),
                  std::move(conversion_callback));
}

void BitcoinRpc::OnGetUtxoList(GetUtxoListCallback callback,
                               const std::string& address,
                               APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto* items = api_request_result.value_body().GetIfList();
  if (!items) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  UnspentOutputs result;

  for (const auto& item : *items) {
    auto utxo = UnspentOutput::FromValue(item);
    if (!utxo) {
      return ReplyWithInvalidJsonError(std::move(callback));
    }

    result.push_back(std::move(*utxo));
  }

  std::move(callback).Run(base::ok(std::move(result)));
}

void BitcoinRpc::PostTransaction(const std::string& chain_id,
                                 const std::vector<uint8_t>& transaction,
                                 PostTransactionCallback callback) {
  GURL request_url = MakePostTransactionUrl(GetNetworkURL(chain_id));
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto payload = base::HexEncode(transaction);

  auto internal_callback =
      base::BindOnce(&BitcoinRpc::OnPostTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&ConvertPlainStringToJsonArray);
  api_request_helper_->Request(
      net::HttpRequestHeaders::kPostMethod, request_url, payload, "",
      std::move(internal_callback), {}, {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void BitcoinRpc::OnPostTransaction(PostTransactionCallback callback,
                                   APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto* list = api_request_result.value_body().GetIfList();
  if (!list) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  if (list->size() != 1 || !list->front().is_string()) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  auto txid = list->front().GetString();
  std::vector<uint8_t> bytes(32, 0);
  if (!base::HexStringToSpan(txid, bytes)) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }
  std::move(callback).Run(base::ok(txid));
}

void BitcoinRpc::RequestInternal(
    const GURL& request_url,
    RequestIntermediateCallback callback,
    APIRequestHelper::ResponseConversionCallback conversion_callback) {
  DCHECK(request_url.is_valid());

  auto endpoint_host = EndpointHost(request_url);

  auto& endpoint = endpoints_[endpoint_host.host()];

  auto& request = endpoint.requests_queue.emplace_back();
  request.request_url = request_url;
  request.callback = std::move(callback);
  request.conversion_callback = std::move(conversion_callback);

  MaybeStartQueuedRequest(endpoint_host);
}

void BitcoinRpc::OnRequestInternalDone(const GURL& endpoint_host,
                                       RequestIntermediateCallback callback,
                                       APIRequestResult api_request_result) {
  auto& endpoint = endpoints_[endpoint_host.host()];
  endpoint.active_requests--;
  DCHECK_GE(endpoint.active_requests, 0u);
  std::move(callback).Run(std::move(api_request_result));

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&BitcoinRpc::MaybeStartQueuedRequest,
                                weak_ptr_factory_.GetWeakPtr(), endpoint_host));
}

void BitcoinRpc::MaybeStartQueuedRequest(const GURL& endpoint_host) {
  auto& endpoint = endpoints_[endpoint_host.host()];

  auto rpc_throttle = features::kBitcoinRpcThrottle.Get();
  if (ShouldThrottleEndpoint(endpoint_host) && rpc_throttle > 0 &&
      endpoint.active_requests >= static_cast<uint32_t>(rpc_throttle)) {
    return;
  }
  if (endpoint.requests_queue.empty()) {
    return;
  }

  auto request = std::move(endpoint.requests_queue.front());
  endpoint.requests_queue.pop_front();

  endpoint.active_requests++;
  api_request_helper_->Request(
      net::HttpRequestHeaders::kGetMethod, request.request_url, "", "",
      base::BindOnce(&BitcoinRpc::OnRequestInternalDone,
                     weak_ptr_factory_.GetWeakPtr(), endpoint_host,
                     std::move(request.callback)),
      MakeBraveServicesKeyHeaders(), {.auto_retry_on_network_change = true},
      std::move(request.conversion_callback));
}

void BitcoinRpc::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_->SetUrlLoaderFactoryForTesting(  // IN-TEST
      std::move(url_loader_factory));
}

GURL BitcoinRpc::GetNetworkURL(const std::string& chain_id) {
  return network_manager_->GetNetworkURL(chain_id, mojom::CoinType::BTC);
}

}  // namespace brave_wallet::bitcoin_rpc
