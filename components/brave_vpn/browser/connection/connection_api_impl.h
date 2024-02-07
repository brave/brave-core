/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_CONNECTION_API_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_CONNECTION_API_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "net/base/network_change_notifier.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_vpn {

class BraveVPNOSConnectionAPI;
class BraveVpnAPIRequest;
struct Hostname;

class ConnectionAPIImpl
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  enum class Type {
    IKEV2,
    WIREGUARD,
  };

  ConnectionAPIImpl(
      BraveVPNOSConnectionAPI* api,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~ConnectionAPIImpl() override;

  ConnectionAPIImpl(const ConnectionAPIImpl&) = delete;
  ConnectionAPIImpl& operator=(const ConnectionAPIImpl&) = delete;

  void ToggleConnection();
  mojom::ConnectionState GetConnectionState() const;
  void ResetConnectionState();
  std::string GetLastConnectionError() const;
  std::string GetHostname() const;

  virtual void Connect() = 0;
  virtual void Disconnect() = 0;
  virtual void CheckConnection() = 0;
  virtual void SetSelectedRegion(const std::string& name) = 0;
  virtual void FetchProfileCredentials() = 0;
  virtual void UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState state);
  virtual Type type() const = 0;

  // net::NetworkChangeNotifier::NetworkChangeObserver
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

 protected:
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
  void ResetHostname();

  const raw_ref<BraveVPNOSConnectionAPI> api_;  // owner

 private:
  void SetConnectionStateForTesting(mojom::ConnectionState state);

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
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_CONNECTION_API_IMPL_H_
