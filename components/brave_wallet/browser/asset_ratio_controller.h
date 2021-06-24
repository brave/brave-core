/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_CONTROLLER_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class AssetRatioController {
 public:
  AssetRatioController(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~AssetRatioController();

  using GetPriceCallback =
      base::OnceCallback<void(bool status, const std::string& price)>;
  void GetPrice(const std::string& asset, GetPriceCallback callback);

  using GetPriceHistoryCallback = base::OnceCallback<void(
      bool status,
      std::vector<brave_wallet::mojom::AssetTimePricePtr> values)>;
  // The asset param is a string like: "basic-attention-token"
  void GetPriceHistory(const std::string& asset,
                       brave_wallet::mojom::AssetPriceTimeframe timeframe,
                       GetPriceHistoryCallback callback);
  static GURL GetPriceURL(const std::string& asset);
  static GURL GetPriceHistoryURL(
      const std::string& asset,
      brave_wallet::mojom::AssetPriceTimeframe timeframe);
  static void SetBaseURLForTest(const GURL& base_url_for_test);

 private:
  void OnGetPrice(GetPriceCallback callback,
                  const int status,
                  const std::string& body,
                  const std::map<std::string, std::string>& headers);
  void OnGetPriceHistory(GetPriceHistoryCallback callback,
                         const int status,
                         const std::string& body,
                         const std::map<std::string, std::string>& headers);

  static GURL base_url_for_test_;
  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<AssetRatioController> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_CONTROLLER_H_
