/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_

#include <memory>
#include <string>

#include "base/observer_list_types.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefService;

namespace brave_vpn {

// Interface for managing OS' vpn connection.
class BraveVPNOSConnectionAPI {
 public:
  virtual ~BraveVPNOSConnectionAPI() = default;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnConnectionStateChanged(mojom::ConnectionState state) = 0;

   protected:
    ~Observer() override = default;
  };

  virtual void SetTargetVpnEntryName(const std::string& name) = 0;
  virtual mojom::ConnectionState GetConnectionState() const = 0;
  virtual void RemoveVPNConnection() = 0;
  virtual void Connect() = 0;
  virtual void Disconnect() = 0;
  virtual void ToggleConnection() = 0;
  virtual void CheckConnection() = 0;
  virtual void ResetConnectionInfo() = 0;
  virtual std::string GetHostname() const = 0;
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
  virtual void SetConnectionState(mojom::ConnectionState state) = 0;
  // Returns user friendly error string if existed.
  // Otherwise returns empty.
  virtual std::string GetLastConnectionError() const = 0;

 protected:
  BraveVPNOSConnectionAPI() = default;
};

// Create platform specific api instance.
// NOTE: Don't call this method directly.
// Only BraveBrowserProcess need to use this method.
std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNOSConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_
