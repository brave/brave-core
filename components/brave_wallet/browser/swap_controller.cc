/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_controller.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
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

GURL AppendSwapParams(const GURL& swap_url,
                      const brave_wallet::mojom::SwapParams& params) {
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
  url = net::AppendQueryParameter(
      url, "buyTokenPercentageFee",
      base::StringPrintf("%.6f", params.buy_token_percentage_fee));
  url = net::AppendQueryParameter(
      url, "slippagePercentage",
      base::StringPrintf("%.6f", params.slippage_percentage));
  if (!params.fee_recipient.empty())
    url = net::AppendQueryParameter(url, "feeRecipient", params.fee_recipient);
  if (!params.gas_price.empty())
    url = net::AppendQueryParameter(url, "gasPrice", params.gas_price);
  return url;
}

}  // namespace

namespace brave_wallet {

GURL SwapController::base_url_for_test_;

SwapController::SwapController(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      weak_ptr_factory_(this) {}

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
GURL SwapController::GetPriceQuoteURL(mojom::SwapParamsPtr swap_params) {
  std::string spec = base::StringPrintf(
      "%sswap/v1/price", base_url_for_test_.is_empty()
                             ? kSwapBaseURL
                             : base_url_for_test_.spec().c_str());
  GURL url(spec);
  url = AppendSwapParams(url, *swap_params);
  return url;
}

// static
GURL SwapController::GetTransactionPayloadURL(
    mojom::SwapParamsPtr swap_params) {
  std::string spec = base::StringPrintf(
      "%sswap/v1/quote", base_url_for_test_.is_empty()
                             ? kSwapBaseURL
                             : base_url_for_test_.spec().c_str());
  GURL url(spec);
  url = AppendSwapParams(url, *swap_params);
  return url;
}

void SwapController::GetPriceQuote(mojom::SwapParamsPtr swap_params,
                                   GetPriceQuoteCallback callback) {
  auto internal_callback =
      base::BindOnce(&SwapController::OnGetPriceQuote,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request("GET", GetPriceQuoteURL(std::move(swap_params)),
                              "", "", true, std::move(internal_callback));
}

void SwapController::OnGetPriceQuote(
    GetPriceQuoteCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  auto swap_response = mojom::SwapResponse::New();
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, std::move(swap_response));
    return;
  }
  if (!ParseSwapResponse(body, false, &swap_response)) {
    std::move(callback).Run(false, std::move(swap_response));
    return;
  }

  std::move(callback).Run(true, std::move(swap_response));
}

void SwapController::GetTransactionPayload(
    mojom::SwapParamsPtr swap_params,
    GetTransactionPayloadCallback callback) {
  auto internal_callback =
      base::BindOnce(&SwapController::OnGetTransactionPayload,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request("GET",
                              GetTransactionPayloadURL(std::move(swap_params)),
                              "", "", true, std::move(internal_callback));
}

void SwapController::OnGetTransactionPayload(
    GetTransactionPayloadCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  auto swap_response = mojom::SwapResponse::New();
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, std::move(std::move(swap_response)));
    return;
  }
  if (!ParseSwapResponse(body, true, &swap_response)) {
    std::move(callback).Run(false, std::move(swap_response));
    return;
  }

  std::move(callback).Run(true, std::move(swap_response));
}

}  // namespace brave_wallet
