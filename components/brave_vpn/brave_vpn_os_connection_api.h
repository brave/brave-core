/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_H_

#include <string>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"

namespace brave_vpn {

// Interface for managing OS' vpn connection.
class BraveVPNOSConnectionAPI {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnCreated() = 0;
    virtual void OnCreateFailed() = 0;
    virtual void OnRemoved() = 0;
    virtual void OnConnected() = 0;
    virtual void OnIsConnecting() = 0;
    virtual void OnConnectFailed() = 0;
    virtual void OnDisconnected() = 0;
    virtual void OnIsDisconnecting() = 0;

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
