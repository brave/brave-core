/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_BRAVE_VPN_RAS_CONNECTION_API_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_BRAVE_VPN_RAS_CONNECTION_API_WIN_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/win/object_watcher.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/brave_vpn_ras_connection_api_base.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/ras_utils.h"

namespace brave_vpn {
namespace ras {
enum class CheckConnectionResult;
}  // namespace ras

class BraveVPNOSConnectionAPIWin : public BraveVPNOSConnectionAPIBase,
                                   public base::win::ObjectWatcher::Delegate {
 public:
  BraveVPNOSConnectionAPIWin(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      version_info::Channel channel);
  BraveVPNOSConnectionAPIWin(const BraveVPNOSConnectionAPIWin&) = delete;
  BraveVPNOSConnectionAPIWin& operator=(const BraveVPNOSConnectionAPIWin&) =
      delete;
  ~BraveVPNOSConnectionAPIWin() override;

 private:
  // BraveVPNOSConnectionAPIBase interfaces:
  void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) override;
  void ConnectImpl(const std::string& name) override;
  void DisconnectImpl(const std::string& name) override;
  void CheckConnectionImpl(const std::string& name) override;
  bool IsPlatformNetworkAvailable() override;

  // base::win::ObjectWatcher::Delegate overrides:
  void OnObjectSignaled(HANDLE object) override;

  void OnCreated(const std::string& name,
                 const ras::RasOperationResult& result);
  void OnConnected(const ras::RasOperationResult& result);
  void OnDisconnected(const ras::RasOperationResult& result);
  void OnCheckConnection(const std::string& name,
                         ras::CheckConnectionResult result);

  void StartVPNConnectionChangeMonitoring();

  HANDLE event_handle_for_connected_disconnected_ = NULL;
  base::win::ObjectWatcher connected_disconnected_event_watcher_;
  base::WeakPtrFactory<BraveVPNOSConnectionAPIWin> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_BRAVE_VPN_RAS_CONNECTION_API_WIN_H_
