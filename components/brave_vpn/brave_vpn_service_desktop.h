/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_

#include <memory>
#include <string>
#include <vector>

#include "base/scoped_observation.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "brave/components/brave_vpn/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace base {
class Value;
}  // namespace base

class SkusSdkService;

class PrefService;

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;

class BraveVpnServiceDesktop
    : public BraveVpnService,
      public brave_vpn::BraveVPNOSConnectionAPI::Observer,
      public brave_vpn::mojom::ServiceHandler {
 public:
  BraveVpnServiceDesktop(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs,
      SkusSdkService* skus_sdk_service);
  ~BraveVpnServiceDesktop() override;

  BraveVpnServiceDesktop(const BraveVpnServiceDesktop&) = delete;
  BraveVpnServiceDesktop& operator=(const BraveVpnServiceDesktop&) = delete;

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

 private:
  friend class BraveAppMenuBrowserTest;
  friend class BraveBrowserCommandControllerTest;
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, RegionDataTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, HostnamesTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, CancelConnectingTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, ConnectionInfoTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, LoadPurchasedStateTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, LoadRegionDataFromPrefsTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, NeedsConnectTest);

  // BraveVpnService overrides:
  void Shutdown() override;

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

  void OnSkusVPNCredentialUpdated();
  void OnGetSubscriberCredential(const std::string& subscriber_credential,
                                 bool success);
  void OnGetProfileCredentials(const std::string& profile_credential,
                               bool success);
  void OnPrepareCredentialsPresentation(
      const std::string& credential_as_cookie);

  brave_vpn::BraveVPNOSConnectionAPI* GetBraveVPNConnectionAPI();

  void set_test_timezone(const std::string& timezone) {
    test_timezone_ = timezone;
  }

  PrefService* prefs_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  std::string skus_credential_;
  SkusSdkService* skus_sdk_service_ = nullptr;
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
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_
