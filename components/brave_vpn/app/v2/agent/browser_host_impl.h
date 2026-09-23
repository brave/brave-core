/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_IMPL_H_

#include "base/functional/callback.h"
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
class BrowserHostImpl : public brave_vpn::mojom::BrowserHost {
 public:
  // Constructs the BrowserHostImpl for a single authenticated browser. The
  // browser's BrowserEndpoint is passed in to allow the agent to call back into
  // the browser. The BrowserHost |receiver| is the pipe the browser handed to
  // BindBrowserHost(), so the browser can start issuing calls as soon as the
  // reply reaches it. |disconnect_handler| runs if either pipe gets dropped
  // while keeping the connection open, and is allowed to destroy this object.
  BrowserHostImpl(mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
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

  base::OnceClosure disconnect_handler_;
  mojo::Remote<mojom::BrowserEndpoint> browser_endpoint_;
  mojo::Receiver<mojom::BrowserHost> receiver_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_IMPL_H_
