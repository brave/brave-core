/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_PROVIDER_IMPL_H_

#include <stdint.h>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_vpn::v2 {

// The unauthenticated API surface: everything reachable on a fresh connection,
// which today is authentication and nothing else. A single instance is shared
// by every connected browser (it is what BrowserRegistry hands the IPC server),
// so it holds no per-connection state and defers all policy to its delegate.
class BrowserHostProviderImpl : public brave_vpn::mojom::BrowserHostProvider {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Decides whether the connection currently being dispatched may have a
    // BrowserHost, and binds |host| to one if so. Called synchronously from
    // BindBrowserHost(), so implementations may read dispatch state (peer info,
    // current receiver) before returning, but not afterwards. |callback| is
    // always run from a posted task.
    virtual void Authenticate(
        uint32_t protocol_version,
        mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
        mojo::PendingReceiver<mojom::BrowserHost> host,
        base::OnceCallback<void(mojom::BrowserAuthResult)> callback) = 0;
  };

  explicit BrowserHostProviderImpl(Delegate* delegate);
  ~BrowserHostProviderImpl() override;

  BrowserHostProviderImpl(const BrowserHostProviderImpl&) = delete;
  BrowserHostProviderImpl& operator=(const BrowserHostProviderImpl&) = delete;

 private:
  // brave_vpn::mojom::BrowserHostProvider:
  void BindBrowserHost(
      uint32_t protocol_version,
      mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
      mojo::PendingReceiver<mojom::BrowserHost> host,
      BindBrowserHostCallback callback) override;

  // Outlives this; the registry owns both.
  const raw_ref<Delegate> delegate_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_HOST_PROVIDER_IMPL_H_
