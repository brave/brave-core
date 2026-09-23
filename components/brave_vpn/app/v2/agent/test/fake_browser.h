/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_H_

#include <stdint.h>

#include "base/functional/callback.h"
#include "base/test/test_future.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser_endpoint.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_vpn::v2 {

// The browser's half of one session: it holds the BrowserHost remote and serves
// the BrowserEndpoint the agent calls back on.
class FakeBrowser {
 public:
  FakeBrowser();
  ~FakeBrowser();

  FakeBrowser(const FakeBrowser&) = delete;
  FakeBrowser& operator=(const FakeBrowser&) = delete;

  // Handles to pass into a BindBrowserHost-style request. Each may only be
  // called once.
  mojo::PendingRemote<mojom::BrowserEndpoint> BindEndpoint();
  mojo::PendingReceiver<mojom::BrowserHost> BindHost();

  // Reply callback for that request, plus its answer. WaitForReply() runs the
  // sequence until the answer arrives.
  base::OnceCallback<void(mojom::BrowserAuthResult)> GetReplyCallback();
  bool has_reply() const;
  mojom::BrowserAuthResult WaitForReply();

  // Watch for the agent closing its end of either pipe, which is what a
  // destroyed session looks like from here. Call after the pipes are bound.
  void WatchHost();
  void WatchEndpoint();

  // Run the sequence until the agent closes the corresponding pipe.
  [[nodiscard]] bool WaitForHostClosed();
  [[nodiscard]] bool WaitForEndpointClosed();

  // Whether the agent has already closed its end. Only meaningful after the
  // matching Watch call, and paired with a barrier such as FlushHost() when
  // asserting that it has *not* happened.
  bool host_closed() const;
  bool endpoint_closed() const;

  // Closes this side of a pipe, as a browser dropping it would.
  void DropHost();
  void DropEndpoint();

  // Round trip through the agent's BrowserHost receiver: returns once the agent
  // has processed everything already queued on that pipe.
  void FlushHost();

  bool host_connected() const;

 private:
  FakeBrowserEndpoint endpoint_impl_;
  mojo::Receiver<mojom::BrowserEndpoint> endpoint_receiver_{&endpoint_impl_};
  mojo::Remote<mojom::BrowserHost> host_remote_;

  base::test::TestFuture<mojom::BrowserAuthResult> replied_;
  base::test::TestFuture<void> host_closed_;
  base::test::TestFuture<void> endpoint_closed_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_H_
