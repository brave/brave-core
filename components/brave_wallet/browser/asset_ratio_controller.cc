/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"

#include <time.h>

#include <utility>

#include "base/strings/stringprintf.h"
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

std::string DateToString(const base::Time time) {
  time_t ts = time.ToTimeT();
  tm timestamp;
#ifdef OS_WIN
  gmtime_s(&timestamp, &ts);
#else
  gmtime_r(&ts, &timestamp);
#endif
  char sz[2048];
  strftime(sz, sizeof(sz), "%G", &timestamp);
  return sz;
}

}  // namespace

namespace brave_wallet {

GURL AssetRatioController::base_url_for_test_;

AssetRatioController::AssetRatioController(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      weak_ptr_factory_(this) {}

AssetRatioController::~AssetRatioController() {}

void AssetRatioController::SetBaseURLForTest(const GURL& base_url_for_test) {
  base_url_for_test_ = base_url_for_test;
}

// static
GURL AssetRatioController::GetPriceURL(const std::string& asset) {
  std::string passthrough = base::StringPrintf(
      "/api/v3/simple/price?ids=%s&vs_currencies=usd", asset.c_str());
  passthrough = net::EscapeQueryParamValue(passthrough, false);
  std::string spec = base::StringPrintf(
      "%sv2/coingecko/passthrough?path=%s",
      base_url_for_test_.is_empty() ? "https://bat-ratios.herokuapp.com/"
                                    : base_url_for_test_.spec().c_str(),
      passthrough.c_str());
  return GURL(spec);
}

// static
GURL AssetRatioController::GetPriceHistoryURL(const std::string& asset,
                                              base::Time from_time,
                                              base::Time to_time) {
  std::string spec = base::StringPrintf(
      "%sv2/history/coingecko/%s/usd/%s/%s",
      base_url_for_test_.is_empty() ? "https://bat-ratios.herokuapp.com/"
                                    : base_url_for_test_.spec().c_str(),
      asset.c_str(), DateToString(from_time).c_str(),
      DateToString(to_time).c_str());
  return GURL(spec);
}

void AssetRatioController::GetPrice(const std::string& asset,
                                    GetPriceCallback callback) {
  auto internal_callback =
      base::BindOnce(&AssetRatioController::OnGetPrice,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request("GET", GetPriceURL(asset), "", "", true,
                              std::move(internal_callback));
}

void AssetRatioController::OnGetPrice(
    GetPriceCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }
  std::string price;
  if (!ParseAssetPrice(body, &price)) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, price);
}

void AssetRatioController::GetPriceHistory(const std::string& asset,
                                           base::Time from_time,
                                           base::Time to_time,
                                           GetPriceHistoryCallback callback) {
  auto internal_callback =
      base::BindOnce(&AssetRatioController::OnGetPriceHistory,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_.Request("GET",
                              GetPriceHistoryURL(asset, from_time, to_time), "",
                              "", true, std::move(internal_callback));
}

void AssetRatioController::OnGetPriceHistory(
    GetPriceHistoryCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::vector<AssetTimePrice> values;
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, values);
    return;
  }
  if (!ParseAssetPriceHistory(body, &values)) {
    std::move(callback).Run(false, values);
    return;
  }

  std::move(callback).Run(true, values);
}

}  // namespace brave_wallet
