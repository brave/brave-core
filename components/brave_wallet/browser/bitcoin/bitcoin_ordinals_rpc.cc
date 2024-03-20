/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_ordinals_rpc.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/bitcoin_ordinals_rpc_responses.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "components/grit/brave_components_strings.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("bitcoin_ordinals_rpc", R"(
      semantics {
        sender: "Bitcoin ordinals RPC"
        description:
          "This service is used to communicate with Bitcoin ordinals nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Bitcoin ordinals JSON RPC response bodies."
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

bool UrlPathEndsWithSlash(const GURL& base_url) {
  auto path_piece = base_url.path_piece();
  return !path_piece.empty() && path_piece.back() == '/';
}

GURL GetBaseURL(const std::string& chain_id) {
  GURL switch_url;
  if (chain_id == brave_wallet::mojom::kBitcoinMainnet) {
    switch_url =
        GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            brave_wallet::switches::kBitcoinOrdinalsMainnetRpcUrl));
  } else if (chain_id == brave_wallet::mojom::kBitcoinTestnet) {
    switch_url =
        GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            brave_wallet::switches::kBitcoinOrdinalsTestnetRpcUrl));
  } else {
    NOTREACHED() << chain_id;
  }

  if (switch_url.is_valid()) {
    return switch_url;
  }

  return GURL();
}

const GURL MakeOutpointInfoUrl(const std::string& chain_id,
                               const brave_wallet::BitcoinOutpoint& outpoint) {
  auto base_url = GetBaseURL(chain_id);
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    base::JoinString({"output", outpoint.ToString()}, "/")});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

std::string EndpointHost(const GURL& request_url) {
  DCHECK(request_url.is_valid());
  return request_url.host();
}

bool ShouldThrottleEndpoint(const std::string& endpoint_host) {
  // TODO(apaymyshev): ordinals proxy host
  // Don't throttle requests if host matches brave proxy.
  return EndpointHost(GURL(brave_wallet::kBitcoinMainnetRpcEndpoint)) !=
         endpoint_host;
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

namespace brave_wallet::bitcoin_ordinals_rpc {

struct QueuedRequestData {
  GURL request_url;
  BitcoinOrdinalsRpc::RequestIntermediateCallback callback;
  BitcoinOrdinalsRpc::ResponseConversionCallback conversion_callback;
};

struct EndpointQueue {
  uint32_t active_requests = 0;
  base::circular_deque<QueuedRequestData> requests_queue;
};

BitcoinOrdinalsRpc::BitcoinOrdinalsRpc(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(new APIRequestHelper(GetNetworkTrafficAnnotationTag(),
                                               url_loader_factory)) {}

BitcoinOrdinalsRpc::~BitcoinOrdinalsRpc() = default;

void BitcoinOrdinalsRpc::GetOutpointInfo(const std::string& chain_id,
                                         const BitcoinOutpoint& outpoint,
                                         GetOutpointInfoCallback callback) {
  GURL request_url = MakeOutpointInfoUrl(chain_id, outpoint);
  if (!request_url.is_valid()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto internal_callback =
      base::BindOnce(&BitcoinOrdinalsRpc::OnGetOutpointInfo,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(request_url, std::move(internal_callback));
}

void BitcoinOrdinalsRpc::OnGetOutpointInfo(
    GetOutpointInfoCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return ReplyWithInternalError(std::move(callback));
  }

  auto* dict = api_request_result.value_body().GetIfDict();
  if (!dict) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  auto outpoint_info = OutpointInfo::FromValue(api_request_result.value_body());
  if (!outpoint_info) {
    return ReplyWithInvalidJsonError(std::move(callback));
  }

  std::move(callback).Run(base::ok(std::move(*outpoint_info)));
}

void BitcoinOrdinalsRpc::RequestInternal(
    const GURL& request_url,
    RequestIntermediateCallback callback,
    APIRequestHelper::ResponseConversionCallback conversion_callback) {
  DCHECK(request_url.is_valid());

  auto endpoint_host = EndpointHost(request_url);

  auto& endpoint = endpoints_[endpoint_host];

  auto& request = endpoint.requests_queue.emplace_back();
  request.request_url = request_url;
  request.callback = std::move(callback);
  request.conversion_callback = std::move(conversion_callback);

  MaybeStartQueuedRequest(endpoint_host);
}

void BitcoinOrdinalsRpc::OnRequestInternalDone(
    const std::string& endpoint_host,
    RequestIntermediateCallback callback,
    APIRequestResult api_request_result) {
  auto& endpoint = endpoints_[endpoint_host];
  endpoint.active_requests--;
  DCHECK_GE(endpoint.active_requests, 0u);
  std::move(callback).Run(std::move(api_request_result));

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&BitcoinOrdinalsRpc::MaybeStartQueuedRequest,
                                weak_ptr_factory_.GetWeakPtr(), endpoint_host));
}

void BitcoinOrdinalsRpc::MaybeStartQueuedRequest(
    const std::string& endpoint_host) {
  auto& endpoint = endpoints_[endpoint_host];

  auto rpc_throttle = features::kBitcoinOrdinalsRpcThrottle.Get();
  if (ShouldThrottleEndpoint(endpoint_host) && rpc_throttle > 0 &&
      endpoint.active_requests >= static_cast<uint32_t>(rpc_throttle)) {
    return;
  }
  if (endpoint.requests_queue.empty()) {
    return;
  }

  auto request = std::move(endpoint.requests_queue.front());
  endpoint.requests_queue.pop_front();
  base::flat_map<std::string, std::string> headers;
  headers.emplace("Accept", "application/json");

  endpoint.active_requests++;
  api_request_helper_->Request(
      net::HttpRequestHeaders::kGetMethod, request.request_url, "", "",
      base::BindOnce(&BitcoinOrdinalsRpc::OnRequestInternalDone,
                     weak_ptr_factory_.GetWeakPtr(), endpoint_host,
                     std::move(request.callback)),
      std::move(headers), {.auto_retry_on_network_change = true},
      std::move(request.conversion_callback));
}

void BitcoinOrdinalsRpc::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_->SetUrlLoaderFactoryForTesting(  // IN-TEST
      std::move(url_loader_factory));
}

}  // namespace brave_wallet::bitcoin_ordinals_rpc
