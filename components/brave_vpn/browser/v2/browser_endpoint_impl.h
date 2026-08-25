/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_BROWSER_ENDPOINT_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_BROWSER_ENDPOINT_IMPL_H_

#include "base/functional/callback.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS));

namespace brave_vpn::v2 {

// The browser's subordinate surface: everything the agent may call back into
// this browser, one instance per connection attempt, created and owned by
// AgentClient.
class BrowserEndpointImpl : public brave_vpn::mojom::BrowserEndpoint {
 public:
  // |disconnect_handler| runs the first time the pipe drops, which happens
  // either because the agent refused this connection, or because the agent went
  // away. It is allowed to destroy this object.
  explicit BrowserEndpointImpl(base::OnceClosure disconnect_handler);
  ~BrowserEndpointImpl() override;

  BrowserEndpointImpl(const BrowserEndpointImpl&) = delete;
  BrowserEndpointImpl& operator=(const BrowserEndpointImpl&) = delete;

  // Binds the receiver and returns the remote to send to the agent. Called
  // exactly once, before the instance is handed to the agent API.
  mojo::PendingRemote<mojom::BrowserEndpoint> BindNewPipeAndPassRemote();

 private:
  // brave_vpn::mojom::BrowserEndpoint:
  // Nothing yet: the interface is a placeholder for the agent-initiated half of
  // the contract. New methods forward to a delegate injected here rather than
  // reaching into browser state directly.

  void OnPipeDisconnected();

  base::OnceClosure disconnect_handler_;
  mojo::Receiver<mojom::BrowserEndpoint> receiver_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_BROWSER_ENDPOINT_IMPL_H_
