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
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_vpn/mojom/brave_vpn.mojom.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include <memory>
#include <vector>

#include "base/power_monitor/power_observer.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/brave_vpn_data_types.h"
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
    public base::PowerSuspendObserver,
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
  void RemoveVPNConnnection();
  bool is_connected() const {
    return connection_state_ == mojom::ConnectionState::CONNECTED;
  }
  mojom::ConnectionState connection_state() const { return connection_state_; }

  // mojom::vpn::ServiceHandler
  void GetConnectionState(GetConnectionStateCallback callback) override;
  void ResetConnectionState() override;
  void Connect() override;
  void Disconnect() override;
  void CreateVPNConnection() override;
  void GetAllRegions(GetAllRegionsCallback callback) override;
  void GetDeviceRegion(GetDeviceRegionCallback callback) override;
  void GetSelectedRegion(GetSelectedRegionCallback callback) override;
  void SetSelectedRegion(mojom::RegionPtr region) override;
  void GetProductUrls(GetProductUrlsCallback callback) override;
  void CreateSupportTicket(const std::string& email,
                           const std::string& subject,
                           const std::string& body,
                           CreateSupportTicketCallback callback) override;
  void GetSupportData(GetSupportDataCallback callback) override;

  // base::PowerMonitor
  void OnSuspend() override;
  void OnResume() override;
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
  void GetPurchaseToken(GetPurchaseTokenCallback callback) override;
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

  void InitP3A();
  void OnP3AInterval();

#if !BUILDFLAG(IS_ANDROID)
  friend class ::BraveAppMenuBrowserTest;
  friend class ::BraveBrowserCommandControllerTest;

  // BraveVPNOSConnectionAPI::Observer overrides:
  void OnCreated() override;
  void OnCreateFailed() override;
  void OnRemoved() override;
  void OnConnected() override;
  void OnIsConnecting() override;
  void OnConnectFailed() override;
  void OnDisconnected() override;
  void OnIsDisconnecting() override;

  const BraveVPNConnectionInfo& GetConnectionInfo();
  void LoadCachedRegionData();
  void UpdateAndNotifyConnectionStateChange(mojom::ConnectionState state,
                                            bool force = false);

  void FetchRegionData(bool background_fetch);
  void OnFetchRegionList(bool background_fetch,
                         const std::string& region_list,
                         bool success);
  bool ParseAndCacheRegionList(const base::Value::List& region_value,
                               bool save_to_prefs = false);
  void OnFetchTimezones(const std::string& timezones_list, bool success);
  void SetDeviceRegionWithTimezone(const base::Value::List& timezons_value);
  void FetchHostnamesForRegion(const std::string& name);
  void OnFetchHostnames(const std::string& region,
                        const std::string& hostnames,
                        bool success);
  void ParseAndCacheHostnames(const std::string& region,
                              const base::Value::List& hostnames_value);
  void SetDeviceRegion(const std::string& name);
  void SetSelectedRegion(const std::string& name);
  std::string GetDeviceRegion() const;
  std::string GetSelectedRegion() const;
  void SetFallbackDeviceRegion();
  void SetRegionListToPrefs();

  std::string GetCurrentTimeZone();
  void ScheduleBackgroundRegionDataFetch();
  void ScheduleFetchRegionDataIfNeeded();

  void OnGetSubscriberCredentialV12(const std::string& subscriber_credential,
                                    bool success);
  void OnGetProfileCredentials(const std::string& profile_credential,
                               bool success);
  void OnCreateSupportTicket(
      CreateSupportTicketCallback callback,
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  BraveVPNOSConnectionAPI* GetBraveVPNConnectionAPI();
#endif  // !BUILDFLAG(IS_ANDROID)

  using URLRequestCallback =
      base::OnceCallback<void(int,
                              const std::string&,
                              const base::flat_map<std::string, std::string>&)>;

  // KeyedService overrides:
  void Shutdown() override;

  void OAuthRequest(
      const GURL& url,
      const std::string& method,
      const std::string& post_data,
      URLRequestCallback callback,
      const base::flat_map<std::string, std::string>& headers = {});

  void OnGetResponse(ResponseCallback callback,
                     int status,
                     const std::string& body,
                     const base::flat_map<std::string, std::string>& headers);

  void OnGetSubscriberCredential(
      ResponseCallback callback,
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
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

  raw_ptr<PrefService> local_prefs_ = nullptr;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
#if !BUILDFLAG(IS_ANDROID)
  std::vector<mojom::Region> regions_;
  std::unique_ptr<Hostname> hostname_;
  BraveVPNConnectionInfo connection_info_;
  bool cancel_connecting_ = false;
  mojom::ConnectionState connection_state_ =
      mojom::ConnectionState::DISCONNECTED;
  bool needs_connect_ = false;
  base::ScopedObservation<BraveVPNOSConnectionAPI,
                          BraveVPNOSConnectionAPI::Observer>
      observed_{this};
  base::RepeatingTimer region_data_update_timer_;

  // Only for testing.
  std::string test_timezone_;
  bool is_simulation_ = false;
#endif  // !BUILDFLAG(IS_ANDROID)

  mojo::ReceiverSet<mojom::ServiceHandler> receivers_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  absl::optional<mojom::PurchasedState> purchased_state_;
  mojo::RemoteSet<mojom::ServiceObserver> observers_;
  api_request_helper::APIRequestHelper api_request_helper_;
  std::string skus_credential_;
  base::RepeatingTimer p3a_timer_;
  base::WeakPtrFactory<BraveVpnService> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_
