/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_

#include <string>

#if !defined(OS_ANDROID)
#include <memory>
#include <vector>

#include "base/scoped_observation.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "brave/components/brave_vpn/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#endif

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "components/keyed_service/core/keyed_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

#if !defined(OS_ANDROID)
namespace base {
class Value;
}  // namespace base

class PrefService;

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;
#endif  // !defined(OS_ANDROID)

class BraveVpnService :
#if !defined(OS_ANDROID)
      public brave_vpn::BraveVPNOSConnectionAPI::Observer,
      public brave_vpn::mojom::ServiceHandler,
#endif
      public KeyedService {
 public:
  BraveVpnService(
#if defined(OS_ANDROID)
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
#else
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter);
#endif
  ~BraveVpnService() override;

  BraveVpnService(const BraveVpnService&) = delete;
  BraveVpnService& operator=(const BraveVpnService&) = delete;

#if !defined(OS_ANDROID)
  void ToggleConnection();
  void RemoveVPNConnnection();

  bool is_connected() const {
    return connection_state_ == ConnectionState::CONNECTED;
  }
  bool is_purchased_user() const {
    return purchased_state_ == PurchasedState::PURCHASED;
  }
  ConnectionState connection_state() const { return connection_state_; }

  void BindInterface(
      mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver);

  // mojom::vpn::ServiceHandler
  void AddObserver(
      mojo::PendingRemote<brave_vpn::mojom::ServiceObserver> observer) override;
  void GetConnectionState(GetConnectionStateCallback callback) override;
  void GetPurchasedState(GetPurchasedStateCallback callback) override;
  void Connect() override;
  void Disconnect() override;
  void CreateVPNConnection() override;
  void GetAllRegions(GetAllRegionsCallback callback) override;
  void GetDeviceRegion(GetDeviceRegionCallback callback) override;
  void GetSelectedRegion(GetSelectedRegionCallback callback) override;
  void SetSelectedRegion(brave_vpn::mojom::RegionPtr region) override;
  void GetProductUrls(GetProductUrlsCallback callback) override;
#endif  // !defined(OS_ANDROID)

  using ResponseCallback =
      base::OnceCallback<void(const std::string&, bool success)>;

  void GetAllServerRegions(ResponseCallback callback);
  void GetTimezonesForRegions(ResponseCallback callback);
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region);
  void GetProfileCredentials(ResponseCallback callback,
                             const std::string& subscriber_credential,
                             const std::string& hostname);
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
                                  const std::string& payments_environment,
                                  const std::string& monthly_pass);

 private:
#if !defined(OS_ANDROID)
  friend class BraveAppMenuBrowserTest;
  friend class BraveBrowserCommandControllerTest;
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, RegionDataTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, HostnamesTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, CancelConnectingTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, ConnectionInfoTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, LoadPurchasedStateTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, LoadRegionDataFromPrefsTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, NeedsConnectTest);

  // brave_vpn::BraveVPNOSConnectionAPI::Observer overrides:
  void OnCreated() override;
  void OnCreateFailed() override;
  void OnRemoved() override;
  void OnConnected() override;
  void OnIsConnecting() override;
  void OnConnectFailed() override;
  void OnDisconnected() override;
  void OnIsDisconnecting() override;

  brave_vpn::BraveVPNConnectionInfo GetConnectionInfo();
  void LoadCachedRegionData();
  void LoadPurchasedState();
  void LoadSelectedRegion();
  void UpdateAndNotifyConnectionStateChange(ConnectionState state);

  void FetchRegionData();
  void OnFetchRegionList(const std::string& region_list, bool success);
  bool ParseAndCacheRegionList(base::Value region_value);
  void OnFetchTimezones(const std::string& timezones_list, bool success);
  void ParseAndCacheDeviceRegionName(base::Value timezons_value);
  void FetchHostnamesForRegion(const std::string& name);
  void OnFetchHostnames(const std::string& region,
                        const std::string& hostnames,
                        bool success);
  void ParseAndCacheHostnames(const std::string& region,
                              base::Value hostnames_value);
  void SetDeviceRegion(const std::string& name);
  void SetFallbackDeviceRegion();
  void SetDeviceRegion(const brave_vpn::mojom::Region& region);

  std::string GetCurrentTimeZone();
  void SetPurchasedState(PurchasedState state);
  void ScheduleFetchRegionDataIfNeeded();
  std::unique_ptr<brave_vpn::Hostname> PickBestHostname(
      const std::vector<brave_vpn::Hostname>& hostnames);

  void EnsureMojoConnected();
  void OnMojoConnectionError();
  void OnGetSubscriberCredentialV12(const std::string& subscriber_credential,
                                    bool success);
  void OnGetProfileCredentials(const std::string& profile_credential,
                               bool success);
  void OnCredentialSummary(const std::string& summary_string);
  void OnPrepareCredentialsPresentation(
      const std::string& credential_as_cookie);

  brave_vpn::BraveVPNOSConnectionAPI* GetBraveVPNConnectionAPI();

  void set_test_timezone(const std::string& timezone) {
    test_timezone_ = timezone;
  }
#endif  // !defined(OS_ANDROID)

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

#if !defined(OS_ANDROID)
  PrefService* prefs_ = nullptr;
  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  std::string skus_credential_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  std::vector<brave_vpn::mojom::Region> regions_;
  brave_vpn::mojom::Region device_region_;
  brave_vpn::mojom::Region selected_region_;
  std::unique_ptr<brave_vpn::Hostname> hostname_;
  brave_vpn::BraveVPNConnectionInfo connection_info_;
  bool cancel_connecting_ = false;
  PurchasedState purchased_state_ = PurchasedState::NOT_PURCHASED;
  ConnectionState connection_state_ = ConnectionState::DISCONNECTED;
  bool needs_connect_ = false;
  base::ScopedObservation<brave_vpn::BraveVPNOSConnectionAPI,
                          brave_vpn::BraveVPNOSConnectionAPI::Observer>
      observed_{this};
  mojo::ReceiverSet<brave_vpn::mojom::ServiceHandler> receivers_;
  mojo::RemoteSet<brave_vpn::mojom::ServiceObserver> observers_;
  base::RepeatingTimer region_data_update_timer_;

  // Only for testing.
  std::string test_timezone_;
  bool is_simulation_ = false;

  SEQUENCE_CHECKER(sequence_checker_);
#endif

  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<BraveVpnService> weak_ptr_factory_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_
