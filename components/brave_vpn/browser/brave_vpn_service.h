/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/scoped_observation.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_delegate.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefService;

#if !BUILDFLAG(IS_ANDROID)
class BraveAppMenuBrowserTest;
class BraveAppMenuModelBrowserTest;
class BraveBrowserCommandControllerTest;
#endif  // !BUILDFLAG(IS_ANDROID)

namespace brave_vpn {

class BraveVPNServiceDelegate;

inline constexpr char kNewUserReturningHistogramName[] =
    "Brave.VPN.NewUserReturning";
inline constexpr char kDaysInMonthUsedHistogramName[] =
    "Brave.VPN.DaysInMonthUsed";
inline constexpr char kLastUsageTimeHistogramName[] = "Brave.VPN.LastUsageTime";

// This class is used by desktop and android.
// However, it includes desktop specific impls and it's hidden
// by IS_ANDROID ifdef.
class BraveVpnService :
#if !BUILDFLAG(IS_ANDROID)
    public BraveVPNConnectionManager::Observer,
#endif
    public mojom::ServiceHandler,
    public KeyedService {
 public:
  BraveVpnService(
      BraveVPNConnectionManager* connection_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      PrefService* profile_prefs,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter);
  ~BraveVpnService() override;

  BraveVpnService(const BraveVpnService&) = delete;
  BraveVpnService& operator=(const BraveVpnService&) = delete;

#if BUILDFLAG(IS_ANDROID)
  mojo::PendingRemote<brave_vpn::mojom::ServiceHandler> MakeRemote();
#endif  // BUILDFLAG(IS_ANDROID)

  std::string GetCurrentEnvironment() const;
  bool is_purchased_user() const {
    return GetPurchasedInfoSync().state == mojom::PurchasedState::PURCHASED;
  }
  void BindInterface(mojo::PendingReceiver<mojom::ServiceHandler> receiver);
  void ReloadPurchasedState();
  bool IsBraveVPNEnabled() const;
#if !BUILDFLAG(IS_ANDROID)
  void ToggleConnection();
  mojom::ConnectionState GetConnectionState() const;
  bool IsConnected() const;

  // mojom::vpn::ServiceHandler
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
#else
  // mojom::vpn::ServiceHandler
  void GetPurchaseToken(GetPurchaseTokenCallback callback) override;
  void OnFetchRegionList(GetAllRegionsCallback callback,
                         const std::string& region_list,
                         bool success);
#endif  // !BUILDFLAG(IS_ANDROID)

  using ResponseCallback =
      base::OnceCallback<void(const std::string&, bool success)>;

  // mojom::vpn::ServiceHandler
  void AddObserver(
      mojo::PendingRemote<mojom::ServiceObserver> observer) override;
  void GetPurchasedState(GetPurchasedStateCallback callback) override;
  void LoadPurchasedState(const std::string& domain) override;

  void GetAllRegions(GetAllRegionsCallback callback) override;

  void GetTimezonesForRegions(ResponseCallback callback);
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region,
                             const std::string& region_precision);
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
  void GetSubscriberCredentialV12(ResponseCallback callback);

  void set_delegate(std::unique_ptr<BraveVPNServiceDelegate> delegate) {
    delegate_ = std::move(delegate);
  }

  // new_usage should be set to true if a new VPN connection was just
  // established.
  void RecordP3A(bool new_usage);
#if BUILDFLAG(IS_ANDROID)
  void RecordAndroidBackgroundP3A(int64_t session_start_time_ms,
                                  int64_t session_end_time_ms);
#endif

 private:
  friend class BraveVPNServiceTest;
  friend class BraveVpnButtonUnitTest;

#if !BUILDFLAG(IS_ANDROID)
  friend class ::BraveAppMenuBrowserTest;
  friend class ::BraveAppMenuModelBrowserTest;
  friend class ::BraveBrowserCommandControllerTest;

  // BraveVPNConnectionManager::Observer overrides:
  void OnConnectionStateChanged(mojom::ConnectionState state) override;
  void OnRegionDataReady(bool success) override;
  void OnSelectedRegionChanged(const std::string& region_name) override;

  void OnCreateSupportTicket(CreateSupportTicketCallback callback,
                             const std::string& ticket,
                             bool success);

  void OnPreferenceChanged(const std::string& pref_name);

  void UpdatePurchasedStateForSessionExpired(const std::string& env);
  bool IsCurrentRegionSelectedAutomatically(
      const brave_vpn::mojom::RegionPtr& region);
#endif  // !BUILDFLAG(IS_ANDROID)

  // KeyedService overrides:
  void Shutdown() override;

  void InitP3A();
  void OnP3AInterval();

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
  raw_ptr<BraveVPNConnectionManager> connection_manager_ = nullptr;

  PrefChangeRegistrar policy_pref_change_registrar_;
#endif  // !BUILDFLAG(IS_ANDROID)

  SEQUENCE_CHECKER(sequence_checker_);

  raw_ptr<PrefService> local_prefs_ = nullptr;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  mojo::ReceiverSet<mojom::ServiceHandler> receivers_;
  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  std::optional<mojom::PurchasedInfo> purchased_state_;
  mojo::RemoteSet<mojom::ServiceObserver> observers_;
  std::unique_ptr<BraveVpnAPIRequest> api_request_;
  std::unique_ptr<BraveVPNServiceDelegate> delegate_;
  base::RepeatingTimer p3a_timer_;
  base::OneShotTimer subs_cred_refresh_timer_;
  base::WeakPtrFactory<BraveVpnService> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_H_
