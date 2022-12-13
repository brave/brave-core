/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_

#include <memory>
#include <string>

#include "base/observer_list_types.h"
#include "brave/components/brave_vpn/mojom/brave_vpn.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}

class PrefService;

namespace brave_vpn {

// Interface for managing OS' vpn connection.
class BraveVPNOSConnectionAPI {
 public:
  BraveVPNOSConnectionAPI() = default;
  virtual ~BraveVPNOSConnectionAPI() = default;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnConnectionStateChanged(mojom::ConnectionState state) = 0;

   protected:
    ~Observer() override = default;
  };

  static BraveVPNOSConnectionAPI* GetInstance();
  static std::unique_ptr<BraveVPNOSConnectionAPI> GetInstanceForTest();

  virtual void SetSharedUrlLoaderFactory(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) = 0;
  virtual void SetLocalPrefs(PrefService* prefs) = 0;
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
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_BRAVE_VPN_OS_CONNECTION_API_H_
