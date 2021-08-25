/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("asset_ratio_controller", R"(
      semantics {
        sender: "Asset Ratio Controller"
        description:
          "This controller is used to obtain asset prices for the Brave wallet."
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

}  // namespace

namespace brave_wallet {

GURL AssetRatioController::base_url_for_test_;

AssetRatioController::AssetRatioController(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      weak_ptr_factory_(this) {}

AssetRatioController::~AssetRatioController() {}

mojo::PendingRemote<mojom::AssetRatioController>
AssetRatioController::MakeRemote() {
  mojo::PendingRemote<mojom::AssetRatioController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void AssetRatioController::Bind(
    mojo::PendingReceiver<mojom::AssetRatioController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void AssetRatioController::SetBaseURLForTest(const GURL& base_url_for_test) {
  base_url_for_test_ = base_url_for_test;
}

// static
GURL AssetRatioController::GetPriceURL(
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
GURL AssetRatioController::GetPriceHistoryURL(
    const std::string& asset,
    brave_wallet::mojom::AssetPriceTimeframe timeframe) {
  std::string spec = base::StringPrintf(
      "%sv2/history/coingecko/%s/usd/%s",
      base_url_for_test_.is_empty() ? kAssetRatioBaseURL
                                    : base_url_for_test_.spec().c_str(),
      asset.c_str(), TimeFrameKeyToString(timeframe).c_str());
  return GURL(spec);
}

void AssetRatioController::GetPrice(
    const std::vector<std::string>& from_assets,
    const std::vector<std::string>& to_assets,
    brave_wallet::mojom::AssetPriceTimeframe timeframe,
    GetPriceCallback callback) {
  auto internal_callback = base::BindOnce(
      &AssetRatioController::OnGetPrice, weak_ptr_factory_.GetWeakPtr(),
      from_assets, to_assets, std::move(callback));
  api_request_helper_.Request("GET",
                              GetPriceURL(from_assets, to_assets, timeframe),
                              "", "", true, std::move(internal_callback));
}

void AssetRatioController::OnGetPrice(
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

void AssetRatioController::GetPriceHistory(
    const std::string& asset,
    brave_wallet::mojom::AssetPriceTimeframe timeframe,
    GetPriceHistoryCallback callback) {
  auto internal_callback =
      base::BindOnce(&AssetRatioController::OnGetPriceHistory,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request("GET", GetPriceHistoryURL(asset, timeframe), "",
                              "", true, std::move(internal_callback));
}

void AssetRatioController::OnGetPriceHistory(
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

}  // namespace brave_wallet
