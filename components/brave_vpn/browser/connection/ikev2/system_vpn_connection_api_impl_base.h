/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_SYSTEM_VPN_CONNECTION_API_IMPL_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_SYSTEM_VPN_CONNECTION_API_IMPL_BASE_H_

#include <string>

#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/connection_api_impl.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

namespace brave_vpn {

// Base class for IKEv2 connection API implementation.
// Subclass should implement os platform dependent APIs.
class SystemVPNConnectionAPIImplBase : public ConnectionAPIImpl {
 public:
  using ConnectionAPIImpl::ConnectionAPIImpl;
  ~SystemVPNConnectionAPIImplBase() override;

  // ConnectionAPIImpl overrides:
  void Connect() override;
  void Disconnect() override;
  void CheckConnection() override;
  void SetSelectedRegion(const std::string& name) override;
  void FetchProfileCredentials() override;
  void UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState state) override;
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;
  Type type() const override;

 protected:
  friend class SystemVPNConnectionAPIUnitTest;
  FRIEND_TEST_ALL_PREFIXES(SystemVPNConnectionAPIUnitTest, NeedsConnectTest);
  FRIEND_TEST_ALL_PREFIXES(SystemVPNConnectionAPIUnitTest, ConnectionInfoTest);
  FRIEND_TEST_ALL_PREFIXES(SystemVPNConnectionAPIUnitTest,
                           CancelConnectingTest);
  FRIEND_TEST_ALL_PREFIXES(SystemVPNConnectionAPIUnitTest,
                           ClearLastConnectionErrorWhenNewConnectionStart);
  FRIEND_TEST_ALL_PREFIXES(SystemVPNConnectionAPIUnitTest,
                           CreateOSVPNEntryWithValidInfoWhenConnectTest);
  FRIEND_TEST_ALL_PREFIXES(SystemVPNConnectionAPIUnitTest,
                           CreateOSVPNEntryWithInvalidInfoTest);

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

 private:
  bool IsInProgress() const;
  void ResetConnectionInfo();
  void CreateVPNConnection();
  void OnGetProfileCredentials(const std::string& profile_credential,
                               bool success);

  void SetPreventCreationForTesting(bool value);

  bool cancel_connecting_ = false;
  bool needs_connect_ = false;
  bool prevent_creation_ = false;
  BraveVPNConnectionInfo connection_info_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_SYSTEM_VPN_CONNECTION_API_IMPL_BASE_H_
