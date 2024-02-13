/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_service.h"

#include <optional>
#include <utility>

#include "base/strings/string_number_conversions.h"
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

std::optional<double> GetFeePercentage(const std::string& chain_id) {
  if (IsEVMNetworkSupported(chain_id)) {
    return brave_wallet::k0xBuyTokenFeePercentage;
  }

  if (IsSolanaNetworkSupported(chain_id)) {
    return brave_wallet::kSolanaBuyTokenFeePercentage;
  }

  return std::nullopt;
}

std::string GetAffiliateAddress(const std::string& chain_id) {
  if (IsEVMNetworkSupported(chain_id)) {
    return brave_wallet::kAffiliateAddress;
  }

  return "";
}

brave_wallet::mojom::BraveSwapFeeResponsePtr GetBraveFeeInternal(
    const brave_wallet::mojom::BraveSwapFeeParamsPtr& params) {
  const auto& percentage_fee = GetFeePercentage(params->chain_id);
  if (!percentage_fee) {
    return nullptr;
  }

  if (IsSolanaNetworkSupported(params->chain_id)) {
    brave_wallet::mojom::BraveSwapFeeResponsePtr response =
        brave_wallet::mojom::BraveSwapFeeResponse::New();

    bool has_fees =
        brave_wallet::HasJupiterFeesForTokenMint(params->output_token);

    // Jupiter API v6 will take 20% of the platform fee charged by integrators.
    // TODO(onyb): update the multipliers below during migration to Jupiter API
    // v6.
    auto protocol_fee_pct = percentage_fee.value() * 0.0;
    auto brave_fee_pct = percentage_fee.value() * 1.0;
    auto discount_on_brave_fee_pct = has_fees ? 0.0 : 100.0;
    auto discount_on_prototcol_fee_pct = has_fees ? 0.0 : 100.0;

    response->brave_fee_pct = base::NumberToString(brave_fee_pct);

    response->discount_on_brave_fee_pct =
        base::NumberToString(discount_on_brave_fee_pct);

    response->protocol_fee_pct = base::NumberToString(
        (100.0 - discount_on_prototcol_fee_pct) / 100.0 * protocol_fee_pct);

    response->effective_fee_pct = base::NumberToString(
        (100.0 - discount_on_brave_fee_pct) / 100.0 * brave_fee_pct);

    // Jupiter swap fee is specified in basis points
    response->fee_param =
        base::NumberToString(static_cast<int>(percentage_fee.value() * 100.0));

    response->discount_code =
        has_fees ? brave_wallet::mojom::DiscountCode::kNone
                 : brave_wallet::mojom::DiscountCode::kUnknownJupiterOutputMint;

    response->has_brave_fee = has_fees;

    return response;
  }

  if (IsEVMNetworkSupported(params->chain_id)) {
    brave_wallet::mojom::BraveSwapFeeResponsePtr response =
        brave_wallet::mojom::BraveSwapFeeResponse::New();

    // We currently do not offer discounts on 0x Brave fees.
    auto discount_on_brave_fee_pct = 0.0;

    // This indicates the 0x Swap fee of 15 bps on select tokens. It should
    // only be surfaced to the users if quote has a non-null zeroExFee field.
    response->protocol_fee_pct =
        base::NumberToString(brave_wallet::k0xProtocolFeePercentage);
    response->brave_fee_pct = base::NumberToString(percentage_fee.value());
    response->discount_on_brave_fee_pct =
        base::NumberToString(discount_on_brave_fee_pct);

    auto effective_fee_pct =
        (100.0 - discount_on_brave_fee_pct) / 100.0 * percentage_fee.value();
    response->effective_fee_pct = base::NumberToString(effective_fee_pct);

    // 0x swap fee is specified as a multiplier
    response->fee_param = base::NumberToString(effective_fee_pct / 100.0);

    response->discount_code = brave_wallet::mojom::DiscountCode::kNone;
    response->has_brave_fee = effective_fee_pct > 0.0;

    return response;
  }

  return nullptr;
}

GURL Append0xSwapParams(const GURL& swap_url,
                        const brave_wallet::mojom::SwapQuoteParams& params) {
  GURL url = swap_url;
  if (!params.from_account_id->address.empty()) {
    url = net::AppendQueryParameter(url, "takerAddress",
                                    params.from_account_id->address);
  }
  if (!params.from_amount.empty()) {
    url = net::AppendQueryParameter(url, "sellAmount", params.from_amount);
  }
  if (!params.to_amount.empty()) {
    url = net::AppendQueryParameter(url, "buyAmount", params.to_amount);
  }
  if (!params.to_token.empty()) {
    url = net::AppendQueryParameter(url, "buyToken", params.to_token);
  }
  if (!params.from_token.empty()) {
    url = net::AppendQueryParameter(url, "sellToken", params.from_token);
  }

  auto brave_swap_fee_params = brave_wallet::mojom::BraveSwapFeeParams::New();
  brave_swap_fee_params->chain_id = params.from_chain_id;
  brave_swap_fee_params->taker = params.from_account_id->address;
  brave_swap_fee_params->input_token = params.from_token;
  brave_swap_fee_params->output_token = params.to_token;
  const auto& brave_fee = GetBraveFeeInternal(brave_swap_fee_params);
  if (brave_fee && brave_fee->has_brave_fee) {
    url = net::AppendQueryParameter(url, "buyTokenPercentageFee",
                                    brave_fee->fee_param);
  }

  double slippage_percentage = 0.0;
  if (base::StringToDouble(params.slippage_percentage, &slippage_percentage)) {
    url = net::AppendQueryParameter(
        url, "slippagePercentage",
        base::StringPrintf("%.6f", slippage_percentage / 100));
  }

  url = net::AppendQueryParameter(url, "feeRecipient",
                                  brave_wallet::kEVMFeeRecipient);

  std::string affiliate_address = GetAffiliateAddress(params.from_chain_id);
  if (!affiliate_address.empty()) {
    url = net::AppendQueryParameter(url, "affiliateAddress", affiliate_address);
  }
  // TODO(onyb): custom gas_price is currently unused and may be removed in
  // future.
  // if (!params.gas_price.empty()) {
  // url = net::AppendQueryParameter(url, "gasPrice", params.gas_price);
  // }
  return url;
}

GURL AppendJupiterQuoteParams(
    const GURL& swap_url,
    const brave_wallet::mojom::SwapQuoteParams& params) {
  GURL url = swap_url;
  if (!params.from_token.empty()) {
    url = net::AppendQueryParameter(url, "inputMint", params.from_token);
  }

  if (!params.to_token.empty()) {
    url = net::AppendQueryParameter(url, "outputMint", params.to_token);
  }

  if (!params.from_amount.empty()) {
    url = net::AppendQueryParameter(url, "amount", params.from_amount);
  }

  url = net::AppendQueryParameter(url, "swapMode", "ExactIn");

  double slippage_percentage = 0.0;
  if (base::StringToDouble(params.slippage_percentage, &slippage_percentage)) {
    url = net::AppendQueryParameter(
        url, "slippageBps",
        base::StringPrintf("%d", static_cast<int>(slippage_percentage * 100)));
  }

  auto brave_swap_fee_params = brave_wallet::mojom::BraveSwapFeeParams::New();
  brave_swap_fee_params->chain_id = params.from_chain_id;
  brave_swap_fee_params->taker = params.from_account_id->address;
  brave_swap_fee_params->input_token = params.from_token;
  brave_swap_fee_params->output_token = params.to_token;
  const auto& brave_fee = GetBraveFeeInternal(brave_swap_fee_params);
  if (brave_fee && brave_fee->has_brave_fee) {
    url =
        net::AppendQueryParameter(url, "platformFeeBps", brave_fee->fee_param);
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
GURL SwapService::GetZeroExQuoteURL(const mojom::SwapQuoteParams& params) {
  const bool use_rfqt = HasRFQTLiquidity(params.from_chain_id);

  // If chain has RFQ-T liquidity available, use the /quote endpoint for
  // fetching the indicative price, since /price is often inaccurate. This is
  // discouraged in 0x docs, particularly for RFQ-T trades, since it locks up
  // the capital of market makers. However, the 0x team has approved us to do
  // this, noting the inability of /price to discover optimal RFQ quotes. This
  // should be considered a temporary workaround until 0x comes up with a
  // solution.
  std::string spec =
      base::StringPrintf(use_rfqt ? "%sswap/v1/quote" : "%sswap/v1/price",
                         GetBaseSwapURL(params.from_chain_id).c_str());
  GURL url(spec);
  url = Append0xSwapParams(url, params);
  // That flag prevents an allowance validation by the 0x router. Disable it
  // here and perform the validation on the client side.
  url = net::AppendQueryParameter(url, "skipValidation", "true");

  if (use_rfqt) {
    url = net::AppendQueryParameter(url, "intentOnFilling", "false");
  }

  return url;
}

// static
GURL SwapService::GetZeroExTransactionURL(
    const mojom::SwapQuoteParams& params) {
  std::string spec = base::StringPrintf(
      "%sswap/v1/quote", GetBaseSwapURL(params.from_chain_id).c_str());
  GURL url(spec);
  url = Append0xSwapParams(url, params);

  if (HasRFQTLiquidity(params.from_chain_id)) {
    url = net::AppendQueryParameter(url, "intentOnFilling", "true");
  }

  return url;
}

// static
GURL SwapService::GetJupiterQuoteURL(const mojom::SwapQuoteParams& params) {
  std::string spec = base::StringPrintf(
      "%sv6/quote", GetBaseSwapURL(params.from_chain_id).c_str());
  GURL url(spec);
  url = AppendJupiterQuoteParams(url, params);

  return url;
}

// static
GURL SwapService::GetJupiterTransactionURL(const std::string& chain_id) {
  std::string spec =
      base::StringPrintf("%sv6/swap", GetBaseSwapURL(chain_id).c_str());
  GURL url(spec);
  return url;
}

void SwapService::IsSwapSupported(const std::string& chain_id,
                                  IsSwapSupportedCallback callback) {
  std::move(callback).Run(IsEVMNetworkSupported(chain_id) ||
                          IsSolanaNetworkSupported(chain_id));
}

void SwapService::GetQuote(mojom::SwapQuoteParamsPtr params,
                           GetQuoteCallback callback) {
  // Cross-chain swaps are not supported.
  if (params->from_chain_id != params->to_chain_id) {
    std::move(callback).Run(
        nullptr, nullptr,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
    return;
  }

  if (IsEVMNetworkSupported(params->from_chain_id)) {
    auto internal_callback =
        base::BindOnce(&SwapService::OnGetZeroExQuote,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));

    api_request_helper_.Request(net::HttpRequestHeaders::kGetMethod,
                                GetZeroExQuoteURL(*params), "", "",
                                std::move(internal_callback), Get0xAPIHeaders(),
                                {.auto_retry_on_network_change = true});

    return;
  }

  if (IsSolanaNetworkSupported(params->from_chain_id)) {
    auto internal_callback =
        base::BindOnce(&SwapService::OnGetJupiterQuote,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));

    auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString);

    base::flat_map<std::string, std::string> request_headers;
    api_request_helper_.Request(
        net::HttpRequestHeaders::kGetMethod, GetJupiterQuoteURL(*params), "",
        "", std::move(internal_callback), request_headers,
        {.auto_retry_on_network_change = true}, std::move(conversion_callback));

    return;
  }

  std::move(callback).Run(
      nullptr, nullptr,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
}

void SwapService::OnGetZeroExQuote(GetQuoteCallback callback,
                                   APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto swap_error_response =
            ParseZeroExErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr,
          mojom::SwapErrorUnion::NewZeroExError(std::move(swap_error_response)),
          "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  if (auto swap_response =
          ParseZeroExQuoteResponse(api_request_result.value_body(), false)) {
    std::move(callback).Run(
        mojom::SwapQuoteUnion::NewZeroExQuote(std::move(swap_response)),
        nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::OnGetJupiterQuote(GetQuoteCallback callback,
                                    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            ParseJupiterErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr,
          mojom::SwapErrorUnion::NewJupiterError(std::move(error_response)),
          "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  if (auto swap_quote =
          ParseJupiterQuoteResponse(api_request_result.value_body())) {
    std::move(callback).Run(
        mojom::SwapQuoteUnion::NewJupiterQuote(std::move(swap_quote)), nullptr,
        "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::GetTransaction(mojom::SwapTransactionParamsUnionPtr params,
                                 GetTransactionCallback callback) {
  if (params->is_zero_ex_transaction_params()) {
    auto internal_callback =
        base::BindOnce(&SwapService::OnGetZeroExTransaction,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));

    api_request_helper_.Request(
        net::HttpRequestHeaders::kGetMethod,
        GetZeroExTransactionURL(*params->get_zero_ex_transaction_params()), "",
        "", std::move(internal_callback), Get0xAPIHeaders(),
        {.auto_retry_on_network_change = true});

    return;
  }

  if (params->is_jupiter_transaction_params()) {
    auto& jupiter_transaction_params = params->get_jupiter_transaction_params();

    auto brave_swap_fee_params = brave_wallet::mojom::BraveSwapFeeParams::New();
    brave_swap_fee_params->chain_id = jupiter_transaction_params->chain_id;
    brave_swap_fee_params->taker = jupiter_transaction_params->user_public_key;
    brave_swap_fee_params->input_token =
        jupiter_transaction_params->quote->input_mint;
    brave_swap_fee_params->output_token =
        jupiter_transaction_params->quote->output_mint;
    const auto& brave_fee =
        GetBraveFeeInternal(std::move(brave_swap_fee_params));
    bool has_fee = brave_fee && brave_fee->has_brave_fee;

    auto encoded_params =
        EncodeJupiterTransactionParams(*jupiter_transaction_params, has_fee);
    if (!encoded_params) {
      std::move(callback).Run(
          nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }

    auto internal_callback =
        base::BindOnce(&SwapService::OnGetJupiterTransaction,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));

    api_request_helper_.Request(
        net::HttpRequestHeaders::kPostMethod,
        GetJupiterTransactionURL(jupiter_transaction_params->chain_id),
        *encoded_params, "application/json", std::move(internal_callback), {},
        {.auto_retry_on_network_change = true});

    return;
  }

  std::move(callback).Run(
      nullptr, nullptr,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
}

void SwapService::OnGetZeroExTransaction(GetTransactionCallback callback,
                                         APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto swap_error_response =
            ParseZeroExErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr,
          mojom::SwapErrorUnion::NewZeroExError(std::move(swap_error_response)),
          "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }

    return;
  }

  if (auto swap_response =
          ParseZeroExQuoteResponse(api_request_result.value_body(), true)) {
    std::move(callback).Run(mojom::SwapTransactionUnion::NewZeroExTransaction(
                                std::move(swap_response)),
                            nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::OnGetJupiterTransaction(GetTransactionCallback callback,
                                          APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            ParseJupiterErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr,
          mojom::SwapErrorUnion::NewJupiterError(std::move(error_response)),
          "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }

    return;
  }

  if (auto swap_transaction =
          ParseJupiterTransactionResponse(api_request_result.value_body())) {
    std::move(callback).Run(
        mojom::SwapTransactionUnion::NewJupiterTransaction(*swap_transaction),
        nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::GetBraveFee(mojom::BraveSwapFeeParamsPtr params,
                              GetBraveFeeCallback callback) {
  if (auto response = GetBraveFeeInternal(std::move(params))) {
    std::move(callback).Run(std::move(response), "");
    return;
  }

  std::move(callback).Run(nullptr,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

}  // namespace brave_wallet
