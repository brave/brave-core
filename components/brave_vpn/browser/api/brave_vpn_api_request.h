/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_BRAVE_VPN_API_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_BRAVE_VPN_API_REQUEST_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"

class GURL;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_vpn {

class BraveVpnAPIRequest {
 public:
  explicit BraveVpnAPIRequest(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  BraveVpnAPIRequest(const BraveVpnAPIRequest&) = delete;
  BraveVpnAPIRequest& operator=(const BraveVpnAPIRequest&) = delete;
  ~BraveVpnAPIRequest();

  using ResponseCallback =
      base::OnceCallback<void(const std::string&, bool success)>;
  using APIRequestResult = api_request_helper::APIRequestResult;

  void GetAllServerRegions(ResponseCallback callback);
  void GetServerRegionsWithCities(ResponseCallback callback);
  void GetTimezonesForRegions(ResponseCallback callback);
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region);
  void GetProfileCredentials(ResponseCallback callback,
                             const std::string& subscriber_credential,
                             const std::string& hostname);
  void GetWireguardProfileCredentials(ResponseCallback callback,
                                      const std::string& subscriber_credential,
                                      const std::string& public_key,
                                      const std::string& hostname);
  void VerifyCredentials(ResponseCallback callback,
                         const std::string& hostname,
                         const std::string& client_id,
                         const std::string& subscriber_credential,
                         const std::string& api_auth_token);
  void InvalidateCredentials(ResponseCallback callback,
                             const std::string& hostname,
                             const std::string& client_id,
                             const std::string& subscriber_credential,
                             const std::string& api_auth_token);
  void GetSubscriberCredential(ResponseCallback callback,
                               const std::string& product_type,
                               const std::string& product_id,
                               const std::string& validation_method,
                               const std::string& purchase_token,
                               const std::string& bundle_id);
  void VerifyPurchaseToken(ResponseCallback callback,
                           const std::string& purchase_token,
                           const std::string& product_id,
                           const std::string& product_type,
                           const std::string& bundle_id);
  void GetSubscriberCredentialV12(ResponseCallback callback,
                                  const std::string& skus_credential,
                                  const std::string& environment);
  void CreateSupportTicket(ResponseCallback callback,
                           const std::string& email,
                           const std::string& subject,
                           const std::string& body,
                           const std::string& subscriber_credential);

 private:
  using URLRequestCallback = base::OnceCallback<void(APIRequestResult)>;

  void OAuthRequest(
      const GURL& url,
      const std::string& method,
      const std::string& post_data,
      URLRequestCallback callback,
      const base::flat_map<std::string, std::string>& headers = {});
  void OnGetResponse(ResponseCallback callback,
                     APIRequestResult api_request_result);
  void OnGetSubscriberCredential(ResponseCallback callback,
                                 APIRequestResult api_request_result);
  void OnCreateSupportTicket(ResponseCallback callback,
                             APIRequestResult api_request_result);

  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<BraveVpnAPIRequest> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_BRAVE_VPN_API_REQUEST_H_
