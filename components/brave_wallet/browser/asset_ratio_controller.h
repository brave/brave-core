/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "url/gurl.h"

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class AssetRatioController : public KeyedService,
                             public mojom::AssetRatioController {
 public:
  AssetRatioController(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~AssetRatioController() override;
  AssetRatioController(const AssetRatioController&) = delete;
  AssetRatioController& operator=(const AssetRatioController&) = delete;

  mojo::PendingRemote<mojom::AssetRatioController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::AssetRatioController> receiver);

  void GetPrice(const std::vector<std::string>& from_assets,
                const std::vector<std::string>& to_assets,
                brave_wallet::mojom::AssetPriceTimeframe timeframe,
                GetPriceCallback callback) override;
  // The asset param is a string like: "bat"
  void GetPriceHistory(const std::string& asset,
                       brave_wallet::mojom::AssetPriceTimeframe timeframe,
                       GetPriceHistoryCallback callback) override;

  static GURL GetPriceURL(const std::vector<std::string>& from_assets,
                          const std::vector<std::string>& to_assets,
                          brave_wallet::mojom::AssetPriceTimeframe timeframe);
  static GURL GetPriceHistoryURL(
      const std::string& asset,
      brave_wallet::mojom::AssetPriceTimeframe timeframe);
  static void SetBaseURLForTest(const GURL& base_url_for_test);

 private:
  void OnGetPrice(std::vector<std::string> from_assets,
                  std::vector<std::string> to_assets,
                  GetPriceCallback callback,
                  const int status,
                  const std::string& body,
                  const base::flat_map<std::string, std::string>& headers);
  void OnGetPriceHistory(
      GetPriceHistoryCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  mojo::ReceiverSet<mojom::AssetRatioController> receivers_;

  static GURL base_url_for_test_;
  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<AssetRatioController> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_CONTROLLER_H_
