/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class AssetRatioService : public KeyedService, public mojom::AssetRatioService {
 public:
  explicit AssetRatioService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~AssetRatioService() override;
  AssetRatioService(const AssetRatioService&) = delete;
  AssetRatioService& operator=(const AssetRatioService&) = delete;

  using APIRequestResult = api_request_helper::APIRequestResult;

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

  // Get sell URL for off-ramps
  void GetSellUrl(mojom::OffRampProvider provider,
                  const std::string& chain_id,
                  const std::string& symbol,
                  const std::string& amount,
                  const std::string& currency_code,
                  GetSellUrlCallback callback) override;

  // mojom::AssetRatioService
  void GetPrice(const std::vector<std::string>& from_assets,
                const std::vector<std::string>& to_assets,
                mojom::AssetPriceTimeframe timeframe,
                GetPriceCallback callback) override;

  // The asset and vs_asset params are strings like: "bat"
  void GetPriceHistory(const std::string& asset,
                       const std::string& vs_asset,
                       mojom::AssetPriceTimeframe timeframe,
                       GetPriceHistoryCallback callback) override;
  void GetCoinMarkets(const std::string& vs_asset,
                      const uint8_t limit,
                      GetCoinMarketsCallback callback) override;

  static GURL GetSardineBuyURL(const std::string& network,
                               const std::string& address,
                               const std::string& symbol,
                               const std::string& amount,
                               const std::string& currency_code,
                               const std::string& auth_token);

  static GURL GetPriceURL(const std::vector<std::string>& from_assets,
                          const std::vector<std::string>& to_assets,
                          mojom::AssetPriceTimeframe timeframe);
  static GURL GetPriceHistoryURL(const std::string& asset,
                                 const std::string& vs_asset,
                                 mojom::AssetPriceTimeframe timeframe);
  static GURL GetCoinMarketsURL(const std::string& vs_asset, uint8_t limit);

  static void SetBaseURLForTest(const GURL& base_url_for_test);
  void SetAPIRequestHelperForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  void EnableDummyPricesForTesting();

 private:
  friend class AssetRatioServiceUnitTest;
  FRIEND_TEST_ALL_PREFIXES(AssetRatioServiceUnitTest, GetStripeBuyURL);
  void OnGetSardineAuthToken(const std::string& network,
                             const std::string& address,
                             const std::string& symbol,
                             const std::string& amount,
                             const std::string& currency_code,
                             GetBuyUrlV1Callback callback,
                             APIRequestResult api_request_result);

  void GetStripeBuyURL(GetBuyUrlV1Callback callback,
                       const std::string& address,
                       const std::string& source_currency,
                       const std::string& source_exchange_amount,
                       const std::string& chain_id,
                       const std::string& destination_currency);

  void OnGetStripeBuyURL(GetBuyUrlV1Callback callback,
                         APIRequestResult api_request_result);

  void OnGetPrice(std::vector<std::string> from_assets,
                  std::vector<std::string> to_assets,
                  GetPriceCallback callback,
                  APIRequestResult api_request_result);
  void OnGetPriceHistory(GetPriceHistoryCallback callback,
                         APIRequestResult api_request_result);

  void OnGetCoinMarkets(GetCoinMarketsCallback callback,
                        APIRequestResult api_request_result);

  mojo::ReceiverSet<mojom::AssetRatioService> receivers_;

  static GURL base_url_for_test_;
  bool dummy_prices_for_testing_ = false;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  base::WeakPtrFactory<AssetRatioService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_SERVICE_H_
