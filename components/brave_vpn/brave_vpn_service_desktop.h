/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_

#include <string>
#include <vector>

#include "base/scoped_observation.h"
#include "brave/components/brave_vpn/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace base {
class Value;
}  // namespace base

class PrefService;

typedef brave_vpn::mojom::ConnectionState ConnectionState;

class BraveVpnServiceDesktop
    : public BraveVpnService,
      public brave_vpn::BraveVPNOSConnectionAPI::Observer,
      public brave_vpn::mojom::ServiceHandler {
 public:
  BraveVpnServiceDesktop(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);
  ~BraveVpnServiceDesktop() override;

  BraveVpnServiceDesktop(const BraveVpnServiceDesktop&) = delete;
  BraveVpnServiceDesktop& operator=(const BraveVpnServiceDesktop&) = delete;

  void RemoveVPNConnnection();

  bool is_connected() const { return state_ == ConnectionState::CONNECTED; }
  bool is_purchased_user() const { return is_purchased_user_; }
  ConnectionState connection_state() const { return state_; }

  void CheckPurchasedStatus();
  void ToggleConnection();

  void BindInterface(
      mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver);

  // mojom::vpn::ServiceHandler
  void AddObserver(
      mojo::PendingRemote<brave_vpn::mojom::ServiceObserver> observer) override;
  void GetConnectionState(GetConnectionStateCallback callback) override;
  void Connect() override;
  void Disconnect() override;
  void CreateVPNConnection() override;
  void GetAllRegions(GetAllRegionsCallback callback) override;
  void GetDeviceRegion(GetDeviceRegionCallback callback) override;
  void GetSelectedRegion(GetSelectedRegionCallback callback) override;
  void SetSelectedRegion(brave_vpn::mojom::RegionPtr region) override;

 private:
  friend class BraveAppMenuBrowserTest;
  friend class BraveBrowserCommandControllerTest;
  FRIEND_TEST_ALL_PREFIXES(BraveVPNTest, RegionDataTest);

  // BraveVpnService overrides:
  void Shutdown() override;

  // brave_vpn::BraveVPNOSConnectionAPI::Observer overrides:
  void OnCreated(const std::string& name) override;
  void OnRemoved(const std::string& name) override;
  void OnConnected(const std::string& name) override;
  void OnIsConnecting(const std::string& name) override;
  void OnConnectFailed(const std::string& name) override;
  void OnDisconnected(const std::string& name) override;
  void OnIsDisconnecting(const std::string& name) override;

  brave_vpn::BraveVPNConnectionInfo GetConnectionInfo();
  void FetchRegionData();
  void OnFetchRegionList(const std::string& region_list, bool success);
  void ParseAndCacheRegionList(base::Value region_value);
  void OnFetchTimezones(const std::string& timezones_list, bool success);
  void ParseAndCacheDefaultRegionName(base::Value timezons_value);
  void SetDeviceRegion(const std::string& name);
  void SetFallbackDeviceRegion();
  std::string GetCurrentTimeZone();

  void set_test_timezone(const std::string& timezone) {
    test_timezone_ = timezone;
  }
  void set_is_purchased_user_for_test(bool purchased) {
    is_purchased_user_ = purchased;
  }

  PrefService* prefs_ = nullptr;
  std::vector<brave_vpn::mojom::Region> regions_;
  brave_vpn::mojom::Region device_region_;
  ConnectionState state_ = ConnectionState::DISCONNECTED;
  bool is_purchased_user_ = false;
  base::ScopedObservation<brave_vpn::BraveVPNOSConnectionAPI,
                          brave_vpn::BraveVPNOSConnectionAPI::Observer>
      observed_{this};
  mojo::ReceiverSet<brave_vpn::mojom::ServiceHandler> receivers_;
  mojo::RemoteSet<brave_vpn::mojom::ServiceObserver> observers_;
  std::string test_timezone_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_
