/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_BRAVE_VPN_API_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_BRAVE_VPN_API_CLIENT_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_vpn/browser/v2/api/purchase_endpoints.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_vpn::v2 {

// BraveVpnApiClient issues typed requests to Brave VPN's backend endpoints
// via Brave Endpoint Client.
// Callbacks return typed results (base::expected<...>) rather than a raw JSON
// string paired with a success bool, so callers can distinguish transport/HTTP
// failures from a structured server-reported error without re-parsing anything
// themselves.
class BraveVpnApiClient {
 public:
  explicit BraveVpnApiClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  BraveVpnApiClient(const BraveVpnApiClient&) = delete;
  BraveVpnApiClient& operator=(const BraveVpnApiClient&) = delete;
  virtual ~BraveVpnApiClient();

  // Connect API: GetSubscriberCredential (legacy, store purchase token).
  // Exchanges store purchase details for a subscriber credential.
  // On success: the subscriber credential.
  // On failure: a user-facing error string (transport/HTTP failure, or the
  // server's reported error message).
  using SubscriberCredentialCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;
  virtual void GetSubscriberCredential(SubscriberCredentialCallback callback,
                                       const std::string& product_type,
                                       const std::string& product_id,
                                       const std::string& validation_method,
                                       const std::string& purchase_token,
                                       const std::string& bundle_id);

  // Connect API: GetSubscriberCredentialV12.
  // Exchanges a SKUS credential for a subscriber credential.
  // On success: the subscriber credential.
  // On failure: a user-facing error string (transport/HTTP failure, or the
  // server's reported error message).
  virtual void GetSubscriberCredentialV12(SubscriberCredentialCallback callback,
                                          const std::string& skus_credential,
                                          const std::string& environment);

  // Connect API: VerifyPurchaseToken.
  // Verifies a store purchase token and returns the server's JSON response
  // verbatim.
  using VerifyPurchaseTokenCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;
  virtual void VerifyPurchaseToken(VerifyPurchaseTokenCallback callback,
                                   const std::string& purchase_token,
                                   const std::string& product_id,
                                   const std::string& product_type,
                                   const std::string& bundle_id);

 private:
  void OnGetSubscriberCredentialResponse(
      SubscriberCredentialCallback callback,
      endpoints::GetSubscriberCredential::Response response);
  void OnVerifyPurchaseTokenResponse(
      VerifyPurchaseTokenCallback callback,
      endpoints::VerifyPurchaseToken::Response response);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<BraveVpnApiClient> weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_BRAVE_VPN_API_CLIENT_H_
