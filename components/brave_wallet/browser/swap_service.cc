/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_service.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("swap_service", R"(
      semantics {
        sender: "Swap Service"
        description:
          "This service is used to obtain 0x price swap quotes and transactions to sign."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Ethereum JSON RPC response bodies."
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

bool IsMainnetNetworkSupported(const std::string& chain_id) {
  return (chain_id == brave_wallet::mojom::kMainnetChainId ||
          chain_id == brave_wallet::mojom::kPolygonMainnetChainId ||
          chain_id == brave_wallet::mojom::kBinanceSmartChainMainnetChainId ||
          chain_id == brave_wallet::mojom::kAvalancheMainnetChainId ||
          chain_id == brave_wallet::mojom::kFantomMainnetChainId ||
          chain_id == brave_wallet::mojom::kCeloMainnetChainId ||
          chain_id == brave_wallet::mojom::kOptimismMainnetChainId);
}

bool IsNetworkSupported(const std::string& chain_id) {
  return (chain_id == brave_wallet::mojom::kRopstenChainId ||
          IsMainnetNetworkSupported(chain_id));
}

GURL AppendSwapParams(const GURL& swap_url,
                      const brave_wallet::mojom::SwapParams& params,
                      const std::string& chain_id) {
  GURL url = swap_url;
  if (!params.taker_address.empty())
    url = net::AppendQueryParameter(url, "takerAddress", params.taker_address);
  if (!params.sell_amount.empty())
    url = net::AppendQueryParameter(url, "sellAmount", params.sell_amount);
  if (!params.buy_amount.empty())
    url = net::AppendQueryParameter(url, "buyAmount", params.buy_amount);
  if (!params.buy_token.empty())
    url = net::AppendQueryParameter(url, "buyToken", params.buy_token);
  if (!params.sell_token.empty())
    url = net::AppendQueryParameter(url, "sellToken", params.sell_token);
  url = net::AppendQueryParameter(url, "buyTokenPercentageFee",
                                  brave_wallet::SwapService::GetFee(chain_id));
  url = net::AppendQueryParameter(
      url, "slippagePercentage",
      base::StringPrintf("%.6f", params.slippage_percentage));
  std::string fee_recipient =
      brave_wallet::SwapService::GetFeeRecipient(chain_id);
  if (!fee_recipient.empty())
    url = net::AppendQueryParameter(url, "feeRecipient", fee_recipient);
  std::string affiliate_address =
      brave_wallet::SwapService::GetAffiliateAddress(chain_id);
  if (!affiliate_address.empty())
    url = net::AppendQueryParameter(url, "affiliateAddress", affiliate_address);
  if (!params.gas_price.empty())
    url = net::AppendQueryParameter(url, "gasPrice", params.gas_price);
  return url;
}

}  // namespace

namespace brave_wallet {

GURL SwapService::base_url_for_test_;

SwapService::SwapService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    JsonRpcService* json_rpc_service)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      json_rpc_service_(json_rpc_service),
      weak_ptr_factory_(this) {
  DCHECK(json_rpc_service_);
}

SwapService::~SwapService() {}

mojo::PendingRemote<mojom::SwapService> SwapService::MakeRemote() {
  mojo::PendingRemote<mojom::SwapService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void SwapService::Bind(mojo::PendingReceiver<mojom::SwapService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SwapService::SetBaseURLForTest(const GURL& base_url_for_test) {
  base_url_for_test_ = base_url_for_test;
}

// static
std::string SwapService::GetFee(const std::string& chain_id) {
  std::string fee;
  if (IsNetworkSupported(chain_id)) {
    fee = brave_wallet::kBuyTokenPercentageFee;
  }

  return fee;
}

// static
std::string SwapService::GetBaseSwapURL(const std::string& chain_id) {
  std::string url;

  if (chain_id == brave_wallet::mojom::kRopstenChainId) {
    url = brave_wallet::kRopstenSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kMainnetChainId) {
    url = brave_wallet::kSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kPolygonMainnetChainId) {
    url = brave_wallet::kPolygonSwapBaseAPIURL;
  } else if (chain_id ==
             brave_wallet::mojom::kBinanceSmartChainMainnetChainId) {
    url = brave_wallet::kBinanceSmartChainSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kAvalancheMainnetChainId) {
    url = brave_wallet::kAvalancheSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kFantomMainnetChainId) {
    url = brave_wallet::kFantomSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kCeloMainnetChainId) {
    url = brave_wallet::kCeloSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kOptimismMainnetChainId) {
    url = brave_wallet::kOptimismSwapBaseAPIURL;
  }

  return url;
}

// static
std::string SwapService::GetFeeRecipient(const std::string& chain_id) {
  std::string feeRecipient;

  // For easy testability on test networks, we use an address different from
  // the production multisig address.
  if (chain_id == brave_wallet::mojom::kRopstenChainId) {
    feeRecipient = brave_wallet::kRopstenFeeRecipient;
  } else if (IsMainnetNetworkSupported(chain_id)) {
    feeRecipient = brave_wallet::kFeeRecipient;
  }

  return feeRecipient;
}

// static
std::string SwapService::GetAffiliateAddress(const std::string& chain_id) {
  std::string affiliateAddress;

  if (IsMainnetNetworkSupported(chain_id)) {
    affiliateAddress = brave_wallet::kAffiliateAddress;
  }

  return affiliateAddress;
}

// static
GURL SwapService::GetPriceQuoteURL(mojom::SwapParamsPtr swap_params,
                                   const std::string& chain_id) {
  std::string spec = base::StringPrintf(
      "%sswap/v1/price", base_url_for_test_.is_empty()
                             ? GetBaseSwapURL(chain_id).c_str()
                             : base_url_for_test_.spec().c_str());
  GURL url(spec);
  url = AppendSwapParams(url, *swap_params, chain_id);
  // That flag prevents an allowance validation on a swap exchange proxy side.
  // We do in clients allowance validation.
  url = net::AppendQueryParameter(url, "skipValidation", "true");

  return url;
}

// static
GURL SwapService::GetTransactionPayloadURL(mojom::SwapParamsPtr swap_params,
                                           const std::string& chain_id) {
  std::string spec = base::StringPrintf(
      "%sswap/v1/quote", base_url_for_test_.is_empty()
                             ? GetBaseSwapURL(chain_id).c_str()
                             : base_url_for_test_.spec().c_str());
  GURL url(spec);
  url = AppendSwapParams(url, *swap_params, chain_id);
  return url;
}

void SwapService::GetPriceQuote(mojom::SwapParamsPtr swap_params,
                                GetPriceQuoteCallback callback) {
  if (!IsNetworkSupported(
          json_rpc_service_->GetChainId(mojom::CoinType::ETH))) {
    std::move(callback).Run(false, nullptr, "UNSUPPORTED_NETWORK");
    return;
  }
  auto internal_callback =
      base::BindOnce(&SwapService::OnGetPriceQuote,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request(
      "GET",
      GetPriceQuoteURL(std::move(swap_params),
                       json_rpc_service_->GetChainId(mojom::CoinType::ETH)),
      "", "", true, std::move(internal_callback));
}

void SwapService::OnGetPriceQuote(
    GetPriceQuoteCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, nullptr, body);
    return;
  }
  auto swap_response = mojom::SwapResponse::New();
  if (!ParseSwapResponse(body, false, &swap_response)) {
    std::move(callback).Run(false, nullptr,
                            "Could not parse response body: " + body);
    return;
  }

  std::move(callback).Run(true, std::move(swap_response), absl::nullopt);
}

void SwapService::GetTransactionPayload(
    mojom::SwapParamsPtr swap_params,
    GetTransactionPayloadCallback callback) {
  if (!IsNetworkSupported(
          json_rpc_service_->GetChainId(mojom::CoinType::ETH))) {
    std::move(callback).Run(false, nullptr, "UNSUPPORTED_NETWORK");
    return;
  }
  auto internal_callback =
      base::BindOnce(&SwapService::OnGetTransactionPayload,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request(
      "GET",
      GetTransactionPayloadURL(
          std::move(swap_params),
          json_rpc_service_->GetChainId(mojom::CoinType::ETH)),
      "", "", true, std::move(internal_callback));
}

void SwapService::OnGetTransactionPayload(
    GetTransactionPayloadCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, nullptr, body);
    return;
  }
  auto swap_response = mojom::SwapResponse::New();
  if (!ParseSwapResponse(body, true, &swap_response)) {
    std::move(callback).Run(false, nullptr,
                            "Could not parse response body: " + body);
    return;
  }

  std::move(callback).Run(true, std::move(swap_response), absl::nullopt);
}

void SwapService::IsSwapSupported(const std::string& chain_id,
                                  IsSwapSupportedCallback callback) {
  std::move(callback).Run(IsNetworkSupported(chain_id));
}

}  // namespace brave_wallet
