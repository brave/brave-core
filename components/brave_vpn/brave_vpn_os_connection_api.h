/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_H_

#include <string>

#include "base/observer_list.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"

namespace brave_vpn {

// Interface for managing OS' vpn connection.
class BraveVPNOSConnectionAPI {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // TODO(simonhong): Don't need |name| parameter because only one vpn
    // connection is managed.
    virtual void OnCreated(const std::string& name) = 0;
    virtual void OnRemoved(const std::string& name) = 0;
    virtual void OnConnected(const std::string& name) = 0;
    virtual void OnIsConnecting(const std::string& name) = 0;
    virtual void OnConnectFailed(const std::string& name) = 0;
    virtual void OnDisconnected(const std::string& name) = 0;
    virtual void OnIsDisconnecting(const std::string& name) = 0;

   protected:
    ~Observer() override = default;
  };

  static BraveVPNOSConnectionAPI* GetInstance();
  static BraveVPNOSConnectionAPI* GetInstanceForTest();

  BraveVPNOSConnectionAPI(const BraveVPNOSConnectionAPI&) = delete;
  BraveVPNOSConnectionAPI& operator=(const BraveVPNOSConnectionAPI&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void set_target_vpn_entry_name(const std::string& name) {
    target_vpn_entry_name_ = name;
  }
  std::string target_vpn_entry_name() const { return target_vpn_entry_name_; }

  virtual void CreateVPNConnection(const BraveVPNConnectionInfo& info) = 0;
  virtual void UpdateVPNConnection(const BraveVPNConnectionInfo& info) = 0;
  virtual void Connect(const std::string& name) = 0;
  virtual void Disconnect(const std::string& name) = 0;
  virtual void RemoveVPNConnection(const std::string& name) = 0;
  virtual void CheckConnection(const std::string& name) = 0;

 protected:
  BraveVPNOSConnectionAPI();
  virtual ~BraveVPNOSConnectionAPI();

  std::string target_vpn_entry_name_;
  base::ObserverList<Observer> observers_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_H_
