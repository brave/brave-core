/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/one_shot_event.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_manager.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "net/base/network_change_notifier.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefService;

namespace brave_vpn {

class BraveVPNRegionDataManager;
struct Hostname;

// Interface for managing OS' vpn connection.
class BraveVPNOSConnectionAPI
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  ~BraveVPNOSConnectionAPI() override;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnConnectionStateChanged(mojom::ConnectionState state) = 0;
    // false when fetching region data is failed.
    virtual void OnRegionDataReady(bool success) {}
    virtual void OnSelectedRegionChanged(const std::string& region_name) {}

   protected:
    ~Observer() override = default;
  };

  // Shared APIs implementation between IKEv2/Wireguard connections.
  mojom::ConnectionState GetConnectionState() const;
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  BraveVPNRegionDataManager& GetRegionDataManager();
  void ResetHostname();
  std::string GetHostname() const;
  void ResetConnectionState();
  // Returns user friendly error string if existed.
  // Otherwise returns empty.
  std::string GetLastConnectionError() const;
  void ToggleConnection();

  std::string target_vpn_entry_name() const { return target_vpn_entry_name_; }
  void set_target_vpn_entry_name(const std::string& name) {
    target_vpn_entry_name_ = name;
  }

  // Connection dependent APIs.
  virtual void Connect() = 0;
  virtual void Disconnect() = 0;
  virtual void CheckConnection() = 0;
  virtual void UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState state);
  virtual void SetSelectedRegion(const std::string& name) = 0;
  virtual void FetchProfileCredentials() = 0;
  virtual void MaybeInstallSystemServices();

 protected:
  explicit BraveVPNOSConnectionAPI(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs);

  std::string GetCurrentEnvironment() const;
  PrefService* local_prefs() { return local_prefs_; }
  // True when do quick cancel.
  bool QuickCancelIfPossible();
  void ResetAPIRequestInstance();
  BraveVpnAPIRequest* GetAPIRequest();
  void SetLastConnectionError(const std::string& error);
  void FetchHostnamesForRegion(const std::string& name);
  void OnFetchHostnames(const std::string& region,
                        const std::string& hostnames,
                        bool success);
  void ParseAndCacheHostnames(const std::string& region,
                              const base::Value::List& hostnames_value);
  // BraveVPNRegionDataManager callbacks
  // Notify it's ready when |regions_| is not empty.
  void NotifyRegionDataReady(bool ready) const;
  void NotifySelectedRegionChanged(const std::string& name) const;
  // net::NetworkChangeNotifier::NetworkChangeObserver
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;
  void OnInstallSystemServicesCompleted(bool success);

  // For now, this is called when Connect() is called.
  // If system service installation is in-progress, connect request
  // is queued and return true.
  // Then, start connect after installation is done.
  bool ScheduleConnectRequestIfNeeded();

  // Installs system services (if neeeded) or is nullptr.
  // Bound in brave_vpn::CreateBraveVPNConnectionAPI.
  base::RepeatingCallback<bool()> install_system_service_callback_;

 private:
  friend class BraveVpnButtonUnitTest;

  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest, NeedsConnectTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest, ConnectionInfoTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           CancelConnectingTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, ResetConnectionStateTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest,
                           ConnectionStateUpdateWithPurchasedStateTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest,
                           IsConnectedWithPurchasedStateTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, DisconnectedIfDisabledByPolicy);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           IgnoreDisconnectedStateWhileConnecting);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest, HostnamesTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNWireguardConnectionAPIUnitTest,
                           SetSelectedRegion);

  void SetConnectionStateForTesting(mojom::ConnectionState state);

  raw_ptr<PrefService> local_prefs_;

  std::unique_ptr<Hostname> hostname_;
  std::string last_connection_error_;
  // Only not null when there is active network request.
  // When network request is done, we reset this so we can know
  // whether we're waiting the response or not.
  // We can cancel connecting request quickly when fetching hostnames or
  // profile credentials is not yet finished by reset this.
  std::unique_ptr<BraveVpnAPIRequest> api_request_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojom::ConnectionState connection_state_ =
      mojom::ConnectionState::DISCONNECTED;
  BraveVPNRegionDataManager region_data_manager_;
  base::ObserverList<Observer> observers_;
  // Used for tracking if the VPN dependencies are being installed.
  // Guard against calling install_system_service_callback_ while a call
  // is already in progress.
  bool install_in_progress_ = false;
  // Used for tracking if the VPN dependencies have been installed.
  // If the user has Brave VPN purchased and loaded with this profile
  // AND they did a system level install, we should call
  // install_system_service_callback_ once per browser open.
  base::OneShotEvent system_service_installed_event_;
  std::string target_vpn_entry_name_;
  base::WeakPtrFactory<BraveVPNOSConnectionAPI> weak_factory_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_
