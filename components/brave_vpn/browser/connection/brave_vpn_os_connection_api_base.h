/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_BASE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/values.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

class PrefService;

namespace brave_vpn {

class BraveVpnAPIRequest;
struct Hostname;

class BraveVPNOSConnectionAPIBase
    : public net::NetworkChangeNotifier::NetworkChangeObserver,
      public BraveVPNOSConnectionAPI {
 public:
  BraveVPNOSConnectionAPIBase(const BraveVPNOSConnectionAPIBase&) = delete;
  BraveVPNOSConnectionAPIBase& operator=(const BraveVPNOSConnectionAPIBase&) =
      delete;

  const BraveVPNConnectionInfo& connection_info() const;
  bool IsInProgress() const;

  // BraveVPNOSConnectionAPI
  void SetLocalPrefs(PrefService* prefs) override;
  void SetSharedUrlLoaderFactory(scoped_refptr<network::SharedURLLoaderFactory>
                                     url_loader_factory) override;
  void SetTargetVpnEntryName(const std::string& name) override;
  mojom::ConnectionState GetConnectionState() const override;
  void Connect() override;
  void Disconnect() override;
  void ToggleConnection() override;
  void RemoveVPNConnection() override;
  void CheckConnection() override;
  void ResetConnectionInfo() override;
  std::string GetHostname() const override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  void SetConnectionState(mojom::ConnectionState state) override;
  void SetPreventCreationForTesting(bool value);

 protected:
  BraveVPNOSConnectionAPIBase();
  ~BraveVPNOSConnectionAPIBase() override;

  // Subclass should add platform dependent impls.
  virtual void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) = 0;
  virtual void ConnectImpl(const std::string& name) = 0;
  virtual void DisconnectImpl(const std::string& name) = 0;
  virtual void RemoveVPNConnectionImpl(const std::string& name) = 0;
  virtual void CheckConnectionImpl(const std::string& name) = 0;

  // Subclass should call below callbacks whenever corresponding event happens.
  void OnCreated();
  void OnCreateFailed();
  void OnConnected();
  void OnIsConnecting();
  void OnConnectFailed();
  void OnDisconnected();
  void OnIsDisconnecting();

  std::string target_vpn_entry_name() const { return target_vpn_entry_name_; }

 private:
  friend class BraveVPNOSConnectionAPISim;
  friend class BraveVPNOSConnectionAPIUnitTest;

  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           CreateOSVPNEntryWithValidInfoWhenConnectTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           CreateOSVPNEntryWithInvalidInfoTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           CheckConnectionStateAfterNetworkStateChanged);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest, HostnamesTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           CancelConnectingTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest, ConnectionInfoTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest, NeedsConnectTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           IgnoreDisconnectedStateWhileConnecting);
  // net::NetworkChangeNotifier::NetworkChangeObserver
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  void CreateVPNConnection();
  std::string GetSelectedRegion() const;
  std::string GetDeviceRegion() const;
  std::string GetCurrentEnvironment() const;
  void FetchHostnamesForRegion(const std::string& name);
  void OnFetchHostnames(const std::string& region,
                        const std::string& hostnames,
                        bool success);
  void ParseAndCacheHostnames(const std::string& region,
                              const base::Value::List& hostnames_value);
  void OnGetProfileCredentials(const std::string& profile_credential,
                               bool success);

  virtual void UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState state);
  BraveVpnAPIRequest* GetAPIRequest();
  // True when do quick cancel.
  bool QuickCancelIfPossible();

  bool cancel_connecting_ = false;
  bool needs_connect_ = false;
  bool prevent_creation_ = false;
  std::string target_vpn_entry_name_;
  mojom::ConnectionState connection_state_ =
      mojom::ConnectionState::DISCONNECTED;
  BraveVPNConnectionInfo connection_info_;
  raw_ptr<PrefService> local_prefs_ = nullptr;
  std::unique_ptr<Hostname> hostname_;
  base::ObserverList<Observer> observers_;
  // Only not null when there is active network request.
  // When network request is done, we reset this so we can know
  // whether we're waiting the response or not.
  // We can cancel connecting request quickly when fetching hostnames or
  // profile credentials is not yet finished by reset this.
  std::unique_ptr<BraveVpnAPIRequest> api_request_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_BASE_H_
