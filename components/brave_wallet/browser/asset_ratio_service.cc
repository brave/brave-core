/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_service.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/environment.h"
#include "base/strings/stringprintf.h"
#include "brave/common/brave_services_key.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
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

AssetRatioService::~AssetRatioService() {}

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
GURL AssetRatioService::GetEstimatedTimeURL(const std::string& gas_price) {
  std::string spec = base::StringPrintf(
      "%sv2/etherscan/"
      "passthrough?module=gastracker&action=gasestimate&gasprice=%s",
      base_url_for_test_.is_empty() ? kAssetRatioBaseURL
                                    : base_url_for_test_.spec().c_str(),
      gas_price.c_str());
  return GURL(spec);
}

void AssetRatioService::GetEstimatedTime(const std::string& gas_price,
                                         GetEstimatedTimeCallback callback) {
  auto internal_callback =
      base::BindOnce(&AssetRatioService::OnGetEstimatedTime,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_->Request("GET", GetEstimatedTimeURL(gas_price), "", "",
                               true, std::move(internal_callback));
}

void AssetRatioService::OnGetEstimatedTime(
    GetEstimatedTimeCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }

  const std::string seconds = ParseEstimatedTime(body);
  if (seconds.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, seconds);
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

}  // namespace brave_wallet
