/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_service.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/swap_request_helper.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("swap_service", R"(
      semantics {
        sender: "Swap Service"
        description:
          "This service is used to obtain swap price quotes and transactions to sign."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "0x and Jupiter API response bodies."
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

bool IsEVMNetworkSupported(const std::string& chain_id) {
  return (chain_id == brave_wallet::mojom::kGoerliChainId ||
          chain_id == brave_wallet::mojom::kMainnetChainId ||
          chain_id == brave_wallet::mojom::kPolygonMainnetChainId ||
          chain_id == brave_wallet::mojom::kBinanceSmartChainMainnetChainId ||
          chain_id == brave_wallet::mojom::kAvalancheMainnetChainId ||
          chain_id == brave_wallet::mojom::kFantomMainnetChainId ||
          chain_id == brave_wallet::mojom::kCeloMainnetChainId ||
          chain_id == brave_wallet::mojom::kOptimismMainnetChainId ||
          chain_id == brave_wallet::mojom::kArbitrumMainnetChainId ||
          chain_id == brave_wallet::mojom::kBaseMainnetChainId);
}

bool IsSolanaNetworkSupported(const std::string& chain_id) {
  return chain_id == brave_wallet::mojom::kSolanaMainnet;
}

bool HasRFQTLiquidity(const std::string& chain_id) {
  return (chain_id == brave_wallet::mojom::kMainnetChainId ||
          chain_id == brave_wallet::mojom::kPolygonMainnetChainId);
}

std::string GetFee(const std::string& chain_id) {
  if (IsEVMNetworkSupported(chain_id)) {
    return brave_wallet::kBuyTokenPercentageFee;
  } else if (IsSolanaNetworkSupported(chain_id)) {
    return brave_wallet::kSolanaBuyTokenFeeBps;
  }

  return "";
}

std::string GetFeeRecipient(const std::string& chain_id) {
  if (IsEVMNetworkSupported(chain_id)) {
    return brave_wallet::kEVMFeeRecipient;
  } else if (IsSolanaNetworkSupported(chain_id)) {
    return brave_wallet::kSolanaFeeRecipient;
  }

  return "";
}

std::string GetAffiliateAddress(const std::string& chain_id) {
  if (IsEVMNetworkSupported(chain_id)) {
    return brave_wallet::kAffiliateAddress;
  }

  return "";
}

GURL Append0xSwapParams(const GURL& swap_url,
                        const brave_wallet::mojom::SwapParams& params,
                        const std::string& chain_id) {
  GURL url = swap_url;
  if (!params.taker_address.empty()) {
    url = net::AppendQueryParameter(url, "takerAddress", params.taker_address);
  }
  if (!params.sell_amount.empty()) {
    url = net::AppendQueryParameter(url, "sellAmount", params.sell_amount);
  }
  if (!params.buy_amount.empty()) {
    url = net::AppendQueryParameter(url, "buyAmount", params.buy_amount);
  }
  if (!params.buy_token.empty()) {
    url = net::AppendQueryParameter(url, "buyToken", params.buy_token);
  }
  if (!params.sell_token.empty()) {
    url = net::AppendQueryParameter(url, "sellToken", params.sell_token);
  }
  url =
      net::AppendQueryParameter(url, "buyTokenPercentageFee", GetFee(chain_id));
  url = net::AppendQueryParameter(
      url, "slippagePercentage",
      base::StringPrintf("%.6f", params.slippage_percentage));
  std::string fee_recipient = GetFeeRecipient(chain_id);
  if (!fee_recipient.empty()) {
    url = net::AppendQueryParameter(url, "feeRecipient", fee_recipient);
  }
  std::string affiliate_address = GetAffiliateAddress(chain_id);
  if (!affiliate_address.empty()) {
    url = net::AppendQueryParameter(url, "affiliateAddress", affiliate_address);
  }
  if (!params.gas_price.empty()) {
    url = net::AppendQueryParameter(url, "gasPrice", params.gas_price);
  }
  return url;
}

GURL AppendJupiterQuoteParams(
    const GURL& swap_url,
    const brave_wallet::mojom::JupiterQuoteParams& params,
    const std::string& chain_id) {
  GURL url = swap_url;
  if (!params.input_mint.empty()) {
    url = net::AppendQueryParameter(url, "inputMint", params.input_mint);
  }

  if (!params.output_mint.empty()) {
    url = net::AppendQueryParameter(url, "outputMint", params.output_mint);
  }

  if (!params.amount.empty()) {
    url = net::AppendQueryParameter(url, "amount", params.amount);
  }

  url = net::AppendQueryParameter(url, "swapMode", "ExactIn");

  url = net::AppendQueryParameter(
      url, "slippageBps", base::StringPrintf("%d", params.slippage_bps));

  if (brave_wallet::HasJupiterFeesForTokenMint(params.output_mint)) {
    url = net::AppendQueryParameter(url, "feeBps", GetFee(chain_id));
  }

  // TODO(onyb): append userPublicKey to get information on fees and ATA
  // deposits.

  return url;
}

base::flat_map<std::string, std::string> Get0xAPIHeaders() {
  std::string brave_zero_ex_api_key(BUILDFLAG(BRAVE_ZERO_EX_API_KEY));
  if (brave_zero_ex_api_key.empty()) {
    return {};
  }
  return {{brave_wallet::k0xAPIKeyHeader, std::move(brave_zero_ex_api_key)}};
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

SwapService::~SwapService() = default;

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
std::string SwapService::GetBaseSwapURL(const std::string& chain_id) {
  if (!base_url_for_test_.is_empty()) {
    return base_url_for_test_.spec();
  }

  if (chain_id == brave_wallet::mojom::kGoerliChainId) {
    return brave_wallet::kGoerliSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kMainnetChainId) {
    return brave_wallet::kSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kPolygonMainnetChainId) {
    return brave_wallet::kPolygonSwapBaseAPIURL;
  } else if (chain_id ==
             brave_wallet::mojom::kBinanceSmartChainMainnetChainId) {
    return brave_wallet::kBinanceSmartChainSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kAvalancheMainnetChainId) {
    return brave_wallet::kAvalancheSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kFantomMainnetChainId) {
    return brave_wallet::kFantomSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kCeloMainnetChainId) {
    return brave_wallet::kCeloSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kOptimismMainnetChainId) {
    return brave_wallet::kOptimismSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kArbitrumMainnetChainId) {
    return brave_wallet::kArbitrumSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kSolanaMainnet) {
    return brave_wallet::kSolanaSwapBaseAPIURL;
  } else if (chain_id == brave_wallet::mojom::kBaseMainnetChainId) {
    return brave_wallet::kBaseSwapBaseAPIURL;
  }

  return "";
}

// static
GURL SwapService::GetPriceQuoteURL(mojom::SwapParamsPtr swap_params,
                                   const std::string& chain_id) {
  const bool use_rfqt = HasRFQTLiquidity(chain_id);

  // If chain has RFQ-T liquidity available, use the /quote endpoint for
  // fetching the indicative price, since /price is often inaccurate. This is
  // discouraged in 0x docs, particularly for RFQ-T trades, since it locks up
  // the capital of market makers. However, the 0x team has approved us to do
  // this, noting the inability of /price to discover optimal RFQ quotes. This
  // should be considered a temporary workaround until 0x comes up with a
  // solution.
  std::string spec =
      base::StringPrintf(use_rfqt ? "%sswap/v1/quote" : "%sswap/v1/price",
                         GetBaseSwapURL(chain_id).c_str());
  GURL url(spec);
  url = Append0xSwapParams(url, *swap_params, chain_id);
  // That flag prevents an allowance validation by the 0x router. Disable it
  // here and perform the validation on the client side.
  url = net::AppendQueryParameter(url, "skipValidation", "true");

  if (use_rfqt) {
    url = net::AppendQueryParameter(url, "intentOnFilling", "false");
  }

  return url;
}

// static
GURL SwapService::GetTransactionPayloadURL(mojom::SwapParamsPtr swap_params,
                                           const std::string& chain_id) {
  std::string spec =
      base::StringPrintf("%sswap/v1/quote", GetBaseSwapURL(chain_id).c_str());
  GURL url(spec);
  url = Append0xSwapParams(url, *swap_params, chain_id);

  if (HasRFQTLiquidity(chain_id)) {
    url = net::AppendQueryParameter(url, "intentOnFilling", "true");
  }

  return url;
}

// static
GURL SwapService::GetJupiterQuoteURL(mojom::JupiterQuoteParamsPtr params,
                                     const std::string& chain_id) {
  DCHECK(params);

  std::string spec =
      base::StringPrintf("%sv4/quote", GetBaseSwapURL(chain_id).c_str());
  GURL url(spec);
  url = AppendJupiterQuoteParams(url, *params, chain_id);

  return url;
}

// static
GURL SwapService::GetJupiterSwapTransactionsURL(const std::string& chain_id) {
  std::string spec =
      base::StringPrintf("%sv4/swap", GetBaseSwapURL(chain_id).c_str());
  GURL url(spec);
  return url;
}

void SwapService::IsSwapSupported(const std::string& chain_id,
                                  IsSwapSupportedCallback callback) {
  std::move(callback).Run(IsEVMNetworkSupported(chain_id) ||
                          IsSolanaNetworkSupported(chain_id));
}

void SwapService::GetPriceQuote(mojom::SwapParamsPtr swap_params,
                                GetPriceQuoteCallback callback) {
  if (!IsEVMNetworkSupported(json_rpc_service_->GetChainIdSync(
          mojom::CoinType::ETH, absl::nullopt))) {
    std::move(callback).Run(
        nullptr, nullptr,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
    return;
  }
  auto internal_callback =
      base::BindOnce(&SwapService::OnGetPriceQuote,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_.Request(
      net::HttpRequestHeaders::kGetMethod,
      GetPriceQuoteURL(std::move(swap_params),
                       json_rpc_service_->GetChainIdSync(mojom::CoinType::ETH,
                                                         absl::nullopt)),
      "", "", std::move(internal_callback), Get0xAPIHeaders(),
      {.auto_retry_on_network_change = true});
}

void SwapService::OnGetPriceQuote(GetPriceQuoteCallback callback,
                                  APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto swap_error_response =
            ParseSwapErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(nullptr, std::move(swap_error_response), "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  if (auto swap_response =
          ParseSwapResponse(api_request_result.value_body(), false)) {
    std::move(callback).Run(std::move(swap_response), nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::GetTransactionPayload(
    mojom::SwapParamsPtr swap_params,
    GetTransactionPayloadCallback callback) {
  if (!IsEVMNetworkSupported(json_rpc_service_->GetChainIdSync(
          mojom::CoinType::ETH, absl::nullopt))) {
    std::move(callback).Run(
        nullptr, nullptr,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
    return;
  }
  auto internal_callback =
      base::BindOnce(&SwapService::OnGetTransactionPayload,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_.Request(
      net::HttpRequestHeaders::kGetMethod,
      GetTransactionPayloadURL(std::move(swap_params),
                               json_rpc_service_->GetChainIdSync(
                                   mojom::CoinType::ETH, absl::nullopt)),
      "", "", std::move(internal_callback), Get0xAPIHeaders(),
      {.auto_retry_on_network_change = true});
}

void SwapService::OnGetTransactionPayload(
    GetTransactionPayloadCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto swap_error_response =
            ParseSwapErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(nullptr, std::move(swap_error_response), "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }

    return;
  }

  if (auto swap_response =
          ParseSwapResponse(api_request_result.value_body(), true)) {
    std::move(callback).Run(std::move(swap_response), nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::GetJupiterQuote(mojom::JupiterQuoteParamsPtr params,
                                  GetJupiterQuoteCallback callback) {
  if (!IsSolanaNetworkSupported(json_rpc_service_->GetChainIdSync(
          mojom::CoinType::SOL, absl::nullopt))) {
    std::move(callback).Run(
        nullptr, nullptr,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
    return;
  }

  auto internal_callback =
      base::BindOnce(&SwapService::OnGetJupiterQuote,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString);

  base::flat_map<std::string, std::string> request_headers;
  api_request_helper_.Request(
      net::HttpRequestHeaders::kGetMethod,
      GetJupiterQuoteURL(std::move(params),
                         json_rpc_service_->GetChainIdSync(mojom::CoinType::SOL,
                                                           absl::nullopt)),
      "", "", std::move(internal_callback), request_headers,
      {.auto_retry_on_network_change = true}, std::move(conversion_callback));
}

void SwapService::OnGetJupiterQuote(GetJupiterQuoteCallback callback,
                                    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            ParseJupiterErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(nullptr, std::move(error_response), "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  if (auto swap_quote = ParseJupiterQuote(api_request_result.value_body())) {
    std::move(callback).Run(std::move(swap_quote), nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::GetJupiterSwapTransactions(
    mojom::JupiterSwapParamsPtr params,
    GetJupiterSwapTransactionsCallback callback) {
  if (!IsSolanaNetworkSupported(json_rpc_service_->GetChainIdSync(
          mojom::CoinType::SOL, absl::nullopt))) {
    std::move(callback).Run(
        nullptr, nullptr,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
    return;
  }

  auto encoded_params = EncodeJupiterTransactionParams(std::move(params));
  if (!encoded_params) {
    std::move(callback).Run(
        nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&SwapService::OnGetJupiterSwapTransactions,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_.Request(
      "POST",
      GetJupiterSwapTransactionsURL(json_rpc_service_->GetChainIdSync(
          mojom::CoinType::SOL, absl::nullopt)),
      *encoded_params, "application/json", std::move(internal_callback), {},
      {.auto_retry_on_network_change = true});
}

void SwapService::OnGetJupiterSwapTransactions(
    GetJupiterSwapTransactionsCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            ParseJupiterErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(nullptr, std::move(error_response), "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }

    return;
  }

  if (auto swap_transactions =
          ParseJupiterSwapTransactions(api_request_result.value_body())) {
    std::move(callback).Run(std::move(swap_transactions), nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::HasJupiterFeesForTokenMint(
    const std::string& mint,
    HasJupiterFeesForTokenMintCallback callback) {
  std::move(callback).Run(brave_wallet::HasJupiterFeesForTokenMint(mint));
}

}  // namespace brave_wallet
