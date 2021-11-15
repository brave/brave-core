// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_FTX_BROWSER_FTX_SERVICE_H_
#define BRAVE_COMPONENTS_FTX_BROWSER_FTX_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/files/file_path.h"
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

const char get_futures_data_path[] = "/api/futures";
const char get_market_data_path[] = "/api/markets";
const char oauth_path[] = "/oauth";
const char oauth_token_path[] = "/api/oauth/token";
const char oauth_balances_path[] = "/api/wallet/balances";
const char oauth_quote_path[] = "/api/otc/quotes";
const char futures_filter[] = "perpetual";

struct TokenPriceData {
  std::string symbol;
  double price;
  double percentChangeDay;
  double volumeDay;
};

typedef std::vector<std::map<std::string, double>> FTXChartData;
typedef std::vector<TokenPriceData> FTXFuturesData;
typedef std::map<std::string, double> FTXAccountBalances;

// FTX is a cryptocurrency service that exposes an API with which we can get
// cryptocurrency market data. When authenticated (usually via oauth), we can
// get information about a user's account with FTX, such as balance. We
// can also perform functions on behalf of the user, such as cryptocurrency
// conversions. There are two domains: with which the API can
// be accessed: ftx.us for USA and ftx.com for any other supported country.
// Documentation for the API can be found at https://docs.ftx.com/#rest-api
class FTXService : public KeyedService {
 public:
  explicit FTXService(content::BrowserContext* context);
  ~FTXService() override;

  FTXService(const FTXService&) = delete;
  FTXService& operator=(FTXService&) = delete;

  // KeyedService:
  void Shutdown() override;

  using GetFuturesDataCallback =
      base::OnceCallback<void(const FTXFuturesData&)>;
  using GetChartDataCallback = base::OnceCallback<void(const FTXChartData&)>;
  using GetAccountBalancesCallback =
      base::OnceCallback<void(const FTXAccountBalances&, bool)>;
  using GetConvertQuoteCallback = base::OnceCallback<void(const std::string&)>;
  using GetConvertQuoteInfoCallback =
      base::OnceCallback<void(const std::string&,
                              const std::string&,
                              const std::string&)>;
  using ExecuteConvertQuoteCallback = base::OnceCallback<void(bool)>;

  bool GetFuturesData(GetFuturesDataCallback callback);
  bool GetChartData(const std::string& symbol,
                    const std::string& start,
                    const std::string& end,
                    GetChartDataCallback callback);
  bool GetAccountBalances(GetAccountBalancesCallback callback);
  bool GetConvertQuote(const std::string& from,
                       const std::string& to,
                       const std::string& amount,
                       GetConvertQuoteCallback callback);
  bool GetConvertQuoteInfo(const std::string& quote_id,
                           GetConvertQuoteInfoCallback callback);
  bool ExecuteConvertQuote(const std::string& quote_id,
                           ExecuteConvertQuoteCallback callback);
  std::string GetOAuthClientUrl();
  bool AuthenticateFromAuthToken(const std::string& auth_token);
  void ClearAuth();

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  using URLRequestCallback =
      base::OnceCallback<void(const int,
                              const std::string&,
                              const std::map<std::string, std::string>&)>;

  void OnFuturesData(GetFuturesDataCallback callback,
                     const int status,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers);
  void OnChartData(GetChartDataCallback callback,
                   const int status,
                   const std::string& body,
                   const std::map<std::string, std::string>& headers);
  void OnGetAccountBalances(GetAccountBalancesCallback callback,
                            const int status,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers);
  void OnGetConvertQuote(GetConvertQuoteCallback callback,
                         const int status, const std::string& body,
                         const std::map<std::string, std::string>& headers);
  void OnGetConvertQuoteInfo(GetConvertQuoteInfoCallback callback,
                             const int status, const std::string& body,
                             const std::map<std::string, std::string>& headers);
  void OnExecuteConvertQuote(ExecuteConvertQuoteCallback callback,
                             const int status, const std::string& body,
                             const std::map<std::string, std::string>& headers);
  GURL GetOAuthURL(const std::string& path);
  std::string GetTokenHeader();
  bool SetAccessToken(const std::string& access_token);

  base::SequencedTaskRunner* io_task_runner();

  bool NetworkRequest(const GURL& url,
                      const std::string& method,
                      const std::string& post_data,
                      const std::string& post_data_type,
                      URLRequestCallback callback,
                      bool set_auth_header);
  void OnURLLoaderComplete(SimpleURLLoaderList::iterator iter,
                           URLRequestCallback callback,
                           const std::unique_ptr<std::string> response_body);

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  std::string access_token_;
  std::string client_id_;
  std::string client_secret_;

  content::BrowserContext* context_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
  base::WeakPtrFactory<FTXService> weak_factory_;
};

#endif  // BRAVE_COMPONENTS_FTX_BROWSER_FTX_SERVICE_H_
