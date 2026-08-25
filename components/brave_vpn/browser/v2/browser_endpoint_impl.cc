/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/browser_endpoint_impl.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"

namespace brave_vpn::v2 {

BrowserEndpointImpl::BrowserEndpointImpl(base::OnceClosure disconnect_handler)
    : disconnect_handler_(std::move(disconnect_handler)) {
  CHECK(disconnect_handler_);
}

BrowserEndpointImpl::~BrowserEndpointImpl() = default;

mojo::PendingRemote<mojom::BrowserEndpoint>
BrowserEndpointImpl::BindNewPipeAndPassRemote() {
  CHECK(!receiver_.is_bound());
  auto remote = receiver_.BindNewPipeAndPassRemote();
  receiver_.set_disconnect_handler(base::BindOnce(
      &BrowserEndpointImpl::OnPipeDisconnected, base::Unretained(this)));
  return remote;
}

void BrowserEndpointImpl::OnPipeDisconnected() {
  // Stop listening before running the handler.
  receiver_.reset();
  if (disconnect_handler_) {
    std::move(disconnect_handler_).Run();
  }
}

}  // namespace brave_vpn::v2
