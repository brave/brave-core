/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_TEST_FAKE_BRAVE_VPN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_TEST_FAKE_BRAVE_VPN_SERVICE_H_

#include <string>

#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "build/build_config.h"

namespace brave_vpn {

// Concrete test subclass providing stub implementations for BraveVpnService's
// pure virtual methods. Allows exercising the base-class behavior (observer
// management, notification dispatch, shutdown, mojo binding) in isolation.
class FakeBraveVpnService : public BraveVpnService {
 public:
  FakeBraveVpnService() = default;
  ~FakeBraveVpnService() override = default;

  // BraveVpnService:
  bool IsBraveVPNEnabled() const override;
  bool IsPurchased() const override;
  void ReloadPurchasedState() override {}
  std::string GetCurrentEnvironment() const override;

  // mojom::ServiceHandler
  void GetPurchasedState(GetPurchasedStateCallback callback) override {}
  void LoadPurchasedState(const std::string& domain) override {}
  void GetAllRegions(GetAllRegionsCallback callback) override {}

  // Re-expose the protected notification helpers and Shutdown for tests.
  using BraveVpnService::NotifyPurchasedStateChanged;
  using BraveVpnService::Shutdown;

#if !BUILDFLAG(IS_ANDROID)
  bool IsConnected() const override;
  void ToggleConnection() override {}
  mojom::ConnectionState GetConnectionState() const override;
  std::string GetLastConnectionError() const override;
  void RecordWidgetUsageMetrics(bool new_usage) override {}

  // mojom::ServiceHandler
  void GetConnectionState(GetConnectionStateCallback callback) override {}
  void Connect() override {}
  void Disconnect() override {}
  void GetSelectedRegion(GetSelectedRegionCallback callback) override {}
  void SetSelectedRegion(mojom::RegionPtr region) override {}
  void ClearSelectedRegion() override {}
  void GetProductUrls(GetProductUrlsCallback callback) override {}
  void CreateSupportTicket(const std::string& email,
                           const std::string& subject,
                           const std::string& body,
                           CreateSupportTicketCallback callback) override {}
  void GetSupportData(GetSupportDataCallback callback) override {}
  void ResetConnectionState() override {}
  void EnableOnDemand(bool enable) override {}
  void GetOnDemandState(GetOnDemandStateCallback callback) override {}
  void EnableSmartProxyRouting(bool enable) override {}
  void GetSmartProxyRoutingState(
      GetSmartProxyRoutingStateCallback callback) override {}

  void SetConnectionStateForTesting(mojom::ConnectionState state) override {}
  void SetPurchasedStateForTesting(const std::string& env,
                                   mojom::PurchasedState state) override {}

  // Re-expose the protected notification helpers and Shutdown for tests.
  using BraveVpnService::NotifyConnectionStateChanged;
  using BraveVpnService::NotifySelectedRegionChanged;
  using BraveVpnService::NotifySmartProxyRoutingStateChanged;
#else   // !BUILDFLAG(IS_ANDROID)
  void GetTimezonesForRegions(ResponseCallback callback) override {}
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region,
                             const std::string& region_precision) override {}
  void GetProfileCredentials(ResponseCallback callback,
                             const std::string& subscriber_credential,
                             const std::string& hostname) override {}
  void GetWireguardProfileCredentials(ResponseCallback callback,
                                      const std::string& subscriber_credential,
                                      const std::string& public_key,
                                      const std::string& hostname) override {}
  void VerifyCredentials(ResponseCallback callback,
                         const std::string& hostname,
                         const std::string& client_id,
                         const std::string& subscriber_credential,
                         const std::string& api_auth_token) override {}
  void InvalidateCredentials(ResponseCallback callback,
                             const std::string& hostname,
                             const std::string& client_id,
                             const std::string& subscriber_credential,
                             const std::string& api_auth_token) override {}
  void VerifyPurchaseToken(ResponseCallback callback,
                           const std::string& purchase_token,
                           const std::string& product_id,
                           const std::string& product_type,
                           const std::string& bundle_id) override {}
  void GetSubscriberCredential(ResponseCallback callback,
                               const std::string& product_type,
                               const std::string& product_id,
                               const std::string& validation_method,
                               const std::string& purchase_token,
                               const std::string& bundle_id) override {}
  void GetSubscriberCredentialV12(ResponseCallback callback) override {}
  void RecordAllMetrics() override {}
  void RecordAndroidBackgroundP3A(int64_t session_start_time_ms,
                                  int64_t session_end_time_ms) override {}

  // mojom::ServiceHandler
  void GetPurchaseToken(GetPurchaseTokenCallback callback) override {}
#endif  // !BUILDFLAG(IS_ANDROID)
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_TEST_FAKE_BRAVE_VPN_SERVICE_H_
