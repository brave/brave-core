/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GEMINI_BROWSER_GEMINI_SERVICE_H_
#define BRAVE_COMPONENTS_GEMINI_BROWSER_GEMINI_SERVICE_H_

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

const char auth_path_access_token[] = "/auth/token";
const char api_path_account_balances[] = "/v1/balances";
const char api_path_account_addresses[] = "/v1/addresses";
const char api_path_get_quote[] = "/v1/instant/quote";
const char api_path_execute_quote[] = "/v1/instant/execute";
const char api_path_ticker_price[] = "/v1/pubticker";
const char api_path_revoke_token[] = "/v1/oauth/revokeByToken";

typedef std::map<std::string, std::string> GeminiAccountBalances;

class GeminiService : public KeyedService {
 public:
  explicit GeminiService(content::BrowserContext* context);
  GeminiService(const GeminiService&) = delete;
  GeminiService& operator=(const GeminiService&) = delete;
  ~GeminiService() override;

  // Callbacks
  using AccessTokenCallback = base::OnceCallback<void(bool)>;
  using GetTickerPriceCallback = base::OnceCallback<void(const std::string&)>;
  using URLRequestCallback =
      base::OnceCallback<void(const int, const std::string&,
                              const std::map<std::string, std::string>&)>;
  using GetAccountBalancesCallback = base::OnceCallback<
      void(const GeminiAccountBalances&, bool)>;
  using GetDepositInfoCallback = base::OnceCallback<void(const std::string&)>;
  using RevokeAccessTokenCallback = base::OnceCallback<void(bool)>;
  using GetOrderQuoteCallback = base::OnceCallback<void(const std::string&,
                                                        const std::string&,
                                                        const std::string&,
                                                        const std::string&,
                                                        const std::string&,
                                                        const std::string&)>;
  using ExecuteOrderCallback = base::OnceCallback<void(bool)>;

  std::string GetOAuthClientUrl();
  void SetAuthToken(const std::string& auth_token);
  bool GetAccessToken(AccessTokenCallback callback);
  bool RefreshAccessToken(AccessTokenCallback callback);
  bool GetTickerPrice(const std::string& asset,
                      GetTickerPriceCallback callback);
  bool GetAccountBalances(GetAccountBalancesCallback callback);
  bool GetDepositInfo(const std::string& asset,
                      GetDepositInfoCallback callback);
  bool RevokeAccessToken(RevokeAccessTokenCallback callback);
  bool GetOrderQuote(const std::string& side,
                     const std::string& symbol,
                     const std::string& spend,
                     GetOrderQuoteCallback callback);
  bool ExecuteOrder(const std::string& symbol,
                    const std::string& side,
                    const std::string& quantity,
                    const std::string& price,
                    const std::string& fee,
                    const int quote_id,
                    ExecuteOrderCallback callback);

  void SetClientIdForTest(const std::string& client_id);
  void SetClientSecretForTest(const std::string& client_secret);
  void SetOAuthHostForTest(const std::string& oauth_host);
  void SetApiHostForTest(const std::string& api_host);

 private:
  base::SequencedTaskRunner* io_task_runner();

  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  bool LoadTokensFromPrefs();
  bool SetAccessTokens(const std::string& access_token,
                       const std::string& refresh_token);
  void ResetAccessTokens();

  void OnGetAccessToken(AccessTokenCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnTickerPrice(GetTickerPriceCallback callback,
                     const int status, const std::string& body,
                     const std::map<std::string, std::string>& headers);
  void OnGetAccountBalances(GetAccountBalancesCallback callback,
                            const int status, const std::string& body,
                            const std::map<std::string, std::string>& headers);
  void OnGetDepositInfo(GetDepositInfoCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnRevokeAccessToken(RevokeAccessTokenCallback callback,
                          const int status, const std::string& body,
                          const std::map<std::string, std::string>& headers);
  void OnGetOrderQuote(GetOrderQuoteCallback callback,
                       const std::string& side,
                       const int status, const std::string& body,
                       const std::map<std::string, std::string>& headers);
  void OnOrderExecuted(ExecuteOrderCallback callback,
                       const int status, const std::string& body,
                       const std::map<std::string, std::string>& headers);

  bool OAuthRequest(const GURL& url, const std::string& method,
      const std::string& post_data, URLRequestCallback callback,
      bool auto_retry_on_network_change, bool set_auth_header,
      const std::string& payload);
  void OnURLLoaderComplete(
      SimpleURLLoaderList::iterator iter,
      URLRequestCallback callback,
      const std::unique_ptr<std::string> response_body);

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  std::string auth_token_;
  std::string access_token_;
  std::string refresh_token_;
  std::string code_challenge_;
  std::string code_verifier_;
  std::string client_id_;
  std::string client_secret_;
  std::string oauth_host_;
  std::string api_host_;

  raw_ptr<content::BrowserContext> context_ = nullptr;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
  base::WeakPtrFactory<GeminiService> weak_factory_;

  FRIEND_TEST_ALL_PREFIXES(GeminiAPIBrowserTest, GetOAuthClientURL);
  FRIEND_TEST_ALL_PREFIXES(GeminiAPIBrowserTest,
      SetAndGetAuthTokenRevokesPref);
  friend class GeminiAPIBrowserTest;
};

#endif  // BRAVE_COMPONENTS_GEMINI_BROWSER_GEMINI_SERVICE_H_
