
/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_BRAVE_VPN_RAS_CONNECTION_API_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_BRAVE_VPN_RAS_CONNECTION_API_BASE_H_

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/values.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_manager.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefService;

namespace brave_vpn {

class BraveVpnAPIRequest;

class BraveVPNOSConnectionAPIBase : public BraveVPNOSConnectionAPI {
 public:
  BraveVPNOSConnectionAPIBase(const BraveVPNOSConnectionAPIBase&) = delete;
  BraveVPNOSConnectionAPIBase& operator=(const BraveVPNOSConnectionAPIBase&) =
      delete;

  const BraveVPNConnectionInfo& connection_info() const;
  bool IsInProgress() const;

  // BraveVPNOSConnectionAPI
  void Connect() override;
  void Disconnect() override;
  void CheckConnection() override;
  void UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState state) override;
  void SetSelectedRegion(const std::string& name) override;
  void FetchProfileCredentials() override;
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

 protected:
  BraveVPNOSConnectionAPIBase(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      version_info::Channel channel);
  ~BraveVPNOSConnectionAPIBase() override;

  // Subclass should add platform dependent impls.
  virtual void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) = 0;
  virtual void ConnectImpl(const std::string& name) = 0;
  virtual void DisconnectImpl(const std::string& name) = 0;
  virtual void CheckConnectionImpl(const std::string& name) = 0;
  virtual bool IsPlatformNetworkAvailable() = 0;

  // Subclass should call below callbacks whenever corresponding event happens.
  void OnCreated();
  void OnCreateFailed();
  void OnConnected();
  void OnIsConnecting();
  void OnConnectFailed();
  void OnDisconnected();
  void OnIsDisconnecting();
  bool MaybeReconnect();

  std::string target_vpn_entry_name() const { return target_vpn_entry_name_; }

 private:
  friend class BraveVPNRegionDataManager;
  friend class BraveVPNOSConnectionAPISim;
  friend class BraveVPNOSConnectionAPIUnitTest;
  friend class BraveVPNServiceTest;

  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest, NeedsConnectTest);
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
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           IgnoreDisconnectedStateWhileConnecting);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           ClearLastConnectionErrorWhenNewConnectionStart);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest, ConnectionInfoTest);

  void ResetConnectionInfo();
  void CreateVPNConnection();
  void OnGetProfileCredentials(const std::string& profile_credential,
                               bool success);

  void SetPreventCreationForTesting(bool value);

  bool cancel_connecting_ = false;
  bool needs_connect_ = false;
  bool prevent_creation_ = false;
  std::string target_vpn_entry_name_;
  BraveVPNConnectionInfo connection_info_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_BRAVE_VPN_RAS_CONNECTION_API_BASE_H_
