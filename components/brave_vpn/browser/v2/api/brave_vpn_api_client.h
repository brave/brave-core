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

  // Connect API: GetSubscriberCredentialV12.
  // Exchanges a SKUS credential for a subscriber credential.
  // On success: the subscriber credential.
  // On failure: a user-facing error string (transport/HTTP failure, or the
  // server's reported error message).
  using SubscriberCredentialCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;
  virtual void GetSubscriberCredentialV12(SubscriberCredentialCallback callback,
                                          const std::string& skus_credential,
                                          const std::string& environment);

 private:
  void OnGetSubscriberCredentialV12Response(
      SubscriberCredentialCallback callback,
      endpoints::GetSubscriberCredentialV12::Response response);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<BraveVpnApiClient> weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_BRAVE_VPN_API_CLIENT_H_
