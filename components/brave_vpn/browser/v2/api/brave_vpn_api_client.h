/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_BRAVE_VPN_API_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_BRAVE_VPN_API_CLIENT_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/browser/v2/api/purchase_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/raw_json_response_body.h"
#include "brave/components/brave_vpn/browser/v2/api/transport_protocol.h"

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

  // The response shape common to every Guardian Connect-API endpoint whose
  // success is forwarded as opaque JSON and whose error is a parsed
  // VpnErrorBody title.
  using RawJsonResponse =
      brave_account::endpoint_client::Response<endpoints::RawJsonResponseBody,
                                               endpoints::VpnErrorBody>;
  using RawJsonCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;

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

  // Connect API: CreateSupportTicket.
  // Submits a customer support inquiry. On success: the server's confirmation
  // JSON (an echo of the submitted data). On failure: a user-facing error
  // string.
  virtual void CreateSupportTicket(RawJsonCallback callback,
                                   const std::string& email,
                                   const std::string& subject,
                                   const std::string& body,
                                   const std::string& subscriber_credential,
                                   const std::string& timezone);

  // Connect API: GetServerRegions.
  // Returns all regions at the given precision, as the server's JSON response
  // verbatim. Typed parsing is left to the caller (e.g. a region data manager).
  virtual void GetServerRegions(RawJsonCallback callback,
                                const std::string& region_precision);

  // Connect API: GetTimezonesForRegions.
  virtual void GetTimezonesForRegions(RawJsonCallback callback);

  // Connect API: GetHostnamesForRegion.
  virtual void GetHostnamesForRegion(RawJsonCallback callback,
                                     const std::string& region,
                                     const std::string& region_precision);

  // SGW API: GetProfileCredentials.
  // Registers VPN credentials with the given node for the given transport
  // protocol. Returns the server's JSON response verbatim.
  virtual void GetProfileCredentials(
      RawJsonCallback callback,
      const std::string& hostname,
      const std::string& subscriber_credential,
      endpoints::TransportProtocol transport_protocol,
      const std::optional<std::string>& public_key,
      const std::optional<std::string>& multihop_exit_region);

  // SGW API: VerifyCredentials.
  // Confirms the credential has not been invalidated. Per Guardian, any non-2xx
  // response should be treated as a hard failure: the credential must not be
  // reused.
  virtual void VerifyCredentials(RawJsonCallback callback,
                                 const std::string& hostname,
                                 const std::string& client_id,
                                 const std::string& api_auth_token);

  // SGW API: InvalidateCredentials.
  // Allows for explicit disabling of VPN credentials to ensure that they cannot
  // be used ever again. VPN credentials that are no longer going to be used
  // should always be invalidated.
  virtual void InvalidateCredentials(RawJsonCallback callback,
                                     const std::string& hostname,
                                     const std::string& client_id,
                                     const std::string& api_auth_token,
                                     const std::string& subscriber_credential);

  // SGW API: GetAvailableMultihopExitRegions.
  // Returns the current multihop exit region and the destinations available
  // to choose from, as the server's JSON response verbatim.
  virtual void GetAvailableMultihopExitRegions(
      RawJsonCallback callback,
      const std::string& hostname,
      const std::string& client_id,
      const std::string& api_auth_token);

  // SGW API: SetMultihopExitRegion.
  // Configures the multihop egress destination |multihop_exit_region|, which
  // must be non-empty and must not be Guardian's wire sentinel for "disabled".
  // Violating either crashes rather than returning an error. Use
  // ClearMultihopExitRegion() instead to disable multihop.
  virtual void SetMultihopExitRegion(RawJsonCallback callback,
                                     const std::string& hostname,
                                     const std::string& client_id,
                                     const std::string& api_auth_token,
                                     const std::string& multihop_exit_region);

  // SGW API: ClearMultihopExitRegion.
  // Disables multihop by clearing the egress destination.
  virtual void ClearMultihopExitRegion(RawJsonCallback callback,
                                       const std::string& hostname,
                                       const std::string& client_id,
                                       const std::string& api_auth_token);

 private:
  void OnRawJsonResponse(RawJsonCallback callback, RawJsonResponse response);
  void OnGetSubscriberCredentialResponse(
      SubscriberCredentialCallback callback,
      endpoints::GetSubscriberCredential::Response response);
  void OnVerifyPurchaseTokenResponse(
      VerifyPurchaseTokenCallback callback,
      endpoints::VerifyPurchaseToken::Response response);

  void DoSetMultihopExitRegion(RawJsonCallback callback,
                               const std::string& hostname,
                               const std::string& client_id,
                               const std::string& api_auth_token,
                               const std::string& multihop_exit_region);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<BraveVpnApiClient> weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_BRAVE_VPN_API_CLIENT_H_
