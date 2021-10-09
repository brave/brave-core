/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_controller.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("swap_controller", R"(
      semantics {
        sender: "Swap Controller"
        description:
          "This controller is used to obtain 0x price swap quotes and transactions to sign."
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

bool IsNetworkSupported(const std::string& chain_id) {
  if (chain_id == brave_wallet::mojom::kRopstenChainId ||
      chain_id == brave_wallet::mojom::kMainnetChainId) {
    return true;
  }

  return false;
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
  url =
      net::AppendQueryParameter(url, "buyTokenPercentageFee",
                                brave_wallet::SwapController::GetFee(chain_id));
  url = net::AppendQueryParameter(
      url, "slippagePercentage",
      base::StringPrintf("%.6f", params.slippage_percentage));
  std::string fee_recipient =
      brave_wallet::SwapController::GetFeeRecipient(chain_id);
  if (!fee_recipient.empty())
    url = net::AppendQueryParameter(url, "feeRecipient", fee_recipient);
  if (!params.gas_price.empty())
    url = net::AppendQueryParameter(url, "gasPrice", params.gas_price);
  return url;
}

}  // namespace

namespace brave_wallet {

GURL SwapController::base_url_for_test_;

SwapController::SwapController(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    EthJsonRpcController* rpc_controller)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      rpc_controller_(rpc_controller),
      weak_ptr_factory_(this) {
  DCHECK(rpc_controller_);
}

SwapController::~SwapController() {}

mojo::PendingRemote<mojom::SwapController> SwapController::MakeRemote() {
  mojo::PendingRemote<mojom::SwapController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void SwapController::Bind(
    mojo::PendingReceiver<mojom::SwapController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SwapController::SetBaseURLForTest(const GURL& base_url_for_test) {
  base_url_for_test_ = base_url_for_test;
}

// static
std::string SwapController::GetFee(const std::string& chain_id) {
  std::string fee;

  if (chain_id == brave_wallet::mojom::kRopstenChainId) {
    fee = brave_wallet::kRopstenBuyTokenPercentageFee;
  } else if (chain_id == brave_wallet::mojom::kMainnetChainId) {
    fee = brave_wallet::kBuyTokenPercentageFee;
  }

  return fee;
}

// static
std::string SwapController::GetBaseSwapURL(const std::string& chain_id) {
  std::string url;

  if (chain_id == brave_wallet::mojom::kRopstenChainId) {
    url = brave_wallet::kRopstenSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kMainnetChainId) {
    url = brave_wallet::kSwapBaseAPIURL;
  }

  return url;
}

// static
std::string SwapController::GetFeeRecipient(const std::string& chain_id) {
  std::string feeRecipient;

  if (chain_id == brave_wallet::mojom::kRopstenChainId) {
    feeRecipient = brave_wallet::kRopstenFeeRecipient;
  } else if (chain_id == brave_wallet::mojom::kMainnetChainId) {
    feeRecipient = brave_wallet::kFeeRecipient;
  }

  return feeRecipient;
}

// static
GURL SwapController::GetPriceQuoteURL(mojom::SwapParamsPtr swap_params,
                                      const std::string& chain_id) {
  std::string spec = base::StringPrintf(
      "%sswap/v1/price", base_url_for_test_.is_empty()
                             ? GetBaseSwapURL(chain_id).c_str()
                             : base_url_for_test_.spec().c_str());
  GURL url(spec);
  url = AppendSwapParams(url, *swap_params, chain_id);
  return url;
}

// static
GURL SwapController::GetTransactionPayloadURL(mojom::SwapParamsPtr swap_params,
                                              const std::string& chain_id) {
  std::string spec = base::StringPrintf(
      "%sswap/v1/quote", base_url_for_test_.is_empty()
                             ? GetBaseSwapURL(chain_id).c_str()
                             : base_url_for_test_.spec().c_str());
  GURL url(spec);
  url = AppendSwapParams(url, *swap_params, chain_id);
  return url;
}

void SwapController::GetPriceQuote(mojom::SwapParamsPtr swap_params,
                                   GetPriceQuoteCallback callback) {
  if (!IsNetworkSupported(rpc_controller_->GetChainId())) {
    std::move(callback).Run(false, nullptr, "UNSUPPORTED_NETWORK");
    return;
  }
  auto internal_callback =
      base::BindOnce(&SwapController::OnGetPriceQuote,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request(
      "GET",
      GetPriceQuoteURL(std::move(swap_params), rpc_controller_->GetChainId()),
      "", "", true, std::move(internal_callback));
}

void SwapController::OnGetPriceQuote(
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

void SwapController::GetTransactionPayload(
    mojom::SwapParamsPtr swap_params,
    GetTransactionPayloadCallback callback) {
  if (!IsNetworkSupported(rpc_controller_->GetChainId())) {
    std::move(callback).Run(false, nullptr, "UNSUPPORTED_NETWORK");
    return;
  }
  auto internal_callback =
      base::BindOnce(&SwapController::OnGetTransactionPayload,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request(
      "GET",
      GetTransactionPayloadURL(std::move(swap_params),
                               rpc_controller_->GetChainId()),
      "", "", true, std::move(internal_callback));
}

void SwapController::OnGetTransactionPayload(
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

}  // namespace brave_wallet
