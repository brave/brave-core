/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"

namespace brave_vpn {

// Interface for managing OS' vpn connection.
class BraveVPNConnectionManager {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // TODO(simonhong): Don't need |name| parameter because only one vpn
    // connection is managed.
    virtual void OnCreated(const std::string& name) = 0;
    virtual void OnRemoved(const std::string& name) = 0;
    virtual void OnConnected(const std::string& name) = 0;
    virtual void OnDisconnected(const std::string& name) = 0;

   protected:
    ~Observer() override = default;
  };

  static BraveVPNConnectionManager* GetInstance();

  BraveVPNConnectionManager(const BraveVPNConnectionManager&) = delete;
  BraveVPNConnectionManager& operator=(const BraveVPNConnectionManager&) =
      delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  virtual void CreateVPNConnection(const BraveVPNConnectionInfo& info) = 0;
  virtual void UpdateVPNConnection(const BraveVPNConnectionInfo& info) = 0;
  virtual void Connect(const std::string& name) = 0;
  virtual void Disconnect(const std::string& name) = 0;
  virtual void RemoveVPNConnection(const std::string& name) = 0;

 protected:
  BraveVPNConnectionManager();
  virtual ~BraveVPNConnectionManager();

  base::ObserverList<Observer> observers_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_CONNECTION_MANAGER_H_
