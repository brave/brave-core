/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_VPN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_VPN_SERVICE_H_

#include <list>
#include <map>
#include <string>

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

typedef std::string JsonResponse;

class VpnService : public KeyedService {
 public:
  explicit VpnService(content::BrowserContext* context);
  ~VpnService() override;

  using ResponseCallback =
      base::OnceCallback<void(const JsonResponse&, bool success)>;

  void GetAllServerRegions(ResponseCallback callback);
  void GetTimezonesForRegions(ResponseCallback callback);
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region);
  void GetSubscriberCredential(ResponseCallback callback,
                               const std::string& product_type,
                               const std::string& product_id,
                               const std::string& validation_method,
                               const std::string& purchase_token);
  void VerifyPurchaseToken(ResponseCallback callback,
                           const std::string& purchase_token,
                           const std::string& product_id,
                           const std::string& product_type);

 private:
  static GURL oauth_endpoint_;
  static GURL api_endpoint_;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  using URLRequestCallback =
      base::OnceCallback<void(const int,
                              const std::string&,
                              const std::map<std::string, std::string>&)>;

  bool OAuthRequest(const GURL& url,
                    const std::string& method,
                    const std::string& post_data,
                    bool set_app_ident,
                    URLRequestCallback callback);
  void OnURLLoaderComplete(SimpleURLLoaderList::iterator iter,
                           URLRequestCallback callback,
                           const std::unique_ptr<std::string> response_body);

  void OnGetAllServerRegions(ResponseCallback callback,
                             const int status,
                             const std::string& body,
                             const std::map<std::string, std::string>& headers);

  void OnGetTimezonesForRegions(
      ResponseCallback callback,
      const int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  void OnGetHostnamesForRegion(
      ResponseCallback callback,
      const int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  void OnGetSubscriberCredential(
      ResponseCallback callback,
      const int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  void OnVerifyPurchaseToken(ResponseCallback callback,
                             const int status,
                             const std::string& body,
                             const std::map<std::string, std::string>& headers);

  content::BrowserContext* context_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
  base::WeakPtrFactory<VpnService> weak_factory_;
  base::WeakPtrFactory<VpnService> vpn_weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(VpnService);
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_VPN_SERVICE_H_
