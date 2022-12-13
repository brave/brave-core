/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_

#include <string>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "brave/components/brave_vpn/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/mojom/brave_vpn.mojom.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include <memory>
#include <vector>

#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefService;

#if !BUILDFLAG(IS_ANDROID)
class BraveAppMenuBrowserTest;
class BraveBrowserCommandControllerTest;
#endif  // !BUILDFLAG(IS_ANDROID)

namespace brave_vpn {

constexpr char kNewUserReturningHistogramName[] = "Brave.VPN.NewUserReturning";
constexpr char kDaysInMonthUsedHistogramName[] = "Brave.VPN.DaysInMonthUsed";
constexpr char kLastUsageTimeHistogramName[] = "Brave.VPN.LastUsageTime";

// This class is used by desktop and android.
// However, it includes desktop specific impls and it's hidden
// by IS_ANDROID ifdef.
class BraveVpnService :
#if !BUILDFLAG(IS_ANDROID)
    public BraveVPNOSConnectionAPI::Observer,
#endif
    public mojom::ServiceHandler,
    public KeyedService {
 public:
  BraveVpnService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      PrefService* profile_prefs,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter);
  ~BraveVpnService() override;

  BraveVpnService(const BraveVpnService&) = delete;
  BraveVpnService& operator=(const BraveVpnService&) = delete;

  std::string GetCurrentEnvironment() const;
  bool is_purchased_user() const {
    return purchased_state_ == mojom::PurchasedState::PURCHASED;
  }
  void BindInterface(mojo::PendingReceiver<mojom::ServiceHandler> receiver);
  void ReloadPurchasedState();

#if !BUILDFLAG(IS_ANDROID)
  void ToggleConnection();
  void RemoveVPNConnection();
  mojom::ConnectionState GetConnectionState() const;
  bool IsConnected() const;

  // mojom::vpn::ServiceHandler
  void GetConnectionState(GetConnectionStateCallback callback) override;
  void Connect() override;
  void Disconnect() override;
  void GetAllRegions(GetAllRegionsCallback callback) override;
  void GetSelectedRegion(GetSelectedRegionCallback callback) override;
  void SetSelectedRegion(mojom::RegionPtr region) override;
  void GetProductUrls(GetProductUrlsCallback callback) override;
  void CreateSupportTicket(const std::string& email,
                           const std::string& subject,
                           const std::string& body,
                           CreateSupportTicketCallback callback) override;
  void GetSupportData(GetSupportDataCallback callback) override;
#else
  // mojom::vpn::ServiceHandler
  void GetPurchaseToken(GetPurchaseTokenCallback callback) override;
#endif  // !BUILDFLAG(IS_ANDROID)

  using ResponseCallback =
      base::OnceCallback<void(const std::string&, bool success)>;

  // mojom::vpn::ServiceHandler
  void AddObserver(
      mojo::PendingRemote<mojom::ServiceObserver> observer) override;
  void GetPurchasedState(GetPurchasedStateCallback callback) override;
  void LoadPurchasedState(const std::string& domain) override;

  void GetAllServerRegions(ResponseCallback callback);
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
  void GetSubscriberCredentialV12(ResponseCallback callback);

  // new_usage should be set to true if a new VPN connection was just
  // established.
  void RecordP3A(bool new_usage);
#if BUILDFLAG(IS_ANDROID)
  void RecordAndroidBackgroundP3A(int64_t session_start_time_ms,
                                  int64_t session_end_time_ms);
#endif

 private:
  friend class BraveVPNServiceTest;

#if !BUILDFLAG(IS_ANDROID)
  friend class ::BraveAppMenuBrowserTest;
  friend class ::BraveBrowserCommandControllerTest;

  // BraveVPNOSConnectionAPI::Observer overrides:
  void OnConnectionStateChanged(mojom::ConnectionState state) override;

  void LoadCachedRegionData();
  void FetchRegionData(bool background_fetch);
  void OnFetchRegionList(bool background_fetch,
                         const std::string& region_list,
                         bool success);
  bool ParseAndCacheRegionList(const base::Value::List& region_value,
                               bool save_to_prefs = false);
  void OnFetchTimezones(const std::string& timezones_list, bool success);
  void SetDeviceRegionWithTimezone(const base::Value::List& timezons_value);
  void SetDeviceRegion(const std::string& name);
  void SetSelectedRegion(const std::string& name);
  std::string GetDeviceRegion() const;
  std::string GetSelectedRegion() const;
  void SetFallbackDeviceRegion();
  void SetRegionListToPrefs();

  std::string GetCurrentTimeZone();
  void ScheduleBackgroundRegionDataFetch();
  void ScheduleFetchRegionDataIfNeeded();

  void OnCreateSupportTicket(CreateSupportTicketCallback callback,
                             const std::string& ticket,
                             bool success);

  void OnPreferenceChanged(const std::string& pref_name);

  BraveVPNOSConnectionAPI* GetBraveVPNConnectionAPI() const;

  void set_mock_brave_vpn_connection_api(BraveVPNOSConnectionAPI* api) {
    mock_connection_api_ = api;
  }
#endif  // !BUILDFLAG(IS_ANDROID)

  // KeyedService overrides:
  void Shutdown() override;

  void InitP3A();
  void OnP3AInterval();

  mojom::PurchasedState GetPurchasedStateSync() const;
  void SetPurchasedState(const std::string& env, mojom::PurchasedState state);
  void SetCurrentEnvironment(const std::string& env);
  void EnsureMojoConnected();
  void OnMojoConnectionError();
  void OnCredentialSummary(const std::string& domain,
                           const std::string& summary_string);
  void OnPrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& credential_as_cookie);
  void OnGetSubscriberCredentialV12(const base::Time& expiration_time,
                                    const std::string& subscriber_credential,
                                    bool success);
  void ScheduleSubscriberCredentialRefresh();
  void RefreshSubscriberCredential();

  // Check initial purchased/connected state.
  void CheckInitialState();

#if !BUILDFLAG(IS_ANDROID)
  std::vector<mojom::Region> regions_;
  base::ScopedObservation<BraveVPNOSConnectionAPI,
                          BraveVPNOSConnectionAPI::Observer>
      observed_{this};
  base::RepeatingTimer region_data_update_timer_;

  // Only for testing.
  std::string test_timezone_;
  bool is_simulation_ = false;
  raw_ptr<BraveVPNOSConnectionAPI> mock_connection_api_ = nullptr;

  PrefChangeRegistrar pref_change_registrar_;
#endif  // !BUILDFLAG(IS_ANDROID)

  SEQUENCE_CHECKER(sequence_checker_);

  raw_ptr<PrefService> local_prefs_ = nullptr;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  mojo::ReceiverSet<mojom::ServiceHandler> receivers_;
  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  absl::optional<mojom::PurchasedState> purchased_state_;
  mojo::RemoteSet<mojom::ServiceObserver> observers_;
  BraveVpnAPIRequest api_request_;
  base::RepeatingTimer p3a_timer_;
  base::OneShotTimer subs_cred_refresh_timer_;
  base::WeakPtrFactory<BraveVpnService> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_
