/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_rpc.h"

#include <algorithm>
#include <optional>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_responses.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/bech32.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

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

constexpr char kProjectIdHeader[] = "project_id";
base::flat_map<std::string, std::string> MakeCardanoRpcHeaders() {
  std::string cardano_project_id =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          brave_wallet::switches::kCardanoProjectId);

  base::flat_map<std::string, std::string> result;
  if (!cardano_project_id.empty()) {
    result.emplace(kProjectIdHeader, cardano_project_id);
  }

  return result;
}

bool UrlPathEndsWithSlash(const GURL& base_url) {
  auto path_piece = base_url.path_piece();
  return !path_piece.empty() && path_piece.back() == '/';
}

GURL MakeGetChainHeightUrl(const GURL& base_url) {
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

GURL EndpointHost(const GURL& request_url) {
  DCHECK(request_url.is_valid());
  return request_url.GetWithEmptyPath();
}

bool ShouldThrottleEndpoint(const GURL& endpoint_host) {
  // Don't throttle requests if host matches brave proxy.
  return !brave_wallet::IsEndpointUsingBraveWalletProxy(endpoint_host);
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

}  // namespace

namespace brave_wallet::cardano_rpc {

struct QueuedRequestData {
  GURL request_url;
  CardanoRpc::RequestIntermediateCallback callback;
  CardanoRpc::ResponseConversionCallback conversion_callback;
};

struct EndpointQueue {
  uint32_t active_requests = 0;
  base::circular_deque<QueuedRequestData> requests_queue;
};

CardanoRpc::CardanoRpc(
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : network_manager_(network_manager),
      api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

CardanoRpc::~CardanoRpc() = default;

void CardanoRpc::GetChainHeight(const std::string& chain_id,
                                GetChainHeightCallback callback) {
  GURL request_url = MakeGetChainHeightUrl(GetNetworkURL(chain_id));
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&CardanoRpc::OnGetChainHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString, "");
  RequestInternal(request_url, std::move(internal_callback),
                  std::move(conversion_callback));
}

void CardanoRpc::OnGetChainHeight(GetChainHeightCallback callback,
                                  APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto* dict = api_request_result.value_body().GetIfDict();
  if (!dict) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  auto block = Block::FromValue(*dict);
  if (!block) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  uint32_t height = 0;
  if (!base::StringToUint(block->height, &height)) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(height));
}

void CardanoRpc::GetUtxoList(const std::string& chain_id,
                             const std::string& address,
                             GetUtxoListCallback callback) {
  DCHECK(bech32::Decode(address)) << address;
  GURL request_url = MakeUtxoListUrl(GetNetworkURL(chain_id), address);
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&CardanoRpc::OnGetUtxoList, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback), address);
  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString, "");
  RequestInternal(request_url, std::move(internal_callback),
                  std::move(conversion_callback));
}

void CardanoRpc::OnGetUtxoList(GetUtxoListCallback callback,
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

void CardanoRpc::RequestInternal(
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

void CardanoRpc::OnRequestInternalDone(const GURL& endpoint_host,
                                       RequestIntermediateCallback callback,
                                       APIRequestResult api_request_result) {
  auto& endpoint = endpoints_[endpoint_host.host()];
  endpoint.active_requests--;
  DCHECK_GE(endpoint.active_requests, 0u);
  std::move(callback).Run(std::move(api_request_result));

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CardanoRpc::MaybeStartQueuedRequest,
                                weak_ptr_factory_.GetWeakPtr(), endpoint_host));
}

void CardanoRpc::MaybeStartQueuedRequest(const GURL& endpoint_host) {
  auto& endpoint = endpoints_[endpoint_host.host()];

  auto rpc_throttle = features::kCardanoRpcThrottle.Get();
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
  api_request_helper_.Request(
      net::HttpRequestHeaders::kGetMethod, request.request_url, "", "",
      base::BindOnce(&CardanoRpc::OnRequestInternalDone,
                     weak_ptr_factory_.GetWeakPtr(), endpoint_host,
                     std::move(request.callback)),
      MakeCardanoRpcHeaders(), {.auto_retry_on_network_change = true},
      std::move(request.conversion_callback));
}

void CardanoRpc::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  CHECK_IS_TEST();
  api_request_helper_.SetUrlLoaderFactoryForTesting(  // IN-TEST
      std::move(url_loader_factory));
}

GURL CardanoRpc::GetNetworkURL(const std::string& chain_id) {
  return network_manager_->GetNetworkURL(chain_id, mojom::CoinType::ADA);
}

}  // namespace brave_wallet::cardano_rpc
