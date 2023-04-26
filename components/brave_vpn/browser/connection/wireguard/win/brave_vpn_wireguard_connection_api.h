/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_manager.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefService;

namespace brave_vpn {

class BraveVPNWireguardConnectionAPI
    : public BraveVPNOSConnectionAPI,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  BraveVPNWireguardConnectionAPI(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      version_info::Channel channel);

  BraveVPNWireguardConnectionAPI(const BraveVPNWireguardConnectionAPI&) =
      delete;
  BraveVPNWireguardConnectionAPI& operator=(
      const BraveVPNWireguardConnectionAPI&) = delete;
  ~BraveVPNWireguardConnectionAPI() override;

  void FetchProfileCredentials();

  // BraveVPNOSConnectionAPI
  mojom::ConnectionState GetConnectionState() const override;
  void ResetConnectionState() override;
  void RemoveVPNConnection() override;
  void Connect() override;
  void Disconnect() override;
  void ToggleConnection() override;
  void CheckConnection() override;
  void ResetConnectionInfo() override;
  std::string GetHostname() const override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  void SetConnectionState(mojom::ConnectionState state) override;
  // Returns user friendly error string if existed.
  // Otherwise returns empty.
  std::string GetLastConnectionError() const override;
  BraveVPNRegionDataManager& GetRegionDataManager() override;
  void SetSelectedRegion(const std::string& name) override;
  void LoadSubscriberCredentials() override;

  // net::NetworkChangeNotifier::NetworkChangeObserver
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;
  // BraveVPNRegionDataManager callbacks
  // Notify it's ready when |regions_| is not empty.
  void NotifyRegionDataReady(bool ready) const;
  void NotifySelectedRegionChanged(const std::string& name) const;
  void UpdateAndNotifyConnectionStateChange(mojom::ConnectionState state);
  BraveVpnAPIRequest* GetAPIRequest();
  std::string GetCurrentEnvironment() const;

 private:
  raw_ptr<PrefService> local_prefs_ = nullptr;
  // Only not null when there is active network request.
  // When network request is done, we reset this so we can know
  // whether we're waiting the response or not.
  // We can cancel connecting request quickly when fetching hostnames or
  // profile credentials is not yet finished by reset this.
  std::unique_ptr<BraveVpnAPIRequest> api_request_;
  mojom::ConnectionState connection_state_ =
      mojom::ConnectionState::DISCONNECTED;
  base::ObserverList<Observer> observers_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  BraveVPNRegionDataManager region_data_manager_;
  base::WeakPtrFactory<BraveVPNWireguardConnectionAPI> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_CONNECTION_API_H_
