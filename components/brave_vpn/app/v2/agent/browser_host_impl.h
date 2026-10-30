/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_IMPL_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_vpn/app/v2/agent/connection_manager.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_vpn::v2 {

// The authenticated API surface: one instance per authenticated browser,
// created and owned by BrowserRegistry once authentication succeeds. A browser
// only ever holds a remote to this after being accepted, so methods here do no
// further checking.
class BrowserHostImpl final : public brave_vpn::mojom::BrowserHost,
                              public ConnectionManager::Observer {
 public:
  // Constructs the BrowserHostImpl for a single authenticated browser. The
  // ConnectionManager is shared by all sessions and must outlive this object.
  // The browser's BrowserEndpoint is passed in to allow the agent to call back
  // into the browser. The BrowserHost |receiver| is the pipe the browser handed
  // to BindBrowserHost(), so the browser can start issuing calls as soon as the
  // reply reaches it. |disconnect_handler| runs if either pipe gets dropped
  // while keeping the connection open, and is allowed to destroy this object.
  BrowserHostImpl(ConnectionManager& connection_manager,
                  mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
                  mojo::PendingReceiver<mojom::BrowserHost> receiver,
                  base::OnceClosure disconnect_handler);
  ~BrowserHostImpl() override;

  BrowserHostImpl(const BrowserHostImpl&) = delete;
  BrowserHostImpl& operator=(const BrowserHostImpl&) = delete;

 private:
  // Runs the disconnect handler the first time either pipe closes. Both pipes
  // share one handler because the session is only useful while both are up: a
  // browser that drops its endpoint can no longer be called back, and one that
  // drops the host can no longer call in.
  void OnPipeDisconnected();

  // brave_vpn::mojom::BrowserHost:
  void Connect() override;
  void Disconnect() override;
  void ResetConnectionState() override;
  void GetAllRegions(GetAllRegionsCallback callback) override;
  void SetSelectedRegion(mojom::VpnRegionPtr region) override;
  void SetAutoReconnectState(bool enable) override;
  void SetSmartProxyRoutingState(bool enable) override;
  void SetAllowLanTrafficState(bool enable) override;

  // ConnectionManager::Observer:
  void OnConnectionStateChanged(
      mojom::VpnConnectionState state,
      std::optional<std::string> error_message) override;
  void OnSelectedRegionChanged(const mojom::VpnRegion& region) override;
  void OnAutoReconnectStateChanged(bool enabled) override;
  void OnSmartProxyRoutingStateChanged(bool enabled) override;
  void OnAllowLanTrafficStateChanged(bool available, bool enabled) override;
  void OnLastHostnameChanged(const std::string& hostname) override;

  const raw_ref<ConnectionManager> connection_manager_;
  base::ScopedObservation<ConnectionManager, ConnectionManager::Observer>
      connection_manager_observation_{this};
  base::OnceClosure disconnect_handler_;
  mojo::Remote<mojom::BrowserEndpoint> browser_endpoint_;
  mojo::Receiver<mojom::BrowserHost> receiver_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_IMPL_H_
