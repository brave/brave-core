/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_host_impl.h"

#include <utility>

#include "base/functional/bind.h"

namespace brave_vpn::v2 {

BrowserHostImpl::BrowserHostImpl(
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> receiver,
    base::OnceClosure disconnect_handler)
    : disconnect_handler_(std::move(disconnect_handler)),
      browser_endpoint_(std::move(browser_endpoint)),
      receiver_(this, std::move(receiver)) {
  browser_endpoint_.set_disconnect_handler(base::BindOnce(
      &BrowserHostImpl::OnPipeDisconnected, base::Unretained(this)));
  receiver_.set_disconnect_handler(base::BindOnce(
      &BrowserHostImpl::OnPipeDisconnected, base::Unretained(this)));
}

BrowserHostImpl::~BrowserHostImpl() = default;

void BrowserHostImpl::OnPipeDisconnected() {
  // Whichever pipe closes first reports the session as gone. The other pipe's
  // handler is destroyed along with this object, so this normally runs once;
  // the guard covers a caller whose handler leaves this object alive.
  if (!disconnect_handler_) {
    return;
  }

  // May delete this: nothing below may touch members.
  std::move(disconnect_handler_).Run();
}

}  // namespace brave_vpn::v2
