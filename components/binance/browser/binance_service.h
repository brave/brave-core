/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_
#define BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class Profile;

const char oauth_path_access_token[] = "/oauth/token";
const char oauth_path_account_balances[] = "/oauth-api/v1/balance";
const char oauth_path_convert_assets[] = "/oauth-api/v1/ocbs/support-coins";
const char oauth_path_convert_quote[] = "/oauth-api/v1/ocbs/quote";
const char oauth_path_convert_confirm[] = "/oauth-api/v1/ocbs/confirm";
const char oauth_path_deposit_info[] = "/oauth-api/v1/get-charge-address";
const char oauth_path_revoke_token[] = "/oauth-api/v1/revoke-token";
const char binance_com_refcode[] = "39346846";

const char gateway_path_networks[] =
    "/gateway-api/v1/public/capital/getNetworkCoinAll";

typedef std::map<std::string, std::string> BinanceCoinNetworks;
typedef std::map<std::string, std::vector<std::string>> BinanceAccountBalances;
typedef std::map<std::string, std::vector<std::map<std::string, std::string>>>
    BinanceConvertAsserts;

class BinanceService : public KeyedService {
 public:
  explicit BinanceService(content::BrowserContext* context);
  BinanceService(const BinanceService&) = delete;
  BinanceService& operator=(const BinanceService&) = delete;
  ~BinanceService() override;

  using GetAccessTokenCallback = base::OnceCallback<void(bool)>;
  using GetConvertQuoteCallback = base::OnceCallback<void(const std::string&,
                                                          const std::string&,
                                                          const std::string&,
                                                          const std::string&)>;
  using GetAccountBalancesCallback = base::OnceCallback<
      void(const BinanceAccountBalances&, bool success)>;
  using GetDepositInfoCallback = base::OnceCallback<void(const std::string&,
                                                         const std::string&,
                                                         bool success)>;
  using ConfirmConvertCallback = base::OnceCallback<void(bool,
                                                         const std::string&)>;
  using GetConvertAssetsCallback = base::OnceCallback<
      void(const BinanceConvertAsserts&)>;
  using RevokeTokenCallback = base::OnceCallback<void(bool)>;
  using GetCoinNetworksCallback = base::OnceCallback<
        void(const BinanceCoinNetworks&)>;

  bool GetAccessToken(GetAccessTokenCallback callback);
  bool IsSupportedRegion();
  std::string GetLocaleForURL();
  bool GetConvertQuote(const std::string& from,
      const std::string& to,
      const std::string& amount,
      GetConvertQuoteCallback callback);
  bool GetAccountBalances(GetAccountBalancesCallback callback);
  bool GetDepositInfo(const std::string& symbol,
      const std::string& ticker_network,
      GetDepositInfoCallback callback);
  bool ConfirmConvert(const std::string& quote_id,
      ConfirmConvertCallback callback);
  bool GetConvertAssets(GetConvertAssetsCallback callback);
  bool RevokeToken(RevokeTokenCallback callback);
  bool GetCoinNetworks(GetCoinNetworksCallback callback);

  std::string GetBinanceTLD();
  std::string GetOAuthClientUrl();
  void SetAuthToken(const std::string& auth_token);

 private:
  static GURL oauth_endpoint_;
  static GURL api_endpoint_;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  bool SetAccessTokens(const std::string& access_token,
                       const std::string& refresh_token);
  void ResetAccessTokens();

  using URLRequestCallback =
      base::OnceCallback<void(const int, const std::string&,
                              const std::map<std::string, std::string>&)>;

  base::SequencedTaskRunner* io_task_runner();
  void OnGetAccessToken(GetAccessTokenCallback callback,
                           const int status, const std::string& body,
                           const std::map<std::string, std::string>& headers);
  void OnGetConvertQuote(GetConvertQuoteCallback callback,
                           const int status, const std::string& body,
                           const std::map<std::string, std::string>& headers);
  void OnGetAccountBalances(GetAccountBalancesCallback callback,
                           const int status, const std::string& body,
                           const std::map<std::string, std::string>& headers);
  void OnGetDepositInfo(GetDepositInfoCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnConfirmConvert(ConfirmConvertCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnGetConvertAssets(GetConvertAssetsCallback callback,
                          const int status, const std::string& body,
                          const std::map<std::string, std::string>& headers);
  void OnRevokeToken(RevokeTokenCallback callback,
                     const int status, const std::string& body,
                     const std::map<std::string, std::string>& headers);
  void OnGetCoinNetworks(GetCoinNetworksCallback callback,
        const int status, const std::string& body,
        const std::map<std::string, std::string>& headers);
  bool OAuthRequest(const GURL& url, const std::string& method,
      const std::string& post_data, URLRequestCallback callback,
      bool auto_retry_on_network_change, bool save_send_cookies);
  bool LoadTokensFromPrefs();
  void OnURLLoaderComplete(
      SimpleURLLoaderList::iterator iter,
      URLRequestCallback callback,
      const std::unique_ptr<std::string> response_body);
  void SetClientIdForTest(const std::string& client_id);
  void SetOAuthHostForTest(const std::string& oauth_host);
  void SetGatewayHostForTest(const std::string& gateway_host);

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  std::string auth_token_;
  std::string access_token_;
  std::string refresh_token_;
  std::string code_challenge_;
  std::string code_verifier_;
  std::string client_id_;
  std::string oauth_host_;
  std::string gateway_host_;

  raw_ptr<content::BrowserContext> context_ = nullptr;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
  base::WeakPtrFactory<BinanceService> weak_factory_;

  FRIEND_TEST_ALL_PREFIXES(BinanceAPIBrowserTest, GetOAuthClientURL);
  FRIEND_TEST_ALL_PREFIXES(BinanceAPIBrowserTest,
      SetAndGetAuthTokenRevokesPref);
  friend class BinanceAPIBrowserTest;
};

#endif  // BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_
