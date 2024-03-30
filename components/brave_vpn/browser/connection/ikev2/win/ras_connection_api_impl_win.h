/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_RAS_CONNECTION_API_IMPL_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_RAS_CONNECTION_API_IMPL_WIN_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/system_vpn_connection_api_impl_base.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/ras_utils.h"
#include "brave/components/brave_vpn/common/win/ras/ras_connection_observer.h"

namespace brave_vpn {

namespace ras {
enum class CheckConnectionResult;
}  // namespace ras

class RasConnectionAPIImplWin : public SystemVPNConnectionAPIImplBase,
                                public ras::RasConnectionObserver {
 public:
  RasConnectionAPIImplWin(
      BraveVPNConnectionManager* manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~RasConnectionAPIImplWin() override;

 private:
  // SystemVPNConnectionAPIIplBase overrides:
  void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) override;
  void ConnectImpl(const std::string& name) override;
  void DisconnectImpl(const std::string& name) override;
  void CheckConnectionImpl(const std::string& name) override;
  bool IsPlatformNetworkAvailable() override;

  // ras::RasConnectionObserver overrides:
  void OnRasConnectionStateChanged() override;

  void OnCreated(const std::string& name,
                 const ras::RasOperationResult& result);
  void OnConnected(const ras::RasOperationResult& result);
  void OnDisconnected(const ras::RasOperationResult& result);
  void OnCheckConnection(const std::string& name,
                         ras::CheckConnectionResult result);

  base::WeakPtrFactory<RasConnectionAPIImplWin> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_RAS_CONNECTION_API_IMPL_WIN_H_
