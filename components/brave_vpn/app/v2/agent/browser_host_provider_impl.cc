/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_host_provider_impl.h"

#include <utility>

#include "base/check_deref.h"

namespace brave_vpn::v2 {

BrowserHostProviderImpl::BrowserHostProviderImpl(Delegate* delegate)
    : delegate_(CHECK_DEREF(delegate)) {}

BrowserHostProviderImpl::~BrowserHostProviderImpl() = default;

void BrowserHostProviderImpl::BindBrowserHost(
    uint32_t protocol_version,
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> host,
    BindBrowserHostCallback callback) {
  // The delegate owns the policy and binds |host| itself on success; this
  // surface only forwards and relays the result back to the browser.
  delegate_->Authenticate(protocol_version, std::move(browser_endpoint),
                          std::move(host), std::move(callback));
}

}  // namespace brave_vpn::v2
