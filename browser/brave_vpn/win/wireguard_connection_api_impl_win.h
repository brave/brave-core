/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_WIREGUARD_CONNECTION_API_IMPL_WIN_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_WIREGUARD_CONNECTION_API_IMPL_WIN_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/credentials/brave_vpn_wireguard_profile_credentials.h"
#include "brave/components/brave_vpn/common/win/brave_windows_service_watcher.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/wireguard_connection_api_impl_base.h"

namespace brave_vpn {

class WireguardConnectionAPIImplWin : public WireguardConnectionAPIImplBase {
 public:
  WireguardConnectionAPIImplWin(
      BraveVPNConnectionManager* manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~WireguardConnectionAPIImplWin() override;

  // WireguardConnectionAPIImplBase overrides:
  void Disconnect() override;
  void CheckConnection() override;
  void UpdateAndNotifyConnectionStateChange(
      mojom::ConnectionState state) override;
  void PlatformConnectImpl(
      const wireguard::WireguardProfileCredentials& credentials) override;

 private:
  void RunServiceWatcher();
  void OnWireguardServiceLaunched(bool success);
  void OnServiceStopped(int mask);
  void ResetServiceWatcher();

  std::unique_ptr<brave::ServiceWatcher> service_watcher_;
  base::WeakPtrFactory<WireguardConnectionAPIImplWin> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_WIREGUARD_CONNECTION_API_IMPL_WIN_H_
