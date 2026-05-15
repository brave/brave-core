/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_IMPL_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/brave_vpn_metrics.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_delegate.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_member.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace misc_metrics {
class UptimeMonitor;
}  // namespace misc_metrics

class PrefService;

namespace brave_vpn {

class BraveVPNServiceDelegate;

// The original Brave VPN service implementation, which is used in
// Architecture 1.0.
class BraveVpnServiceImpl : public BraveVpnService,
#if !BUILDFLAG(IS_ANDROID)
                            public BraveVPNConnectionManager::Observer,
#endif
                            public BraveVpnMetrics::Delegate {
 public:
  BraveVpnServiceImpl(
      BraveVPNConnectionManager* connection_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      PrefService* profile_prefs,
      base::WeakPtr<misc_metrics::UptimeMonitor> uptime_monitor,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter);
  ~BraveVpnServiceImpl() override;

  BraveVpnServiceImpl(const BraveVpnServiceImpl&) = delete;
  BraveVpnServiceImpl& operator=(const BraveVpnServiceImpl&) = delete;

  // BraveVpnService overrides:
  // Implementation of public interface exposed to other components.
  bool IsBraveVPNEnabled() const override;
  bool IsPurchased() const override;
  void ReloadPurchasedState() override;
  std::string GetCurrentEnvironment() const override;
#if !BUILDFLAG(IS_ANDROID)
  bool IsConnected() const override;
  void ToggleConnection() override;
  mojom::ConnectionState GetConnectionState() const override;
  void RecordWidgetUsageMetrics(bool new_usage) override;
#endif  // !BUILDFLAG(IS_ANDROID)

  // mojom::ServiceHandler overrides:
  void GetPurchasedState(GetPurchasedStateCallback callback) override;
  void LoadPurchasedState(const std::string& domain) override;
  void GetAllRegions(GetAllRegionsCallback callback) override;
#if !BUILDFLAG(IS_ANDROID)
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
  void GetPurchaseToken(GetPurchaseTokenCallback callback) override;
#endif  // !BUILDFLAG(IS_ANDROID)

  // BraveVpnMetrics::Delegate
  bool is_purchased_user() const override;
#if !BUILDFLAG(IS_ANDROID)
  bool is_connected_vpn() const override;
#endif  //! BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)
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

  void set_delegate(std::unique_ptr<BraveVPNServiceDelegate> delegate) {
    delegate_ = std::move(delegate);
  }

  BraveVpnMetrics* brave_vpn_metrics() { return &brave_vpn_metrics_; }

 private:
  friend class BraveVpnServiceImplV1Test;

#if !BUILDFLAG(IS_ANDROID)
  // BraveVPNConnectionManager::Observer overrides:
  void OnConnectionStateChanged(mojom::ConnectionState state) override;
  void OnRegionDataReady(bool success) override;
  void OnSelectedRegionChanged(const std::string& region_name) override;
  void OnInstallSystemServicesCompleted(bool success) override;

  void OnCreateSupportTicket(CreateSupportTicketCallback callback,
                             const std::string& ticket,
                             bool success);

  void OnPreferenceChanged(const std::string& pref_name);

  void UpdatePurchasedStateForSessionExpired(const std::string& env);
  bool IsCurrentRegionSelectedAutomatically(
      const brave_vpn::mojom::RegionPtr& region);
#else   // !BUILDFLAG(IS_ANDROID)
  void OnFetchRegionList(GetAllRegionsCallback callback,
                         const std::string& region_list,
                         bool success);
#endif  // !BUILDFLAG(IS_ANDROID)

  // KeyedService overrides:
  void Shutdown() override;

  // BraveVpnService overrides:
#if !BUILDFLAG(IS_ANDROID)
  void SetConnectionStateForTesting(mojom::ConnectionState state) override;
  void SetPurchasedStateForTesting(const std::string& env,
                                   mojom::PurchasedState state) override;
#endif  // !BUILDFLAG(IS_ANDROID)

  mojom::PurchasedInfo GetPurchasedInfoSync() const;
  void SetPurchasedState(
      const std::string& env,
      mojom::PurchasedState state,
      const std::optional<std::string>& description = std::nullopt);
  void SetCurrentEnvironment(const std::string& env);
  void EnsureMojoConnected();
  void OnMojoConnectionError();
  void RequestCredentialSummary(const std::string& domain);
  void OnCredentialSummary(const std::string& domain,
                           skus::mojom::SkusResultPtr summary);
  void OnPrepareCredentialsPresentation(
      const std::string& domain,
      skus::mojom::SkusResultPtr credential_as_cookie);
  void OnGetSubscriberCredentialV12(const base::Time& expiration_time,
                                    const std::string& subscriber_credential,
                                    bool success);
  void ScheduleSubscriberCredentialRefresh();
  void RefreshSubscriberCredential();

  // Check initial purchased/connected state.
  void CheckInitialState();
#if !BUILDFLAG(IS_ANDROID)
  base::ScopedObservation<BraveVPNConnectionManager,
                          BraveVPNConnectionManager::Observer>
      observed_{this};
  bool wait_region_data_ready_ = false;
  raw_ptr<BraveVPNConnectionManager, DanglingUntriaged> connection_manager_ =
      nullptr;

  BooleanPrefMember smart_proxy_routing_enabled_;
  PrefChangeRegistrar policy_pref_change_registrar_;
#endif  // !BUILDFLAG(IS_ANDROID)

  raw_ptr<PrefService> local_prefs_ = nullptr;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  std::optional<mojom::PurchasedInfo> purchased_state_;
  std::unique_ptr<BraveVpnAPIRequest> api_request_;
  std::unique_ptr<BraveVPNServiceDelegate> delegate_;
  base::OneShotTimer subs_cred_refresh_timer_;
  BraveVpnMetrics brave_vpn_metrics_;
  base::WeakPtrFactory<BraveVpnServiceImpl> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_IMPL_H_
