/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_service.h"

#include <limits>
#include <optional>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/swap_request_helper.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/constants/brave_services_key.h"
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

}  // namespace

namespace brave_wallet {

namespace {

// Ref: https://0x.org/docs/introduction/0x-cheat-sheet#-chain-support
bool IsNetworkSupportedByZeroEx(const std::string& chain_id) {
  return GetZeroExAllowanceHolderAddress(chain_id).has_value();
}

bool IsNetworkSupportedByJupiter(const std::string& chain_id) {
  return chain_id == mojom::kSolanaMainnet;
}

// Extracted from https://li.quest/v1/chains
bool IsNetworkSupportedByLiFi(const std::string& chain_id) {
  return (chain_id == mojom::kMainnetChainId ||
          chain_id == mojom::kOptimismMainnetChainId ||
          chain_id == mojom::kBnbSmartChainMainnetChainId ||
          chain_id == mojom::kPolygonZKEVMChainId ||
          chain_id == mojom::kAvalancheMainnetChainId ||
          chain_id == mojom::kGnosisChainId ||
          chain_id == mojom::kArbitrumMainnetChainId ||
          chain_id == mojom::kPolygonMainnetChainId ||
          chain_id == mojom::kZkSyncEraChainId ||
          chain_id == mojom::kBaseMainnetChainId ||
          chain_id == mojom::kFantomMainnetChainId ||
          chain_id == mojom::kAuroraMainnetChainId ||
          chain_id == mojom::kSolanaMainnet);
}

bool IsNetworkSupportedBySquid(const std::string& chain_id) {
  return (chain_id == mojom::kMainnetChainId ||
          chain_id == mojom::kArbitrumMainnetChainId ||
          chain_id == mojom::kAvalancheMainnetChainId ||
          chain_id == mojom::kOptimismMainnetChainId ||
          chain_id == mojom::kPolygonMainnetChainId ||
          chain_id == mojom::kBaseMainnetChainId ||
          chain_id == mojom::kLineaChainId ||
          chain_id == mojom::kBnbSmartChainMainnetChainId ||
          chain_id == mojom::kFantomMainnetChainId ||
          chain_id == mojom::kMoonbeamChainId ||
          chain_id == mojom::kCeloMainnetChainId ||
          chain_id == mojom::kScrollChainId ||
          chain_id == mojom::kFilecoinEthereumMainnetChainId ||
          chain_id == mojom::kBlastMainnetChainId ||
          chain_id == mojom::kImmutableZkEVMChainId);
}

std::optional<std::string> EncodeChainId(const std::string& value) {
  uint256_t val;
  if (!HexValueToUint256(value, &val)) {
    return std::nullopt;
  }

  if (val > std::numeric_limits<uint64_t>::max()) {
    return std::nullopt;
  }

  return base::NumberToString(static_cast<uint64_t>(val));
}

mojom::SwapFeesPtr GetZeroSwapFee() {
  mojom::SwapFeesPtr response = mojom::SwapFees::New();
  response->fee_pct = "0";
  response->discount_pct = "0";
  response->effective_fee_pct = "0";
  response->fee_param = "";
  response->discount_code = mojom::SwapDiscountCode::kNone;
  return response;
}

GURL AppendZeroExSwapParams(const GURL& swap_url,
                            const mojom::SwapQuoteParams& params,
                            const std::optional<std::string>& fee_param) {
  GURL url = swap_url;

  if (!IsNetworkSupportedByZeroEx(params.from_chain_id)) {
    return GURL();
  }

  if (auto chain_id = EncodeChainId(params.from_chain_id)) {
    url = net::AppendQueryParameter(url, "chainId", chain_id.value());
  }

  if (!params.from_account_id->address.empty()) {
    url = net::AppendQueryParameter(url, "taker",
                                    params.from_account_id->address);
  }
  if (!params.from_amount.empty()) {
    url = net::AppendQueryParameter(url, "sellAmount", params.from_amount);
  }
  if (!params.to_amount.empty()) {
    url = net::AppendQueryParameter(url, "buyAmount", params.to_amount);
  }

  auto buy_token = params.to_token.empty() ? kNativeEVMAssetContractAddress
                                           : params.to_token;
  url = net::AppendQueryParameter(url, "buyToken", buy_token);
  url = net::AppendQueryParameter(url, "sellToken",
                                  params.from_token.empty()
                                      ? kNativeEVMAssetContractAddress
                                      : params.from_token);

  if (fee_param.has_value() && !fee_param->empty()) {
    url = net::AppendQueryParameter(url, "swapFeeBps", fee_param.value());
    url = net::AppendQueryParameter(url, "swapFeeRecipient", kEVMFeeRecipient);
    url = net::AppendQueryParameter(url, "swapFeeToken", buy_token);
  }

  double slippage_percentage = 0.0;
  if (base::StringToDouble(params.slippage_percentage, &slippage_percentage)) {
    url = net::AppendQueryParameter(
        url, "slippageBps",
        base::StringPrintf("%d", static_cast<int>(slippage_percentage * 100)));
  }

  // TODO(onyb): custom gas_price is currently unused and may be removed in
  // future.
  // if (!params.gas_price.empty()) {
  // url = net::AppendQueryParameter(url, "gasPrice", params.gas_price);
  // }

  return url;
}

GURL AppendJupiterQuoteParams(const GURL& swap_url,
                              const mojom::SwapQuoteParams& params,
                              const std::optional<std::string>& fee_param) {
  GURL url = swap_url;
  url = net::AppendQueryParameter(url, "inputMint",
                                  params.from_token.empty()
                                      ? kWrappedSolanaMintAddress
                                      : params.from_token);
  url = net::AppendQueryParameter(
      url, "outputMint",
      params.to_token.empty() ? kWrappedSolanaMintAddress : params.to_token);

  if (!params.from_amount.empty()) {
    url = net::AppendQueryParameter(url, "amount", params.from_amount);
    url = net::AppendQueryParameter(url, "swapMode", "ExactIn");
  } else if (!params.to_amount.empty()) {
    url = net::AppendQueryParameter(url, "amount", params.to_amount);
    url = net::AppendQueryParameter(url, "swapMode", "ExactOut");
  }

  double slippage_percentage = 0.0;
  if (base::StringToDouble(params.slippage_percentage, &slippage_percentage)) {
    url = net::AppendQueryParameter(
        url, "slippageBps",
        base::StringPrintf("%d", static_cast<int>(slippage_percentage * 100)));
  }

  if (fee_param.has_value() && !fee_param->empty()) {
    url = net::AppendQueryParameter(url, "platformFeeBps", fee_param.value());
  }

  // TODO(onyb): append userPublicKey to get information on fees and ATA
  // deposits.

  return url;
}

base::flat_map<std::string, std::string> GetHeaders() {
  return {{kBraveServicesKeyHeader, BUILDFLAG(BRAVE_SERVICES_KEY)}};
}

base::flat_map<std::string, std::string> GetHeadersForZeroEx() {
  auto headers = GetHeaders();
  headers[kZeroExAPIVersionHeader] = kZeroExAPIVersion;
  return headers;
}

}  // namespace

SwapService::SwapService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

SwapService::~SwapService() = default;

mojo::PendingRemote<mojom::SwapService> SwapService::MakeRemote() {
  mojo::PendingRemote<mojom::SwapService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void SwapService::Bind(mojo::PendingReceiver<mojom::SwapService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// static
GURL SwapService::GetZeroExQuoteURL(
    const mojom::SwapQuoteParams& params,
    const std::optional<std::string>& fee_param) {
  auto url = GURL(kZeroExBaseAPIURL).Resolve("/swap/allowance-holder/price");
  return AppendZeroExSwapParams(url, params, fee_param);
}

// static
GURL SwapService::GetZeroExTransactionURL(
    const mojom::SwapQuoteParams& params,
    const std::optional<std::string>& fee_param) {
  auto url = GURL(kZeroExBaseAPIURL).Resolve("/swap/allowance-holder/quote");
  return AppendZeroExSwapParams(url, params, fee_param);
}

// static
GURL SwapService::GetJupiterQuoteURL(
    const mojom::SwapQuoteParams& params,
    const std::optional<std::string>& fee_param) {
  auto url = GURL(kJupiterBaseAPIURL).Resolve("/v6/quote");
  url = AppendJupiterQuoteParams(url, params, fee_param);

  return url;
}

// static
GURL SwapService::GetJupiterTransactionURL(const std::string& chain_id) {
  return GURL(kJupiterBaseAPIURL).Resolve("/v6/swap");
}

// static
GURL SwapService::GetLiFiQuoteURL() {
  return GURL(kLiFiBaseAPIURL).Resolve("/v1/advanced/routes");
}

// static
GURL SwapService::GetLiFiTransactionURL() {
  return GURL(kLiFiBaseAPIURL).Resolve("/v1/advanced/stepTransaction");
}

// static
GURL SwapService::GetLiFiStatusURL(const std::string& tx_hash) {
  return GURL(kLiFiBaseAPIURL).Resolve("/v1/status?txHash=" + tx_hash);
}

// static
GURL SwapService::GetSquidURL() {
  return GURL(kSquidBaseAPIURL).Resolve("/v2/route");
}

void SwapService::IsSwapSupported(const std::string& chain_id,
                                  IsSwapSupportedCallback callback) {
  std::move(callback).Run(IsNetworkSupportedByZeroEx(chain_id) ||
                          IsNetworkSupportedByJupiter(chain_id) ||
                          IsNetworkSupportedByLiFi(chain_id) ||
                          IsNetworkSupportedBySquid(chain_id));
}

// Method to fetch a quote for a swap or a bridge transaction.
//
// If the provider is set to kAuto, the method will attempt to fetch a quote
// based on the following priority:
//   P1. LiFi, Jupiter
//   P2. 0x, Squid
void SwapService::GetQuote(mojom::SwapQuoteParamsPtr params,
                           GetQuoteCallback callback) {
  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString, "");

  auto has_zero_ex_support = params->from_chain_id == params->to_chain_id &&
                             IsNetworkSupportedByZeroEx(params->from_chain_id);
  auto has_jupiter_support = params->from_chain_id == params->to_chain_id &&
                             IsNetworkSupportedByJupiter(params->from_chain_id);
  auto has_lifi_support = IsNetworkSupportedByLiFi(params->from_chain_id) &&
                          IsNetworkSupportedByLiFi(params->to_chain_id) &&
                          // LiFi does not support ExactOut swaps.
                          !params->from_amount.empty();
  auto has_squid_support = IsNetworkSupportedBySquid(params->from_chain_id) &&
                           IsNetworkSupportedBySquid(params->to_chain_id);

  // If the provider is set to Auto, Solana swaps are served via Jupiter.
  if ((params->provider == mojom::SwapProvider::kJupiter ||
       params->provider == mojom::SwapProvider::kAuto) &&
      has_jupiter_support) {
    auto swap_fee = GetZeroSwapFee();
    auto fee_param = swap_fee->fee_param;

    auto internal_callback = base::BindOnce(
        &SwapService::OnGetJupiterQuote, weak_ptr_factory_.GetWeakPtr(),
        std::move(swap_fee), std::move(callback));

    api_request_helper_.Request(net::HttpRequestHeaders::kGetMethod,
                                GetJupiterQuoteURL(*params, fee_param), "", "",
                                std::move(internal_callback), GetHeaders(), {},
                                std::move(conversion_callback));

    return;
  }

  // EVM swaps are served via LiFi if the provider is set to kLiFi or kAuto.
  if ((params->provider == mojom::SwapProvider::kLiFi ||
       params->provider == mojom::SwapProvider::kAuto) &&
      has_lifi_support) {
    auto swap_fee = GetZeroSwapFee();
    auto fee_param = swap_fee->fee_param;

    auto encoded_params = lifi::EncodeQuoteParams(std::move(params), fee_param);
    if (!encoded_params) {
      std::move(callback).Run(
          nullptr, nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }

    auto internal_callback = base::BindOnce(
        &SwapService::OnGetLiFiQuote, weak_ptr_factory_.GetWeakPtr(),
        std::move(swap_fee), std::move(callback));

    api_request_helper_.Request(
        net::HttpRequestHeaders::kPostMethod, GetLiFiQuoteURL(),
        *encoded_params, "application/json", std::move(internal_callback),
        GetHeaders(), {}, std::move(conversion_callback));
    return;
  }

  // EVM swaps are served via 0x only if the provider is set to kZeroEx.
  if ((params->provider == mojom::SwapProvider::kZeroEx ||
       params->provider == mojom::SwapProvider::kAuto) &&
      has_zero_ex_support) {
    auto swap_fee = GetZeroSwapFee();
    auto fee_param = swap_fee->fee_param;

    auto internal_callback = base::BindOnce(
        &SwapService::OnGetZeroExQuote, weak_ptr_factory_.GetWeakPtr(),
        params->from_chain_id, std::move(swap_fee), std::move(callback));

    api_request_helper_.Request(net::HttpRequestHeaders::kGetMethod,
                                GetZeroExQuoteURL(*params, fee_param), "", "",
                                std::move(internal_callback),
                                GetHeadersForZeroEx(), {},
                                std::move(conversion_callback));

    return;
  }

  if ((params->provider == mojom::SwapProvider::kSquid ||
       params->provider == mojom::SwapProvider::kAuto) &&
      has_squid_support) {
    auto encoded_params = squid::EncodeQuoteParams(std::move(params));
    if (!encoded_params) {
      std::move(callback).Run(
          nullptr, nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }

    auto swap_fee = GetZeroSwapFee();

    auto internal_callback = base::BindOnce(
        &SwapService::OnGetSquidQuote, weak_ptr_factory_.GetWeakPtr(),
        std::move(swap_fee), std::move(callback));

    api_request_helper_.Request(
        net::HttpRequestHeaders::kPostMethod, GetSquidURL(), *encoded_params,
        "application/json", std::move(internal_callback), GetHeaders(), {},
        std::move(conversion_callback));
    return;
  }

  std::move(callback).Run(
      nullptr, nullptr, nullptr,
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
}

void SwapService::OnGetZeroExQuote(const std::string& chain_id,
                                   mojom::SwapFeesPtr swap_fee,
                                   GetQuoteCallback callback,
                                   APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto swap_error_response =
            zeroex::ParseErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr, nullptr,
          mojom::SwapErrorUnion::NewZeroExError(std::move(swap_error_response)),
          "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  if (auto swap_response = zeroex::ParseQuoteResponse(
          api_request_result.value_body(), chain_id)) {
    if (!swap_response->liquidity_available) {
      std::move(callback).Run(
          nullptr, nullptr,
          mojom::SwapErrorUnion::NewZeroExError(mojom::ZeroExError::New(
              "INSUFFICIENT_LIQUIDITY",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SWAP_INSUFFICIENT_LIQUIDITY),
              true)),
          "");
      return;
    }

    std::move(callback).Run(
        mojom::SwapQuoteUnion::NewZeroExQuote(std::move(swap_response)),
        std::move(swap_fee), nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::OnGetJupiterQuote(mojom::SwapFeesPtr swap_fee,
                                    GetQuoteCallback callback,
                                    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            jupiter::ParseErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr, nullptr,
          mojom::SwapErrorUnion::NewJupiterError(std::move(error_response)),
          "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  if (auto swap_quote =
          jupiter::ParseQuoteResponse(api_request_result.value_body())) {
    std::move(callback).Run(
        mojom::SwapQuoteUnion::NewJupiterQuote(std::move(swap_quote)),
        std::move(swap_fee), nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::OnGetLiFiQuote(mojom::SwapFeesPtr swap_fee,
                                 GetQuoteCallback callback,
                                 APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            lifi::ParseErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr, nullptr,
          mojom::SwapErrorUnion::NewLifiError(std::move(error_response)), "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  if (auto quote = lifi::ParseQuoteResponse(api_request_result.value_body())) {
    if (quote->routes.empty()) {
      auto error_response = mojom::LiFiError::New();
      error_response->code = mojom::LiFiErrorCode::kNotFoundError;
      error_response->message =
          l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_NO_ROUTES_FOUND);

      std::move(callback).Run(
          nullptr, nullptr,
          mojom::SwapErrorUnion::NewLifiError(std::move(error_response)), "");
      return;
    }

    std::move(callback).Run(
        mojom::SwapQuoteUnion::NewLifiQuote(std::move(quote)),
        std::move(swap_fee), nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::OnGetSquidQuote(mojom::SwapFeesPtr swap_fee,
                                  GetQuoteCallback callback,
                                  APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            squid::ParseErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr, nullptr,
          mojom::SwapErrorUnion::NewSquidError(std::move(error_response)), "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  if (auto quote = squid::ParseQuoteResponse(api_request_result.value_body())) {
    std::move(callback).Run(
        mojom::SwapQuoteUnion::NewSquidQuote(std::move(quote)),
        std::move(swap_fee), nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::GetTransaction(mojom::SwapTransactionParamsUnionPtr params,
                                 GetTransactionCallback callback) {
  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString, "");

  if (params->is_zero_ex_transaction_params()) {
    auto swap_fee = GetZeroSwapFee();

    auto internal_callback =
        base::BindOnce(&SwapService::OnGetZeroExTransaction,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));

    api_request_helper_.Request(
        net::HttpRequestHeaders::kGetMethod,
        GetZeroExTransactionURL(*params->get_zero_ex_transaction_params(),
                                swap_fee->fee_param),
        "", "", std::move(internal_callback), GetHeadersForZeroEx(), {},
        std::move(conversion_callback));

    return;
  }

  if (params->is_jupiter_transaction_params()) {
    auto& jupiter_transaction_params = params->get_jupiter_transaction_params();
    auto encoded_params =
        jupiter::EncodeTransactionParams(*jupiter_transaction_params);
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
        *encoded_params, "application/json", std::move(internal_callback),
        GetHeaders(), {}, std::move(conversion_callback));

    return;
  }

  if (params->is_lifi_transaction_params()) {
    auto& lifi_transaction_params = params->get_lifi_transaction_params();

    auto encoded_params =
        lifi::EncodeTransactionParams(std::move(lifi_transaction_params));
    if (!encoded_params) {
      std::move(callback).Run(
          nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }

    auto internal_callback =
        base::BindOnce(&SwapService::OnGetLiFiTransaction,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));

    api_request_helper_.Request(
        net::HttpRequestHeaders::kPostMethod, GetLiFiTransactionURL(),
        *encoded_params, "application/json", std::move(internal_callback),
        GetHeaders(), {}, std::move(conversion_callback));

    return;
  }

  if (params->is_squid_transaction_params()) {
    auto& squid_transaction_params = params->get_squid_transaction_params();
    auto encoded_params =
        squid::EncodeTransactionParams(std::move(squid_transaction_params));
    if (!encoded_params) {
      std::move(callback).Run(
          nullptr, nullptr,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }

    auto internal_callback =
        base::BindOnce(&SwapService::OnGetSquidTransaction,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));

    api_request_helper_.Request(
        net::HttpRequestHeaders::kPostMethod, GetSquidURL(), *encoded_params,
        "application/json", std::move(internal_callback), GetHeaders(), {},
        std::move(conversion_callback));

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
            zeroex::ParseErrorResponse(api_request_result.value_body())) {
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
          zeroex::ParseTransactionResponse(api_request_result.value_body())) {
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
            jupiter::ParseErrorResponse(api_request_result.value_body())) {
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
          jupiter::ParseTransactionResponse(api_request_result.value_body())) {
    std::move(callback).Run(
        mojom::SwapTransactionUnion::NewJupiterTransaction(*swap_transaction),
        nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::OnGetLiFiTransaction(GetTransactionCallback callback,
                                       APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            lifi::ParseErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr,
          mojom::SwapErrorUnion::NewLifiError(std::move(error_response)), "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }

    return;
  }

  if (auto transaction =
          lifi::ParseTransactionResponse(api_request_result.value_body())) {
    std::move(callback).Run(
        mojom::SwapTransactionUnion::NewLifiTransaction(std::move(transaction)),
        nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::OnGetSquidTransaction(GetTransactionCallback callback,
                                        APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            squid::ParseErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(
          nullptr,
          mojom::SwapErrorUnion::NewSquidError(std::move(error_response)), "");
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }

    return;
  }

  if (auto transaction =
          squid::ParseTransactionResponse(api_request_result.value_body())) {
    std::move(callback).Run(mojom::SwapTransactionUnion::NewSquidTransaction(
                                std::move(transaction)),
                            nullptr, "");
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SwapService::GetLiFiStatus(const std::string& tx_hash,
                                GetLiFiStatusCallback callback) {
  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString, "");
  auto internal_callback =
      base::BindOnce(&SwapService::OnGetLiFiStatus,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_.Request(net::HttpRequestHeaders::kGetMethod,
                              GetLiFiStatusURL(tx_hash), "", "",
                              std::move(internal_callback), GetHeaders(), {},
                              std::move(conversion_callback));
}

void SwapService::OnGetLiFiStatus(GetLiFiStatusCallback callback,
                                  APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            lifi::ParseErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(nullptr, std::move(error_response), "");
      return;
    } else {
      std::move(callback).Run(
          nullptr, nullptr, l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
      return;
    }
  }

  if (auto status =
          lifi::ParseStatusResponse(api_request_result.value_body())) {
    std::move(callback).Run(std::move(status), nullptr, "");
    return;
  } else {
    std::move(callback).Run(nullptr, nullptr,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }
}

}  // namespace brave_wallet
