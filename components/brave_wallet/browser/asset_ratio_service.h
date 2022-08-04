/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_SERVICE_H_

#include <memory>
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

class AssetRatioService : public KeyedService, public mojom::AssetRatioService {
 public:
  AssetRatioService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~AssetRatioService() override;
  AssetRatioService(const AssetRatioService&) = delete;
  AssetRatioService& operator=(const AssetRatioService&) = delete;

  mojo::PendingRemote<mojom::AssetRatioService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::AssetRatioService> receiver);

  // Get buy URL for on-ramps
  void GetBuyUrlV1(mojom::OnRampProvider provider,
                   const std::string& chain_id,
                   const std::string& address,
                   const std::string& symbol,
                   const std::string& amount,
                   const std::string& currency_code,
                   GetBuyUrlV1Callback callback) override;

  // mojom::AssetRatioService
  void GetPrice(const std::vector<std::string>& from_assets,
                const std::vector<std::string>& to_assets,
                brave_wallet::mojom::AssetPriceTimeframe timeframe,
                GetPriceCallback callback) override;

  // The asset and vs_asset params are strings like: "bat"
  void GetPriceHistory(const std::string& asset,
                       const std::string& vs_asset,
                       brave_wallet::mojom::AssetPriceTimeframe timeframe,
                       GetPriceHistoryCallback callback) override;
  void GetTokenInfo(const std::string& contract_address,
                    GetTokenInfoCallback callback) override;

  static GURL GetSardineBuyURL(const std::string network,
                               const std::string address,
                               const std::string symbol,
                               const std::string amount,
                               const std::string currency_code,
                               const std::string auth_token);

  static GURL GetPriceURL(const std::vector<std::string>& from_assets,
                          const std::vector<std::string>& to_assets,
                          brave_wallet::mojom::AssetPriceTimeframe timeframe);
  static GURL GetPriceHistoryURL(
      const std::string& asset,
      const std::string& vs_asset,
      brave_wallet::mojom::AssetPriceTimeframe timeframe);
  static GURL GetTokenInfoURL(const std::string& contract_address);

  static void SetBaseURLForTest(const GURL& base_url_for_test);
  void SetAPIRequestHelperForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  void OnGetSardineAuthToken(
      const std::string& network,
      const std::string& address,
      const std::string& symbol,
      const std::string& amount,
      const std::string& currency_code,
      GetBuyUrlV1Callback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

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

  void OnGetTokenInfo(GetTokenInfoCallback callback,
                      const int status,
                      const std::string& body,
                      const base::flat_map<std::string, std::string>& headers);

  mojo::ReceiverSet<mojom::AssetRatioService> receivers_;

  static GURL base_url_for_test_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  base::WeakPtrFactory<AssetRatioService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_SERVICE_H_
