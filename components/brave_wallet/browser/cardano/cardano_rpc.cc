/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_rpc.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_blockfrost_api.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

constexpr char kNotFoundResponse[] = "Not Found";
constexpr char kProjectIdHeader[] = "project_id";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("cardano_rpc", R"(
      semantics {
        sender: "Cardano RPC"
        description:
          "This service is used to communicate with Cardano nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Cardano JSON RPC response bodies."
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

base::flat_map<std::string, std::string> MakeCardanoRpcHeaders(
    const std::string& chain_id,
    const GURL& request_url) {
  base::flat_map<std::string, std::string> request_headers =
      brave_wallet::IsEndpointUsingBraveWalletProxy(request_url)
          ? brave_wallet::MakeBraveServicesKeyHeaders()
          : base::flat_map<std::string, std::string>();

  std::string cardano_project_id =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          chain_id == brave_wallet::mojom::kCardanoMainnet
              ? brave_wallet::switches::kCardanoMainnetProjectId
              : brave_wallet::switches::kCardanoTestnetProjectId);
  if (!cardano_project_id.empty()) {
    request_headers.emplace(kProjectIdHeader, cardano_project_id);
  }

  return request_headers;
}

bool UrlPathEndsWithSlash(const GURL& base_url) {
  auto path_piece = base_url.path_piece();
  return !path_piece.empty() && path_piece.back() == '/';
}

GURL MakeGetLatestBlockUrl(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  const std::string path = base::StrCat({base_url.path(), "blocks/latest"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

GURL MakeGetLatestEpochParametersUrl(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  const std::string path =
      base::StrCat({base_url.path(), "epochs/latest/parameters"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

GURL MakeUtxoListUrl(const GURL& base_url, const std::string& address) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path = base::StrCat(
      {base_url.path(),
       base::JoinString({"addresses", base::EscapePath(address), "utxos"},
                        "/")});

  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

GURL MakePostTransactionUrl(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path = base::StrCat({base_url.path(), "tx/submit"});

  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

GURL MakeGetTransactionUrl(const GURL& base_url, std::string_view txid) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    base::JoinString({"txs", base::EscapePath(txid)}, "/")});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

bool ShouldThrottleEndpoint(const GURL& request_url) {
  // Don't throttle requests if host matches brave proxy.
  return !brave_wallet::IsEndpointUsingBraveWalletProxy(request_url);
}

template <class TCallback>
void ReplyWithInvalidJsonError(TCallback callback) {
  std::move(callback).Run(
      base::unexpected(brave_wallet::WalletParsingErrorMessage()));
}

template <class TCallback>
void ReplyWithInternalError(TCallback callback) {
  std::move(callback).Run(
      base::unexpected(brave_wallet::WalletInternalErrorMessage()));
}

std::optional<std::string> ConvertJsonStringToJsonArray(
    const std::string& json) {
  return base::StrCat({"[", json, "]"});
}

std::optional<std::string> HandleGetUtxoListRawResponse(
    const std::string& raw_response) {
  // `404 Not Found` is returned for never transacted address. Covert HTTP error
  // string to a valid json string so APIRequestHelper can handle that.
  if (raw_response == kNotFoundResponse) {
    return "{}";
  }

  return brave_wallet::ConvertAllNumbersToString("", raw_response);
}

}  // namespace

namespace brave_wallet::cardano_rpc {

struct QueuedRequestData {
  std::string method = net::HttpRequestHeaders::kGetMethod;
  std::string payload;
  std::string payload_content_type;
  GURL request_url;
  CardanoRpc::RequestIntermediateCallback callback;
  CardanoRpc::ResponseConversionCallback conversion_callback;
};

CardanoRpc::CardanoRpc(
    const std::string& chain_id,
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : chain_id_(chain_id),
      network_manager_(network_manager),
      api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {
  CHECK(IsCardanoNetwork(chain_id_));
}

CardanoRpc::~CardanoRpc() = default;

void CardanoRpc::GetLatestBlock(GetLatestBlockCallback callback) {
  GURL request_url = MakeGetLatestBlockUrl(GetNetworkURL());
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&CardanoRpc::OnGetLatestBlock,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  DoGetRequestInternal(request_url, std::move(internal_callback),
                       base::BindOnce(&ConvertAllNumbersToString, ""));
}

void CardanoRpc::OnGetLatestBlock(GetLatestBlockCallback callback,
                                  APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto block = Block::FromBlockfrostApiValue(
      blockfrost_api::Block::FromValue(api_request_result.value_body()));
  if (!block) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(std::move(*block)));
}

void CardanoRpc::GetLatestEpochParameters(
    GetLatestEpochParametersCallback callback) {
  GURL request_url = MakeGetLatestEpochParametersUrl(GetNetworkURL());
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&CardanoRpc::OnGetLatestEpochParameters,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  DoGetRequestInternal(request_url, std::move(internal_callback),
                       base::BindOnce(&ConvertAllNumbersToString, ""));
}

void CardanoRpc::OnGetLatestEpochParameters(
    GetLatestEpochParametersCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto epoch_parameters = EpochParameters::FromBlockfrostApiValue(
      blockfrost_api::EpochParameters::FromValue(
          api_request_result.value_body()));
  if (!epoch_parameters) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(std::move(*epoch_parameters)));
}

void CardanoRpc::GetUtxoList(const CardanoAddress& address,
                             GetUtxoListCallback callback) {
  GURL request_url = MakeUtxoListUrl(GetNetworkURL(), address.ToString());
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&CardanoRpc::OnGetUtxoList, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback), address);
  DoGetRequestInternal(request_url, std::move(internal_callback),
                       base::BindOnce(&HandleGetUtxoListRawResponse));
}

void CardanoRpc::OnGetUtxoList(GetUtxoListCallback callback,
                               const CardanoAddress& address,
                               APIRequestResult api_request_result) {
  // Utxo list for never transacted address is returned as 404. This just means
  // an empty utxo list for us.
  if (api_request_result.response_code() == net::HTTP_NOT_FOUND) {
    std::move(callback).Run(base::ok(UnspentOutputs()));
    return;
  }

  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto* items = api_request_result.value_body().GetIfList();
  if (!items) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  UnspentOutputs result;
  for (const auto& item : *items) {
    auto utxo = UnspentOutput::FromBlockfrostApiValue(
        address, blockfrost_api::UnspentOutput::FromValue(item));
    if (!utxo) {
      return ReplyWithInvalidJsonError(std::move(callback));
    }

    result.push_back(std::move(*utxo));
  }

  std::move(callback).Run(base::ok(std::move(result)));
}

void CardanoRpc::PostTransaction(const std::vector<uint8_t>& transaction,
                                 PostTransactionCallback callback) {
  GURL request_url = MakePostTransactionUrl(GetNetworkURL());
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&CardanoRpc::OnPostTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  DoPostRequestInternal(request_url, base::as_string_view(transaction),
                        "application/cbor", std::move(internal_callback),
                        base::BindOnce(&ConvertJsonStringToJsonArray));
}

void CardanoRpc::OnPostTransaction(PostTransactionCallback callback,
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

void CardanoRpc::GetTransaction(std::string_view txid,
                                GetTransactionCallback callback) {
  GURL request_url = MakeGetTransactionUrl(GetNetworkURL(), txid);
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&CardanoRpc::OnGetTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  DoGetRequestInternal(request_url, std::move(internal_callback));
}

void CardanoRpc::OnGetTransaction(GetTransactionCallback callback,
                                  APIRequestResult api_request_result) {
  // Transaction still in mempool is returned as 404.
  if (api_request_result.response_code() == net::HTTP_NOT_FOUND) {
    std::move(callback).Run(base::ok(std::nullopt));
    return;
  }

  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto transaction = Transaction::FromBlockfrostApiValue(
      blockfrost_api::Transaction::FromValue(api_request_result.value_body()));
  if (!transaction) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(std::move(*transaction)));
}

void CardanoRpc::DoGetRequestInternal(
    const GURL& request_url,
    RequestIntermediateCallback callback,
    APIRequestHelper::ResponseConversionCallback
        conversion_callback /* = base::NullCallback() */) {
  DCHECK(request_url.is_valid());

  auto& request = requests_queue_.emplace_back();
  request.method = net::HttpRequestHeaders::kGetMethod;
  request.request_url = request_url;
  request.callback = std::move(callback);
  request.conversion_callback = std::move(conversion_callback);

  MaybeStartQueuedRequest();
}

void CardanoRpc::DoPostRequestInternal(
    const GURL& request_url,
    std::string_view payload,
    std::string_view payload_content_type,
    RequestIntermediateCallback callback,
    APIRequestHelper::ResponseConversionCallback
        conversion_callback /* = base::NullCallback() */) {
  DCHECK(request_url.is_valid());

  auto& request = requests_queue_.emplace_back();
  request.method = net::HttpRequestHeaders::kPostMethod;
  request.payload = payload;
  request.payload_content_type = payload_content_type;
  request.request_url = request_url;
  request.callback = std::move(callback);
  request.conversion_callback = std::move(conversion_callback);

  MaybeStartQueuedRequest();
}

void CardanoRpc::OnRequestInternalDone(RequestIntermediateCallback callback,
                                       APIRequestResult api_request_result) {
  active_requests_--;
  DCHECK_GE(active_requests_, 0u);
  std::move(callback).Run(std::move(api_request_result));

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CardanoRpc::MaybeStartQueuedRequest,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CardanoRpc::MaybeStartQueuedRequest() {
  if (requests_queue_.empty()) {
    return;
  }
  auto rpc_throttle = features::kCardanoRpcThrottle.Get();
  if (ShouldThrottleEndpoint(requests_queue_.front().request_url) &&
      rpc_throttle > 0 &&
      active_requests_ >= static_cast<uint32_t>(rpc_throttle)) {
    return;
  }

  auto request = std::move(requests_queue_.front());
  requests_queue_.pop_front();

  active_requests_++;
  api_request_helper_.Request(
      request.method, request.request_url, request.payload,
      request.payload_content_type,
      base::BindOnce(&CardanoRpc::OnRequestInternalDone,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(request.callback)),
      MakeCardanoRpcHeaders(chain_id_, request.request_url),
      {.auto_retry_on_network_change = true},
      std::move(request.conversion_callback));
}

void CardanoRpc::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  CHECK_IS_TEST();
  api_request_helper_.SetUrlLoaderFactoryForTesting(  // IN-TEST
      std::move(url_loader_factory));
}

GURL CardanoRpc::GetNetworkURL() {
  return network_manager_->GetNetworkURL(chain_id_, mojom::CoinType::ADA);
}

}  // namespace brave_wallet::cardano_rpc
