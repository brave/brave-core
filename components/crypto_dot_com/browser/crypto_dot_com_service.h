/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_CRYPTO_DOT_COM_SERVICE_H_
#define BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_CRYPTO_DOT_COM_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefService;

namespace base {
class FilePath;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace net {
class HttpRequestHeaders;
}  // namespace net

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

const char get_ticker_info_path[] = "/v2/public/get-ticker";
const char get_chart_data_path[] = "/v2/public/get-candlestick";
const char get_pairs_path[] = "/v2/public/get-instruments";
const char get_gainers_losers_path[] = "/fe-ex-api/widget/get-gainers";

typedef std::map<std::string, double> CryptoDotComTickerInfo;
typedef std::vector<std::map<std::string, double>> CryptoDotComChartData;
typedef std::vector<std::map<std::string, std::string>>
    CryptoDotComSupportedPairs;
typedef std::map<std::string, std::vector<std::map<std::string, std::string>>>
    CryptoDotComAssetRankings;

class CryptoDotComService : public KeyedService {
 public:
  CryptoDotComService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);
  ~CryptoDotComService() override;

  CryptoDotComService(const CryptoDotComService&) = delete;
  CryptoDotComService& operator=(const CryptoDotComService&) = delete;

  using GetTickerInfoCallback =
        base::OnceCallback<void(const CryptoDotComTickerInfo&)>;
  using GetChartDataCallback =
        base::OnceCallback<void(const CryptoDotComChartData&)>;
  using GetSupportedPairsCallback =
        base::OnceCallback<void(const CryptoDotComSupportedPairs&)>;
  using GetAssetRankingsCallback =
        base::OnceCallback<void(const CryptoDotComAssetRankings&)>;
  using GetAccountBalancesCallback = base::OnceCallback<void(base::Value)>;
  using IsConnectedCallback = base::OnceCallback<void(bool)>;
  using DisconnectCallback = base::OnceCallback<void(bool)>;
  using GetNewsEventsCallback = base::OnceCallback<void(base::Value)>;
  using GetDepositAddressCallback = base::OnceCallback<void(base::Value)>;
  using CreateMarketOrderCallback = base::OnceCallback<void(base::Value)>;

  bool GetTickerInfo(const std::string& asset,
                     GetTickerInfoCallback callback);
  bool GetChartData(const std::string& asset,
                    GetChartDataCallback callback);
  bool GetSupportedPairs(GetSupportedPairsCallback callback);
  bool GetAssetRankings(GetAssetRankingsCallback callback);
  bool GetAccountBalances(GetAccountBalancesCallback callback);
  bool IsConnected(IsConnectedCallback callback);
  bool Disconnect(DisconnectCallback callback);
  bool IsLoggedIn();
  bool GetDepositAddress(const std::string& asset,
                         GetDepositAddressCallback callback);
  bool CreateMarketOrder(base::Value order, CreateMarketOrderCallback callback);
  bool GetNewsEvents(GetNewsEventsCallback callback);
  std::string GetAuthClientUrl() const;

  bool SetAccessToken(const std::string& access_token);

 private:
  friend class CryptoDotComAPIBrowserTest;

  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  using URLRequestCallback =
      base::OnceCallback<void(const int, const std::string&,
                              const std::map<std::string, std::string>&)>;

  void OnTickerInfo(GetTickerInfoCallback callback,
                    const int status, const std::string& body,
                    const std::map<std::string, std::string>& headers);
  void OnChartData(GetChartDataCallback callback,
                   const int status, const std::string& body,
                   const std::map<std::string, std::string>& headers);
  void OnSupportedPairs(GetSupportedPairsCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnAssetRankings(GetAssetRankingsCallback callback,
                       const int status, const std::string& body,
                       const std::map<std::string, std::string>& headers);
  void OnGetAccountBalances(GetAccountBalancesCallback callback,
                            const int status,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers);
  void OnIsConnected(IsConnectedCallback callback,
                     const int status,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers);
  // Delete |asset| args when crypto.com api is fixed.
  void OnGetDepositAddress(GetDepositAddressCallback callback,
                           const std::string& asset,
                           const int status,
                           const std::string& body,
                           const std::map<std::string, std::string>& headers);
  void OnCreateMarketOrder(CreateMarketOrderCallback callback,
                           const int status,
                           const std::string& body,
                           const std::map<std::string, std::string>& headers);
  void OnGetNewsEvents(GetNewsEventsCallback callback,
                       const int status,
                       const std::string& body,
                       const std::map<std::string, std::string>& headers);
  void OnDisconnect(DisconnectCallback callback,
                    const int status,
                    const std::string& body,
                    const std::map<std::string, std::string>& headers);

  bool LoadTokenFromPrefs();

  bool NetworkRequest(const GURL& url,
                      const std::string& method,
                      const std::string& post_data,
                      const net::HttpRequestHeaders& headers,
                      URLRequestCallback callback);

  void OnURLLoaderComplete(
      SimpleURLLoaderList::iterator iter,
      URLRequestCallback callback,
      const std::unique_ptr<std::string> response_body);

  std::string access_token_;
  PrefService* prefs_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
};

#endif  // BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_CRYPTO_DOT_COM_SERVICE_H_
