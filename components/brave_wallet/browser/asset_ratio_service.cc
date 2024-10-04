/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_service.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/environment.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("asset_ratio_service", R"(
      semantics {
        sender: "Asset Ratio Service"
        description:
          "This service is used to obtain asset prices for the Brave wallet."
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

std::string VectorToCommaSeparatedList(const std::vector<std::string>& assets) {
  std::stringstream ss;
  std::for_each(assets.begin(), assets.end(), [&ss](const std::string& asset) {
    if (ss.tellp() != 0) {
      ss << ",";
    }
    ss << asset;
  });
  return ss.str();
}

std::string TimeFrameKeyToString(
    brave_wallet::mojom::AssetPriceTimeframe timeframe) {
  std::string timeframe_key;
  switch (timeframe) {
    case brave_wallet::mojom::AssetPriceTimeframe::Live:
      timeframe_key = "live";
      break;
    case brave_wallet::mojom::AssetPriceTimeframe::OneDay:
      timeframe_key = "1d";
      break;
    case brave_wallet::mojom::AssetPriceTimeframe::OneWeek:
      timeframe_key = "1w";
      break;
    case brave_wallet::mojom::AssetPriceTimeframe::OneMonth:
      timeframe_key = "1m";
      break;
    case brave_wallet::mojom::AssetPriceTimeframe::ThreeMonths:
      timeframe_key = "3m";
      break;
    case brave_wallet::mojom::AssetPriceTimeframe::OneYear:
      timeframe_key = "1y";
      break;
    case brave_wallet::mojom::AssetPriceTimeframe::All:
      timeframe_key = "all";
      break;
  }
  return timeframe_key;
}

std::vector<std::string> VectorToLowerCase(const std::vector<std::string>& v) {
  std::vector<std::string> v_lower(v.size());
  std::transform(v.begin(), v.end(), v_lower.begin(),
                 [](const std::string& from) -> std::string {
                   return base::ToLowerASCII(from);
                 });
  return v_lower;
}

std::optional<std::string> ChainIdToStripeChainId(const std::string& chain_id) {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      chain_id_lookup({{brave_wallet::mojom::kMainnetChainId, "ethereum"},
                       {brave_wallet::mojom::kSolanaMainnet, "solana"},
                       {brave_wallet::mojom::kPolygonMainnetChainId, "polygon"},
                       {brave_wallet::mojom::kBitcoinMainnet, "bitcoin"}});
  if (!chain_id_lookup->contains(chain_id)) {
    return std::nullopt;
  }

  return chain_id_lookup->at(chain_id);
}

}  // namespace

namespace brave_wallet {

namespace {

std::vector<mojom::AssetPricePtr> DummyPrices(
    const std::vector<std::string>& from_assets,
    const std::vector<std::string>& to_assets) {
  std::vector<mojom::AssetPricePtr> test_result;
  for (auto& from : from_assets) {
    for (auto& to : to_assets) {
      auto price = mojom::AssetPrice::New();
      price->from_asset = from;
      price->to_asset = to;
      price->price = "1";
      price->asset_timeframe_change = "1";
      test_result.push_back(std::move(price));
    }
  }

  return test_result;
}

}  // namespace

GURL AssetRatioService::base_url_for_test_;

AssetRatioService::AssetRatioService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(new api_request_helper::APIRequestHelper(
          GetNetworkTrafficAnnotationTag(),
          url_loader_factory)),
      weak_ptr_factory_(this) {}

AssetRatioService::~AssetRatioService() = default;

void AssetRatioService::SetAPIRequestHelperForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetNetworkTrafficAnnotationTag(), url_loader_factory);
}

void AssetRatioService::EnableDummyPricesForTesting() {
  dummy_prices_for_testing_ = true;
}

mojo::PendingRemote<mojom::AssetRatioService> AssetRatioService::MakeRemote() {
  mojo::PendingRemote<mojom::AssetRatioService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void AssetRatioService::Bind(
    mojo::PendingReceiver<mojom::AssetRatioService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void AssetRatioService::SetBaseURLForTest(const GURL& base_url_for_test) {
  base_url_for_test_ = base_url_for_test;
}

GURL AssetRatioService::GetSardineBuyURL(const std::string& chain_id,
                                         const std::string& address,
                                         const std::string& symbol,
                                         const std::string& amount,
                                         const std::string& currency_code,
                                         const std::string& auth_token) {
  const std::string sardine_network_name = GetSardineNetworkName(chain_id);
  GURL url = GURL(kSardineStorefrontBaseURL);
  url = net::AppendQueryParameter(url, "address", address);
  url = net::AppendQueryParameter(url, "network", sardine_network_name);
  url = net::AppendQueryParameter(url, "asset_type", symbol);
  url = net::AppendQueryParameter(url, "fiat_amount", amount);
  url = net::AppendQueryParameter(url, "fiat_currency", currency_code);
  url = net::AppendQueryParameter(url, "client_token", auth_token);
  url = net::AppendQueryParameter(url, "fixed_asset_type", symbol);
  url = net::AppendQueryParameter(url, "fixed_network", sardine_network_name);
  return url;
}

// static
GURL AssetRatioService::GetPriceURL(const std::vector<std::string>& from_assets,
                                    const std::vector<std::string>& to_assets,
                                    mojom::AssetPriceTimeframe timeframe) {
  std::string from = VectorToCommaSeparatedList(from_assets);
  std::string to = VectorToCommaSeparatedList(to_assets);
  std::string spec = base::StringPrintf(
      "%s/v2/relative/provider/coingecko/%s/%s/%s",
      base_url_for_test_.is_empty() ? GetAssetRatioBaseURL().c_str()
                                    : base_url_for_test_.spec().c_str(),
      from.c_str(), to.c_str(), TimeFrameKeyToString(timeframe).c_str());
  return GURL(spec);
}

// static
GURL AssetRatioService::GetPriceHistoryURL(
    const std::string& asset,
    const std::string& vs_asset,
    mojom::AssetPriceTimeframe timeframe) {
  std::string spec = base::StringPrintf(
      "%s/v2/history/coingecko/%s/%s/%s",
      base_url_for_test_.is_empty() ? GetAssetRatioBaseURL().c_str()
                                    : base_url_for_test_.spec().c_str(),
      asset.c_str(), vs_asset.c_str(), TimeFrameKeyToString(timeframe).c_str());
  return GURL(spec);
}

void AssetRatioService::GetBuyUrlV1(mojom::OnRampProvider provider,
                                    const std::string& chain_id,
                                    const std::string& address,
                                    const std::string& symbol,
                                    const std::string& amount,
                                    const std::string& currency_code,
                                    GetBuyUrlV1Callback callback) {
  std::string url;
  if (provider == mojom::OnRampProvider::kRamp) {
    GURL ramp_url = GURL(kRampBaseUrl);
    ramp_url = net::AppendQueryParameter(ramp_url, "enabledFlows",
                                         kOnRampEnabledFlows);
    ramp_url = net::AppendQueryParameter(ramp_url, "userAddress", address);
    ramp_url = net::AppendQueryParameter(ramp_url, "swapAsset", symbol);
    ramp_url = net::AppendQueryParameter(ramp_url, "fiatValue", amount);
    ramp_url =
        net::AppendQueryParameter(ramp_url, "fiatCurrency", currency_code);
    ramp_url = net::AppendQueryParameter(ramp_url, "hostApiKey", kOnRampID);
    std::move(callback).Run(std::move(ramp_url.spec()), std::nullopt);
  } else if (provider == mojom::OnRampProvider::kSardine) {
    auto internal_callback =
        base::BindOnce(&AssetRatioService::OnGetSardineAuthToken,
                       weak_ptr_factory_.GetWeakPtr(), chain_id, address,
                       symbol, amount, currency_code, std::move(callback));
    GURL sardine_token_url = GURL(kSardineClientTokensURL);
    const std::string sardine_client_id(SARDINE_CLIENT_ID);
    const std::string sardine_client_secret(SARDINE_CLIENT_SECRET);

    base::Value::Dict payload_value;
    payload_value.Set("clientId", sardine_client_id);
    payload_value.Set("clientSecret", sardine_client_id);
    std::string payload;
    base::JSONWriter::Write(payload_value, &payload);
    base::flat_map<std::string, std::string> request_headers;
    std::string credentials = base::StringPrintf(
        "%s:%s", sardine_client_id.c_str(),  // username:password
        sardine_client_secret.c_str());
    std::string base64_credentials = base::Base64Encode(credentials);
    std::string header =
        base::StringPrintf("Basic %s", base64_credentials.c_str());
    request_headers["Authorization"] = std::move(header);
    api_request_helper_->Request("POST", sardine_token_url, payload,
                                 "application/json",
                                 std::move(internal_callback), request_headers,
                                 {.auto_retry_on_network_change = true});
  } else if (provider == mojom::OnRampProvider::kTransak) {
    GURL transak_url = GURL(kTransakURL);
    transak_url =
        net::AppendQueryParameter(transak_url, "fiatCurrency", currency_code);
    transak_url =
        net::AppendQueryParameter(transak_url, "defaultCryptoCurrency", symbol);
    transak_url =
        net::AppendQueryParameter(transak_url, "defaultFiatAmount", amount);
    transak_url =
        net::AppendQueryParameter(transak_url, "walletAddress", address);
    transak_url = net::AppendQueryParameter(
        transak_url, "networks",
        "ethereum,arbitrum,optimism,polygon,bsc,solana,avaxcchain,osmosis,"
        "fantom,aurora,celo,mainnet");
    transak_url =
        net::AppendQueryParameter(transak_url, "apiKey", kTransakApiKey);

    std::move(callback).Run(std::move(transak_url.spec()), std::nullopt);
  } else if (provider == mojom::OnRampProvider::kStripe) {
    GetStripeBuyURL(std::move(callback), address, currency_code, amount,
                    chain_id, symbol);
  } else if (provider == mojom::OnRampProvider::kCoinbase) {
    GURL coinbase_url = GURL(kCoinbaseURL);
    coinbase_url =
        net::AppendQueryParameter(coinbase_url, "appId", kCoinbaseAppId);
    coinbase_url =
        net::AppendQueryParameter(coinbase_url, "defaultExperience", "buy");
    coinbase_url =
        net::AppendQueryParameter(coinbase_url, "presetFiatAmount", amount);

    // Construct the destinationWallets JSON
    base::Value::List destinationWallets;
    base::Value::Dict wallet;
    wallet.Set("address", address);

    // Restrict to ETH or SOL chains based on the address
    base::Value::List blockchains;
    if (EthAddress::IsValidAddress(address)) {
      // Supported networks list
      // https://docs.cloud.coinbase.com/pay-sdk/docs/faq#which-blockchains-and-cryptocurrencies-do-you-support
      blockchains.Append("ethereum");
      blockchains.Append("arbitrum");
      blockchains.Append("optimism");
      blockchains.Append("polygon");
      blockchains.Append("avalanche-c-chain");
      blockchains.Append("celo");
    } else {
      // Assume it's a valid SOL address
      blockchains.Append("solana");
    }
    wallet.Set("blockchains", std::move(blockchains));
    base::Value::List assets;
    assets.Append(symbol);
    wallet.Set("assets", std::move(assets));
    destinationWallets.Append(std::move(wallet));

    // Convert the JSON to a string and URL-encode it
    std::string destinationWalletsStr;
    base::JSONWriter::Write(destinationWallets, &destinationWalletsStr);

    // Append the destinationWallets parameter
    coinbase_url = net::AppendQueryParameter(coinbase_url, "destinationWallets",
                                             destinationWalletsStr);

    std::move(callback).Run(std::move(coinbase_url.spec()), std::nullopt);
  } else {
    std::move(callback).Run(url, "UNSUPPORTED_ONRAMP_PROVIDER");
  }
}

void AssetRatioService::GetSellUrl(mojom::OffRampProvider provider,
                                   const std::string& chain_id,
                                   const std::string& symbol,
                                   const std::string& amount,
                                   const std::string& currency_code,
                                   GetSellUrlCallback callback) {
  std::string url;
  if (provider == mojom::OffRampProvider::kRamp) {
    GURL off_ramp_url = GURL(kRampBaseUrl);
    off_ramp_url = net::AppendQueryParameter(off_ramp_url, "enabledFlows",
                                             kOffRampEnabledFlows);
    off_ramp_url = net::AppendQueryParameter(off_ramp_url, "swapAsset", symbol);
    off_ramp_url =
        net::AppendQueryParameter(off_ramp_url, "offrampAsset", symbol);
    off_ramp_url =
        net::AppendQueryParameter(off_ramp_url, "swapAmount", amount);
    off_ramp_url =
        net::AppendQueryParameter(off_ramp_url, "fiatCurrency", currency_code);
    off_ramp_url =
        net::AppendQueryParameter(off_ramp_url, "hostApiKey", kOffRampID);
    std::move(callback).Run(off_ramp_url.spec(), std::nullopt);
  } else {
    std::move(callback).Run(url, "UNSUPPORTED_OFFRAMP_PROVIDER");
  }
}

void AssetRatioService::GetPrice(const std::vector<std::string>& from_assets,
                                 const std::vector<std::string>& to_assets,
                                 mojom::AssetPriceTimeframe timeframe,
                                 GetPriceCallback callback) {
  if (dummy_prices_for_testing_) {
    std::move(callback).Run(true, DummyPrices(from_assets, to_assets));
    return;
  }
  std::vector<std::string> from_assets_lower = VectorToLowerCase(from_assets);
  std::vector<std::string> to_assets_lower = VectorToLowerCase(to_assets);
  auto internal_callback = base::BindOnce(
      &AssetRatioService::OnGetPrice, weak_ptr_factory_.GetWeakPtr(),
      from_assets_lower, to_assets_lower, std::move(callback));

  api_request_helper_->Request(
      "GET", GetPriceURL(from_assets_lower, to_assets_lower, timeframe), "", "",
      std::move(internal_callback), MakeBraveServicesKeyHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void AssetRatioService::OnGetSardineAuthToken(
    const std::string& chain_id,
    const std::string& address,
    const std::string& symbol,
    const std::string& amount,
    const std::string& currency_code,
    GetBuyUrlV1Callback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run("", "INTERNAL_SERVICE_ERROR");
    return;
  }

  auto auth_token = ParseSardineAuthToken(api_request_result.value_body());
  if (!auth_token) {
    std::move(callback).Run("", "INTERNAL_SERVICE_ERROR");
    return;
  }

  GURL sardine_buy_url = GetSardineBuyURL(chain_id, address, symbol, amount,
                                          currency_code, *auth_token);
  std::move(callback).Run(std::move(sardine_buy_url.spec()), std::nullopt);
}

void AssetRatioService::GetStripeBuyURL(
    GetBuyUrlV1Callback callback,
    const std::string& address,
    const std::string& source_currency,
    const std::string& source_exchange_amount,
    const std::string& chain_id,
    const std::string& destination_currency) {
  // Convert the frontend supplied chain ID to the chain ID used by Stripe
  std::optional<std::string> destination_network =
      ChainIdToStripeChainId(chain_id);
  if (!destination_network) {
    std::move(callback).Run("", "UNSUPPORTED_CHAIN_ID");
    return;
  }

  base::Value::Dict payload;
  AddKeyIfNotEmpty(&payload, "wallet_address", address);
  AddKeyIfNotEmpty(&payload, "source_currency", source_currency);
  AddKeyIfNotEmpty(&payload, "source_exchange_amount", source_exchange_amount);
  AddKeyIfNotEmpty(&payload, "destination_network", *destination_network);
  AddKeyIfNotEmpty(&payload, "destination_currency", destination_currency);

  const std::string json_payload = GetJSON(payload);

  GURL url = GURL(base::StringPrintf("%s/v2/stripe/onramp_sessions",
                                     base_url_for_test_.is_empty()
                                         ? GetAssetRatioBaseURL().c_str()
                                         : base_url_for_test_.spec().c_str()));

  auto internal_callback =
      base::BindOnce(&AssetRatioService::OnGetStripeBuyURL,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "POST", url, json_payload, "application/json",
      std::move(internal_callback), MakeBraveServicesKeyHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = false});
}

void AssetRatioService::OnGetStripeBuyURL(GetBuyUrlV1Callback callback,
                                          APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run("", "INTERNAL_SERVICE_ERROR");
    return;
  }

  auto url = ParseStripeBuyURL(api_request_result.value_body());
  if (!url) {
    std::move(callback).Run("", "PARSING_ERROR");
    return;
  }

  std::move(callback).Run(*url, std::nullopt);
}

void AssetRatioService::OnGetPrice(std::vector<std::string> from_assets,
                                   std::vector<std::string> to_assets,
                                   GetPriceCallback callback,
                                   APIRequestResult api_request_result) {
  std::vector<mojom::AssetPricePtr> prices;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(false, std::move(prices));
    return;
  }
  if (!ParseAssetPrice(api_request_result.value_body(), from_assets, to_assets,
                       &prices)) {
    std::move(callback).Run(false, std::move(prices));
    return;
  }

  std::move(callback).Run(true, std::move(prices));
}

void AssetRatioService::GetPriceHistory(const std::string& asset,
                                        const std::string& vs_asset,
                                        mojom::AssetPriceTimeframe timeframe,
                                        GetPriceHistoryCallback callback) {
  std::string asset_lower = base::ToLowerASCII(asset);
  std::string vs_asset_lower = base::ToLowerASCII(vs_asset);
  auto internal_callback =
      base::BindOnce(&AssetRatioService::OnGetPriceHistory,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_->Request(
      "GET", GetPriceHistoryURL(asset_lower, vs_asset_lower, timeframe), "", "",
      std::move(internal_callback), MakeBraveServicesKeyHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void AssetRatioService::OnGetPriceHistory(GetPriceHistoryCallback callback,
                                          APIRequestResult api_request_result) {
  std::vector<mojom::AssetTimePricePtr> values;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(false, std::move(values));
    return;
  }
  if (!ParseAssetPriceHistory(api_request_result.value_body(), &values)) {
    std::move(callback).Run(false, std::move(values));
    return;
  }

  std::move(callback).Run(true, std::move(values));
}

// static
GURL AssetRatioService::GetCoinMarketsURL(const std::string& vs_asset,
                                          const uint8_t limit) {
  GURL url = GURL(base::StringPrintf("%s/v2/market/provider/coingecko",
                                     base_url_for_test_.is_empty()
                                         ? GetAssetRatioBaseURL().c_str()
                                         : base_url_for_test_.spec().c_str()));
  url = net::AppendQueryParameter(url, "vsCurrency", vs_asset);
  url = net::AppendQueryParameter(url, "limit", std::to_string(limit));
  return url;
}

void AssetRatioService::GetCoinMarkets(const std::string& vs_asset,
                                       const uint8_t limit,
                                       GetCoinMarketsCallback callback) {
  std::string vs_asset_lower = base::ToLowerASCII(vs_asset);
  auto internal_callback =
      base::BindOnce(&AssetRatioService::OnGetCoinMarkets,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_->Request(
      "GET", GetCoinMarketsURL(vs_asset_lower, limit), "", "",
      std::move(internal_callback), MakeBraveServicesKeyHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true});
}

void AssetRatioService::OnGetCoinMarkets(GetCoinMarketsCallback callback,
                                         APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(false, {});
    return;
  }

  auto values = ParseCoinMarkets(api_request_result.value_body());
  if (!values) {
    std::move(callback).Run(false, {});
    return;
  }

  std::move(callback).Run(true, std::move(*values));
}

}  // namespace brave_wallet
