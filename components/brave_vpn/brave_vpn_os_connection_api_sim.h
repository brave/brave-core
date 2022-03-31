/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_SIM_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_SIM_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"

namespace brave_vpn {

class BraveVPNOSConnectionAPISim : public BraveVPNOSConnectionAPI {
 public:
  BraveVPNOSConnectionAPISim(const BraveVPNOSConnectionAPISim&) = delete;
  BraveVPNOSConnectionAPISim& operator=(const BraveVPNOSConnectionAPISim&) =
      delete;

 protected:
  friend class base::NoDestructor<BraveVPNOSConnectionAPISim>;

  BraveVPNOSConnectionAPISim();
  ~BraveVPNOSConnectionAPISim() override;

  // BraveVPNOSConnectionAPI overrides:
  void CreateVPNConnection(const BraveVPNConnectionInfo& info) override;
  void UpdateVPNConnection(const BraveVPNConnectionInfo& info) override;
  void RemoveVPNConnection(const std::string& name) override;
  void Connect(const std::string& name) override;
  void Disconnect(const std::string& name) override;
  void CheckConnection(const std::string& name) override;

 private:
  void OnCreated(const std::string& name, bool success);
  void OnConnected(const std::string& name, bool success);
  void OnIsConnecting(const std::string& name);
  void OnDisconnected(const std::string& name, bool success);
  void OnIsDisconnecting(const std::string& name);
  void OnRemoved(const std::string& name, bool success);

  bool disconnect_requested_ = false;
  base::WeakPtrFactory<BraveVPNOSConnectionAPISim> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_SIM_H_
