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
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
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

const char oauth_path_access_token[] = "/auth/token";
const char api_path_account_balances = "/v1/balances";
const char api_path_account_addresses = "v1/addresses/";
const char api_path_get_quote = "/v1/instant/quote";
const char api_path_execute_quote = "/v1/instant/execute";

class GeminiService : public KeyedService {
 public:
  explicit GeminiService(content::BrowserContext* context);
  ~GeminiService() override;

  // Callbacks
  using GetAccessTokenCallback = base::OnceCallback<void(bool)>;

  std::string GetOAuthClientUrl();
  void SetAuthToken();
  bool GetAccessToken(GetAccessTokenCallback callback);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  bool LoadTokensFromPrefs();
  bool SetAccessTokens(const std::string& access_token,
                       const std::string& refresh_token);
  void ResetAccessTokens();

  void OnGetAccessToken(GetAccessTokenCallback callback,
                           const int status, const std::string& body,
                           const std::map<std::string, std::string>& headers);

  bool OAuthRequest(const GURL& url, const std::string& method,
      const std::string& post_data, URLRequestCallback callback,
      bool auto_retry_on_network_change);
  void OnURLLoaderComplete(
      SimpleURLLoaderList::iterator iter,
      URLRequestCallback callback,
      const std::unique_ptr<std::string> response_body);

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  std::string auth_token_;
  std::string access_token_;
  std::string refresh_token_;
  std::string client_id_;
  std::string client_secret_;

  content::BrowserContext* context_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
  base::WeakPtrFactory<BinanceService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GeminiService);
};

#endif  // BRAVE_COMPONENTS_GEMINI_BROWSER_GEMINI_SERVICE_H_
