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

void BrowserHostProviderImpl::Initialize(uint32_t protocol_version,
                                         mojo::PlatformHandle identity_channel,
                                         InitializeCallback callback) {
  // The delegate owns the policy and rejects the connection if needed; this
  // surface only forwards and relays the result back to the browser.
  delegate_->InitializeBrowser(protocol_version, std::move(identity_channel),
                               std::move(callback));
}

void BrowserHostProviderImpl::BindBrowserHost(
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> host,
    BindBrowserHostCallback callback) {
  // The delegate owns the policy, rejects the connection if needed, and binds
  // |host| itself on success; this surface only forwards and relays the result
  // back to the browser.
  delegate_->AuthenticateBrowser(std::move(browser_endpoint), std::move(host),
                                 std::move(callback));
}

}  // namespace brave_vpn::v2
