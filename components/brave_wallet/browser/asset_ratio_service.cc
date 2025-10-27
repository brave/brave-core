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
#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_map.h"
#include "base/environment.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

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

std::optional<std::string_view> ChainIdToStripeChainId(
    std::string_view chain_id) {
  static constexpr auto kChainIdLookup =
      base::MakeFixedFlatMap<std::string_view, std::string_view>(
          {{brave_wallet::mojom::kMainnetChainId, "ethereum"},
           {brave_wallet::mojom::kSolanaMainnet, "solana"},
           {brave_wallet::mojom::kPolygonMainnetChainId, "polygon"},
           {brave_wallet::mojom::kBitcoinMainnet, "bitcoin"}});
  if (!kChainIdLookup.contains(chain_id)) {
    return std::nullopt;
  }

  return kChainIdLookup.at(chain_id);
}

}  // namespace

namespace brave_wallet {

namespace {

std::string CreatePricingRequestPayload(
    const std::vector<mojom::AssetPriceRequestPtr>& requests) {
  base::Value::List response;

  for (const auto& request : requests) {
    base::Value::Dict response_item;
    if (auto coin_str = GetStringFromCoinType(request->coin)) {
      response_item.Set("coin", *coin_str);
    } else {
      LOG(ERROR) << "Invalid coin type: " << request->coin;
      continue;
    }

    response_item.Set("chain_id", request->chain_id);

    if (request->address) {
      response_item.Set("address", *request->address);
    }

    response.Append(std::move(response_item));
  }

  std::string json_payload;
  base::JSONWriter::Write(response, &json_payload);
  return json_payload;
}

std::vector<mojom::AssetPricePtr> DummyPrices(
    const std::vector<mojom::AssetPriceRequestPtr>& requests,
    const std::string& vs_currency) {
  std::vector<mojom::AssetPricePtr> test_result;
  for (const auto& request : requests) {
    auto price = mojom::AssetPrice::New();
    price->coin = request->coin;
    price->chain_id = request->chain_id;
    if (request->address) {
      price->address = *request->address;
    }
    price->vs_currency = vs_currency;
    price->price = "1";
    price->cache_status = mojom::Gate3CacheStatus::kHit;
    price->source = mojom::AssetPriceSource::kCoingecko;
    price->percentage_change_24h = "0";  // Dummy timeframe change for testing
    test_result.push_back(std::move(price));
  }

  return test_result;
}

}  // namespace

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] GURL AssetRatioService::base_url_for_test_;

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

// static
GURL AssetRatioService::GetPriceURL(const std::string& vs_currency) {
  std::string base_url =
      base_url_for_test_.is_empty() ? kGate3URL : base_url_for_test_.spec();
  GURL url = GURL(absl::StrFormat("%s/api/pricing/v1/getPrices", base_url));
  url = net::AppendQueryParameter(url, "vs_currency",
                                  base::ToUpperASCII(vs_currency));
  return url;
}

// static
GURL AssetRatioService::GetPriceHistoryURL(
    const std::string& asset,
    const std::string& vs_asset,
    mojom::AssetPriceTimeframe timeframe) {
  std::string spec =
      absl::StrFormat("%s/v2/history/coingecko/%s/%s/%s",
                      base_url_for_test_.is_empty() ? GetAssetRatioBaseURL()
                                                    : base_url_for_test_.spec(),
                      asset, vs_asset, TimeFrameKeyToString(timeframe));
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

void AssetRatioService::GetPrice(
    std::vector<mojom::AssetPriceRequestPtr> requests,
    const std::string& vs_currency,
    GetPriceCallback callback) {
  if (dummy_prices_for_testing_) {
    std::move(callback).Run(true, DummyPrices(requests, vs_currency));
    return;
  }

  std::string json_payload = CreatePricingRequestPayload(requests);

  GURL url = GetPriceURL(vs_currency);

  auto conversion_callback = base::BindOnce(&ConvertAllNumbersToString, "");

  auto internal_callback =
      base::BindOnce(&AssetRatioService::OnGetPrice,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_request_helper_->Request(
      "POST", url, json_payload, "application/json",
      std::move(internal_callback), MakeBraveServicesKeyHeaders(),
      {.auto_retry_on_network_change = true, .enable_cache = true},
      std::move(conversion_callback));
}

void AssetRatioService::GetStripeBuyURL(
    GetBuyUrlV1Callback callback,
    const std::string& address,
    const std::string& source_currency,
    const std::string& source_exchange_amount,
    const std::string& chain_id,
    const std::string& destination_currency) {
  // Convert the frontend supplied chain ID to the chain ID used by Stripe
  std::optional<std::string_view> destination_network =
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

  GURL url = GURL(absl::StrFormat("%s/v2/stripe/onramp_sessions",
                                  base_url_for_test_.is_empty()
                                      ? GetAssetRatioBaseURL()
                                      : base_url_for_test_.spec()));

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

void AssetRatioService::OnGetPrice(GetPriceCallback callback,
                                   APIRequestResult api_request_result) {
  std::vector<mojom::AssetPricePtr> prices;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(false, std::move(prices));
    return;
  }

  prices = ParseAssetPrices(api_request_result.value_body());
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
  GURL url = GURL(absl::StrFormat("%s/v2/market/provider/coingecko",
                                  base_url_for_test_.is_empty()
                                      ? GetAssetRatioBaseURL()
                                      : base_url_for_test_.spec()));
  url = net::AppendQueryParameter(url, "vsCurrency", vs_asset);
  url = net::AppendQueryParameter(url, "limit", base::NumberToString(limit));
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
