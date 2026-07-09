/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_BRAVE_VPN_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_BRAVE_VPN_SERVICE_IMPL_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"
#include "build/build_config.h"

class PrefService;

namespace brave_vpn::v2 {

class PurchasedStateManager;

class BraveVpnServiceImpl : public BraveVpnService {
 public:
  BraveVpnServiceImpl(PrefService* local_prefs,
                      PrefService* profile_prefs,
                      GetSkusServiceCallback skus_service_getter);
  ~BraveVpnServiceImpl() override;

  BraveVpnServiceImpl(const BraveVpnServiceImpl&) = delete;
  BraveVpnServiceImpl& operator=(const BraveVpnServiceImpl&) = delete;

  // BraveVpnService overrides:
  // Implementation of public interface exposed to other components.
  bool IsBraveVPNEnabled() const override;
  bool IsPurchased() const override;
  void ReloadPurchasedState() override;
  std::string GetCurrentEnvironment() const override;

  // mojom::ServiceHandler overrides:
  void GetPurchasedState(GetPurchasedStateCallback callback) override;
  void LoadPurchasedState(const std::string& domain) override;
  void GetAllRegions(GetAllRegionsCallback callback) override;

#if !BUILDFLAG(IS_ANDROID)
  // BraveVpnService overrides:
  bool IsConnected() const override;
  void ToggleConnection() override;
  mojom::ConnectionState GetConnectionState() const override;
  std::string GetLastConnectionError() const override;
  void RecordWidgetUsageMetrics(bool new_usage) override;

  // mojom::ServiceHandler overrides:
  void GetConnectionState(GetConnectionStateCallback callback) override;
  void Connect() override;
  void Disconnect() override;
  void GetSelectedRegion(GetSelectedRegionCallback callback) override;
  void SetSelectedRegion(mojom::RegionPtr region) override;
  void ClearSelectedRegion() override;
  void GetProductUrls(GetProductUrlsCallback callback) override;
  void CreateSupportTicket(const std::string& email,
                           const std::string& subject,
                           const std::string& body,
                           CreateSupportTicketCallback callback) override;
  void GetSupportData(GetSupportDataCallback callback) override;
  void ResetConnectionState() override;
  void EnableOnDemand(bool enable) override;
  void GetOnDemandState(GetOnDemandStateCallback callback) override;
  void EnableSmartProxyRouting(bool enable) override;
  void GetSmartProxyRoutingState(
      GetSmartProxyRoutingStateCallback callback) override;
#else   // !BUILDFLAG(IS_ANDROID)
  // mojom::ServiceHandler overrides:
  void GetPurchaseToken(GetPurchaseTokenCallback callback) override;

  // Implementation of public interface for Android native worker.
  void GetTimezonesForRegions(ResponseCallback callback) override;
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region,
                             const std::string& region_precision) override;
  void GetProfileCredentials(ResponseCallback callback,
                             const std::string& subscriber_credential,
                             const std::string& hostname) override;
  void GetWireguardProfileCredentials(ResponseCallback callback,
                                      const std::string& subscriber_credential,
                                      const std::string& public_key,
                                      const std::string& hostname) override;
  void VerifyCredentials(ResponseCallback callback,
                         const std::string& hostname,
                         const std::string& client_id,
                         const std::string& subscriber_credential,
                         const std::string& api_auth_token) override;
  void InvalidateCredentials(ResponseCallback callback,
                             const std::string& hostname,
                             const std::string& client_id,
                             const std::string& subscriber_credential,
                             const std::string& api_auth_token) override;
  void VerifyPurchaseToken(ResponseCallback callback,
                           const std::string& purchase_token,
                           const std::string& product_id,
                           const std::string& product_type,
                           const std::string& bundle_id) override;
  void GetSubscriberCredential(ResponseCallback callback,
                               const std::string& product_type,
                               const std::string& product_id,
                               const std::string& validation_method,
                               const std::string& purchase_token,
                               const std::string& bundle_id) override;
  void GetSubscriberCredentialV12(ResponseCallback callback) override;
  void RecordAllMetrics() override;
  void RecordAndroidBackgroundP3A(int64_t session_start_time_ms,
                                  int64_t session_end_time_ms) override;
#endif  // BUILDFLAG(IS_ANDROID)

 private:
  friend class BraveVpnServiceImplTest;

  // KeyedService overrides:
  void Shutdown() override;

  // BraveVpnService overrides:
#if !BUILDFLAG(IS_ANDROID)
  void SetConnectionStateForTesting(mojom::ConnectionState state) override;
  void SetPurchasedStateForTesting(const std::string& env,
                                   mojom::PurchasedState state) override;
#endif  // !BUILDFLAG(IS_ANDROID)

  void OnPurchasedStateChanged(mojom::PurchasedState state,
                               std::optional<std::string> description);

  const raw_ref<PrefService> profile_prefs_;
  std::unique_ptr<SkusServiceClient> skus_client_;
  std::unique_ptr<PurchasedStateManager> purchased_state_manager_;
  [[maybe_unused]] mojom::ConnectionState connection_state_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_BRAVE_VPN_SERVICE_IMPL_H_
