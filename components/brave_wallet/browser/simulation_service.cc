/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_wallet/browser/simulation_service.h"

#include <optional>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/simulation_request_helper.h"
#include "brave/components/brave_wallet/browser/simulation_response_parser.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/json/rs/src/lib.rs.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("simulation_service", R"(
      semantics {
        sender: "Simulation Service"
        description:
          "This service is used scan proposed transactions and simulate expected state changes."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Blowfish API response bodies."
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

// Function to convert all numbers in JSON string to strings.
//
// For sample JSON response, refer to ParseJupiterQuote.
std::optional<std::string> ConvertAllNumbersToString(const std::string& json) {
  auto converted_json =
      std::string(json::convert_all_numbers_to_string(json, ""));
  if (converted_json.empty()) {
    return std::nullopt;
  }

  return converted_json;
}

}  // namespace

namespace brave_wallet {

namespace {

std::string GetRelativeScanPath(const std::string& chain_id,
                                mojom::CoinType coin) {
  if (coin == mojom::CoinType::SOL && chain_id == mojom::kSolanaMainnet) {
    return "solana/v0/mainnet/scan";
  } else if (coin == mojom::CoinType::SOL &&
             chain_id == mojom::kSolanaTestnet) {
    return "solana/v0/testnet/scan";
  } else if (coin == mojom::CoinType::SOL && chain_id == mojom::kSolanaDevnet) {
    return "solana/v0/devnet/scan";
  } else if (coin == mojom::CoinType::ETH &&
             chain_id == mojom::kMainnetChainId) {
    return "ethereum/v0/mainnet/scan";
  } else if (coin == mojom::CoinType::ETH &&
             chain_id == mojom::kGoerliChainId) {
    return "ethereum/v0/goerli/scan";
  } else if (coin == mojom::CoinType::ETH &&
             chain_id == mojom::kPolygonMainnetChainId) {
    return "polygon/v0/mainnet/scan";
  } else if (coin == mojom::CoinType::ETH &&
             chain_id == mojom::kBinanceSmartChainMainnetChainId) {
    return "bnb/v0/mainnet/scan";
  } else if (coin == mojom::CoinType::ETH &&
             chain_id == mojom::kArbitrumMainnetChainId) {
    return "arbitrum/v0/one/scan";
  }

  return "";
}

bool HasTransactionScanSupportInternal(const std::string& chain_id,
                                       mojom::CoinType coin) {
  return (coin == mojom::CoinType::SOL && chain_id == mojom::kSolanaMainnet) ||
         (coin == mojom::CoinType::SOL && chain_id == mojom::kSolanaTestnet) ||
         (coin == mojom::CoinType::SOL && chain_id == mojom::kSolanaDevnet) ||
         (coin == mojom::CoinType::ETH && chain_id == mojom::kMainnetChainId) ||
         (coin == mojom::CoinType::ETH && chain_id == mojom::kGoerliChainId) ||
         (coin == mojom::CoinType::ETH &&
          chain_id == mojom::kPolygonMainnetChainId) ||
         (coin == mojom::CoinType::ETH &&
          chain_id == mojom::kBinanceSmartChainMainnetChainId) ||
         (coin == mojom::CoinType::ETH &&
          chain_id == mojom::kArbitrumMainnetChainId);
}

bool HasMessageScanSupportInternal(const std::string& chain_id,
                                   mojom::CoinType coin) {
  return (coin == mojom::CoinType::ETH && chain_id == mojom::kMainnetChainId) ||
         (coin == mojom::CoinType::ETH && chain_id == mojom::kGoerliChainId) ||
         (coin == mojom::CoinType::ETH &&
          chain_id == mojom::kPolygonMainnetChainId) ||
         (coin == mojom::CoinType::ETH &&
          chain_id == mojom::kBinanceSmartChainMainnetChainId) ||
         (coin == mojom::CoinType::ETH &&
          chain_id == mojom::kArbitrumMainnetChainId);
}

const base::flat_map<std::string, std::string> GetHeaders() {
  return {{kBlowfishAPIVersionHeader, kBlowfishAPIVersion},
          {net::HttpRequestHeaders::kContentType, "application/json"},
          {kBraveServicesKeyHeader, BUILDFLAG(BRAVE_SERVICES_KEY)}};
}

}  // namespace

SimulationService::SimulationService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

SimulationService::~SimulationService() = default;

mojo::PendingRemote<mojom::SimulationService> SimulationService::MakeRemote() {
  mojo::PendingRemote<mojom::SimulationService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void SimulationService::Bind(
    mojo::PendingReceiver<mojom::SimulationService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// static
GURL SimulationService::GetScanTransactionURL(const std::string& chain_id,
                                              mojom::CoinType coin,
                                              const std::string& language) {
  std::string spec = base::StringPrintf(
      "%s/%s/%s", kBlowfishBaseAPIURL,
      GetRelativeScanPath(chain_id, coin).c_str(),
      coin == mojom::CoinType::SOL ? "transactions" : "transaction");

  return net::AppendQueryParameter(GURL(spec), "language", language);
}

// static
GURL SimulationService::GetScanMessageURL(const std::string& chain_id,
                                          mojom::CoinType coin,
                                          const std::string& language) {
  std::string spec =
      base::StringPrintf("%s/%s/message", kBlowfishBaseAPIURL,
                         GetRelativeScanPath(chain_id, coin).c_str());
  return net::AppendQueryParameter(GURL(spec), "language", language);
}

void SimulationService::HasTransactionScanSupport(
    const std::string& chain_id,
    mojom::CoinType coin,
    HasTransactionScanSupportCallback callback) {
  bool result = HasTransactionScanSupportInternal(chain_id, coin);
  std::move(callback).Run(result);
}

void SimulationService::HasMessageScanSupport(
    const std::string& chain_id,
    mojom::CoinType coin,
    HasMessageScanSupportCallback callback) {
  bool result = HasMessageScanSupportInternal(chain_id, coin);
  std::move(callback).Run(result);
}

void SimulationService::ScanSolanaTransaction(
    mojom::SolanaTransactionRequestUnionPtr request,
    const std::string& language,
    ScanSolanaTransactionCallback callback) {
  if (!request) {
    std::move(callback).Run(
        nullptr, "", l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string chain_id;
  if (request->is_transaction_info()) {
    chain_id = request->get_transaction_info()->chain_id;
  } else if (request->is_sign_transaction_request()) {
    chain_id = request->get_sign_transaction_request()->chain_id;
  } else if (request->is_sign_all_transactions_request()) {
    chain_id = request->get_sign_all_transactions_request()->chain_id;
  }

  if (chain_id.empty()) {
    std::move(callback).Run(
        nullptr, "",
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
    return;
  }

  if (!HasTransactionScanSupportInternal(chain_id, mojom::CoinType::SOL)) {
    std::move(callback).Run(
        nullptr, "",
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
    return;
  }

  const auto& encoded_params = solana::EncodeScanTransactionParams(request);
  if (!encoded_params) {
    std::move(callback).Run(
        nullptr, "", l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&SimulationService::OnScanSolanaTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString);

  api_request_helper_.Request(
      net::HttpRequestHeaders::kPostMethod,
      GetScanTransactionURL(chain_id, mojom::CoinType::SOL, language),
      *encoded_params, "application/json", std::move(internal_callback),
      GetHeaders(), {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void SimulationService::OnScanSolanaTransaction(
    ScanSolanaTransactionCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            ParseSimulationErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(nullptr, std::move(*error_response), "");
    } else {
      std::move(callback).Run(
          nullptr, "", l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }

    return;
  }

  if (auto simulation_response =
          solana::ParseSimulationResponse(api_request_result.value_body())) {
    std::move(callback).Run(std::move(simulation_response), "", "");
  } else {
    std::move(callback).Run(nullptr, "",
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

void SimulationService::ScanEVMTransaction(
    mojom::TransactionInfoPtr tx_info,
    const std::string& language,
    ScanEVMTransactionCallback callback) {
  if (!tx_info) {
    std::move(callback).Run(
        nullptr, "", l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  const auto chain_id = tx_info->chain_id;

  if (!HasTransactionScanSupportInternal(chain_id, mojom::CoinType::ETH)) {
    std::move(callback).Run(
        nullptr, "",
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK));
    return;
  }

  const auto& encoded_params = evm::EncodeScanTransactionParams(tx_info);
  if (!encoded_params) {
    std::move(callback).Run(
        nullptr, "", l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&SimulationService::OnScanEVMTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString);

  api_request_helper_.Request(
      net::HttpRequestHeaders::kPostMethod,
      GetScanTransactionURL(chain_id, mojom::CoinType::ETH, language),
      *encoded_params, "application/json", std::move(internal_callback),
      GetHeaders(), {.auto_retry_on_network_change = true},
      std::move(conversion_callback));
}

void SimulationService::OnScanEVMTransaction(
    ScanEVMTransactionCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    if (auto error_response =
            ParseSimulationErrorResponse(api_request_result.value_body())) {
      std::move(callback).Run(nullptr, std::move(*error_response), "");
    } else {
      std::move(callback).Run(
          nullptr, "", l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }

    return;
  }

  if (auto simulation_response =
          evm::ParseSimulationResponse(api_request_result.value_body())) {
    std::move(callback).Run(std::move(simulation_response), "", "");
  } else {
    std::move(callback).Run(nullptr, "",
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

}  // namespace brave_wallet
