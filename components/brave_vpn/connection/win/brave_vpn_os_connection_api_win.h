/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_WIN_BRAVE_VPN_OS_CONNECTION_API_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_WIN_BRAVE_VPN_OS_CONNECTION_API_WIN_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/win/object_watcher.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/connection/brave_vpn_os_connection_api_base.h"

namespace brave_vpn {
namespace internal {
enum class CheckConnectionResult;
}  // namespace internal

class BraveVPNOSConnectionAPIWin : public BraveVPNOSConnectionAPIBase,
                                   public base::win::ObjectWatcher::Delegate {
 public:
  BraveVPNOSConnectionAPIWin(const BraveVPNOSConnectionAPIWin&) = delete;
  BraveVPNOSConnectionAPIWin& operator=(const BraveVPNOSConnectionAPIWin&) =
      delete;

 protected:
  friend class base::NoDestructor<BraveVPNOSConnectionAPIWin>;

  BraveVPNOSConnectionAPIWin();
  ~BraveVPNOSConnectionAPIWin() override;

 private:
  // BraveVPNOSConnectionAPIBase interfaces:
  void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) override;
  void RemoveVPNConnectionImpl(const std::string& name) override;
  void ConnectImpl(const std::string& name) override;
  void DisconnectImpl(const std::string& name) override;
  void CheckConnectionImpl(const std::string& name) override;

  // base::win::ObjectWatcher::Delegate overrides:
  void OnObjectSignaled(HANDLE object) override;

  void OnCreated(const std::string& name, bool success);
  void OnConnected(bool success);
  void OnDisconnected(bool success);
  void OnRemoved(const std::string& name, bool success);
  void OnCheckConnection(const std::string& name,
                         internal::CheckConnectionResult result);

  void StartVPNConnectionChangeMonitoring();

  HANDLE event_handle_for_connected_disconnected_ = NULL;
  base::win::ObjectWatcher connected_disconnected_event_watcher_;
  base::WeakPtrFactory<BraveVPNOSConnectionAPIWin> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_CONNECTION_WIN_BRAVE_VPN_OS_CONNECTION_API_WIN_H_
