/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_service.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/environment.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
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
  std::for_each(assets.begin(), assets.end(), [&ss](const std::string asset) {
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

}  // namespace

namespace brave_wallet {

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
  api_request_helper_.reset(new api_request_helper::APIRequestHelper(
      GetNetworkTrafficAnnotationTag(), url_loader_factory));
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

GURL AssetRatioService::GetSardineBuyURL(const std::string chain_id,
                                         const std::string address,
                                         const std::string symbol,
                                         const std::string amount,
                                         const std::string currency_code,
                                         const std::string auth_token) {
  const std::string sardine_network_name = GetSardineNetworkName(chain_id);
  GURL url = GURL(kSardineStorefrontBaseURL);
  url = net::AppendQueryParameter(url, "address", address);
  url = net::AppendQueryParameter(url, "network", sardine_network_name);
  url = net::AppendQueryParameter(url, "asset_type", symbol);
  url = net::AppendQueryParameter(url, "fiat_amount", amount);
  url = net::AppendQueryParameter(url, "fiat_currency", currency_code);
  url = net::AppendQueryParameter(url, "client_token", auth_token);
  return url;
}

// static
GURL AssetRatioService::GetPriceURL(
    const std::vector<std::string>& from_assets,
    const std::vector<std::string>& to_assets,
    brave_wallet::mojom::AssetPriceTimeframe timeframe) {
  std::string from = VectorToCommaSeparatedList(from_assets);
  std::string to = VectorToCommaSeparatedList(to_assets);
  std::string spec = base::StringPrintf(
      "%sv2/relative/provider/coingecko/%s/%s/%s",
      base_url_for_test_.is_empty() ? kAssetRatioBaseURL
                                    : base_url_for_test_.spec().c_str(),
      from.c_str(), to.c_str(), TimeFrameKeyToString(timeframe).c_str());
  return GURL(spec);
}

// static
GURL AssetRatioService::GetPriceHistoryURL(
    const std::string& asset,
    const std::string& vs_asset,
    brave_wallet::mojom::AssetPriceTimeframe timeframe) {
  std::string spec = base::StringPrintf(
      "%sv2/history/coingecko/%s/%s/%s",
      base_url_for_test_.is_empty() ? kAssetRatioBaseURL
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
  if (provider == mojom::OnRampProvider::kWyre) {
    GURL url = GURL(kWyreBaseUrl);
    url = net::AppendQueryParameter(url, "dest", "ethereum:" + address);
    url = net::AppendQueryParameter(url, "sourceCurrency", currency_code);
    url = net::AppendQueryParameter(url, "destCurrency", symbol);
    url = net::AppendQueryParameter(url, "amount", amount);
    url = net::AppendQueryParameter(url, "accountId", kWyreID);
    url = net::AppendQueryParameter(url, "paymentMethod", "debit-card");
    std::move(callback).Run(std::move(url.spec()), absl::nullopt);
  } else if (provider == mojom::OnRampProvider::kRamp) {
    GURL url = GURL(kRampBaseUrl);
    url = net::AppendQueryParameter(url, "userAddress", address);
    url = net::AppendQueryParameter(url, "swapAsset", symbol);
    url = net::AppendQueryParameter(url, "fiatValue", amount);
    url = net::AppendQueryParameter(url, "fiatCurrency", currency_code);
    url = net::AppendQueryParameter(url, "hostApiKey", kRampID);
    std::move(callback).Run(std::move(url.spec()), absl::nullopt);
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
    std::string base64_credentials;
    std::string credentials = base::StringPrintf(
        "%s:%s", sardine_client_id.c_str(),  // username:password
        sardine_client_secret.c_str());
    base::Base64Encode(credentials, &base64_credentials);
    std::string header =
        base::StringPrintf("Basic %s", base64_credentials.c_str());
    request_headers["Authorization"] = std::move(header);
    api_request_helper_->Request("POST", sardine_token_url, payload,
                                 "application/json", true,
                                 std::move(internal_callback), request_headers);
  } else {
    std::move(callback).Run(url, "UNSUPPORTED_ONRAMP_PROVIDER");
  }
}

void AssetRatioService::GetPrice(
    const std::vector<std::string>& from_assets,
    const std::vector<std::string>& to_assets,
    brave_wallet::mojom::AssetPriceTimeframe timeframe,
    GetPriceCallback callback) {
  std::vector<std::string> from_assets_lower = VectorToLowerCase(from_assets);
  std::vector<std::string> to_assets_lower = VectorToLowerCase(to_assets);
  auto internal_callback = base::BindOnce(
      &AssetRatioService::OnGetPrice, weak_ptr_factory_.GetWeakPtr(),
      from_assets_lower, to_assets_lower, std::move(callback));

  base::flat_map<std::string, std::string> request_headers;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string brave_key(BUILDFLAG(BRAVE_SERVICES_KEY));
  if (env->HasVar("BRAVE_SERVICES_KEY")) {
    env->GetVar("BRAVE_SERVICES_KEY", &brave_key);
  }
  request_headers["x-brave-key"] = std::move(brave_key);

  api_request_helper_->Request(
      "GET", GetPriceURL(from_assets_lower, to_assets_lower, timeframe), "", "",
      true, std::move(internal_callback), request_headers);
}

void AssetRatioService::OnGetSardineAuthToken(
    const std::string& chain_id,
    const std::string& address,
    const std::string& symbol,
    const std::string& amount,
    const std::string& currency_code,
    GetBuyUrlV1Callback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run("", "INTERNAL_SERVICE_ERROR");
    return;
  }

  auto auth_token = ParseSardineAuthToken(body);
  if (!auth_token) {
    std::move(callback).Run("", "INTERNAL_SERVICE_ERROR");
    return;
  }

  GURL sardine_buy_url = GetSardineBuyURL(chain_id, address, symbol, amount,
                                          currency_code, *auth_token);
  std::move(callback).Run(std::move(sardine_buy_url.spec()), absl::nullopt);
}

void AssetRatioService::OnGetPrice(
    std::vector<std::string> from_assets,
    std::vector<std::string> to_assets,
    GetPriceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  std::vector<brave_wallet::mojom::AssetPricePtr> prices;
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, std::move(prices));
    return;
  }
  if (!ParseAssetPrice(body, from_assets, to_assets, &prices)) {
    std::move(callback).Run(false, std::move(prices));
    return;
  }

  std::move(callback).Run(true, std::move(prices));
}

void AssetRatioService::GetPriceHistory(
    const std::string& asset,
    const std::string& vs_asset,
    brave_wallet::mojom::AssetPriceTimeframe timeframe,
    GetPriceHistoryCallback callback) {
  std::string asset_lower = base::ToLowerASCII(asset);
  std::string vs_asset_lower = base::ToLowerASCII(vs_asset);
  auto internal_callback =
      base::BindOnce(&AssetRatioService::OnGetPriceHistory,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_->Request(
      "GET", GetPriceHistoryURL(asset_lower, vs_asset_lower, timeframe), "", "",
      true, std::move(internal_callback));
}

void AssetRatioService::OnGetPriceHistory(
    GetPriceHistoryCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  std::vector<brave_wallet::mojom::AssetTimePricePtr> values;
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, std::move(values));
    return;
  }
  if (!ParseAssetPriceHistory(body, &values)) {
    std::move(callback).Run(false, std::move(values));
    return;
  }

  std::move(callback).Run(true, std::move(values));
}

// static
GURL AssetRatioService::GetTokenInfoURL(const std::string& contract_address) {
  std::string spec = base::StringPrintf(
      "%sv2/etherscan/"
      "passthrough?module=token&action=tokeninfo&contractaddress=%s",
      base_url_for_test_.is_empty() ? kAssetRatioBaseURL
                                    : base_url_for_test_.spec().c_str(),
      contract_address.c_str());
  return GURL(spec);
}

void AssetRatioService::GetTokenInfo(const std::string& contract_address,
                                     GetTokenInfoCallback callback) {
  auto internal_callback =
      base::BindOnce(&AssetRatioService::OnGetTokenInfo,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_->Request("GET", GetTokenInfoURL(contract_address), "", "",
                               true, std::move(internal_callback));
}

void AssetRatioService::OnGetTokenInfo(
    GetTokenInfoCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(
      ParseTokenInfo(body, mojom::kMainnetChainId, mojom::CoinType::ETH));
}

// static
GURL AssetRatioService::GetCoinMarketsURL(const std::string& vs_asset,
                                          const uint8_t limit) {
  GURL url = GURL(base::StringPrintf("%sv2/market/provider/coingecko",
                                     base_url_for_test_.is_empty()
                                         ? kAssetRatioBaseURL
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
  api_request_helper_->Request("GET", GetCoinMarketsURL(vs_asset_lower, limit),
                               "", "", true, std::move(internal_callback));
}

void AssetRatioService::OnGetCoinMarkets(
    GetCoinMarketsCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  std::vector<brave_wallet::mojom::CoinMarketPtr> values;
  if (status != 200) {
    std::move(callback).Run(false, std::move(values));
    return;
  }

  if (!ParseCoinMarkets(body, &values)) {
    std::move(callback).Run(false, std::move(values));
    return;
  }

  std::move(callback).Run(true, std::move(values));
}

}  // namespace brave_wallet
