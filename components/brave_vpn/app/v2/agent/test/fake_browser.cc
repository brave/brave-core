/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser.h"

#include "base/check.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"

namespace brave_vpn::v2 {

FakeBrowser::FakeBrowser() = default;

FakeBrowser::~FakeBrowser() = default;

mojo::PendingRemote<mojom::BrowserEndpoint> FakeBrowser::BindEndpoint() {
  return endpoint_receiver_.BindNewPipeAndPassRemote();
}

mojo::PendingReceiver<mojom::BrowserHost> FakeBrowser::BindHost() {
  return host_remote_.BindNewPipeAndPassReceiver();
}

mojo::PlatformHandle FakeBrowser::BindIdentityChannel() {
  mojo::PlatformChannelEndpoint endpoint =
      identity_channel_.TakeLocalEndpoint();
  CHECK(endpoint.is_valid()) << "Identity channel already taken";
  return endpoint.TakePlatformHandle();
}

base::OnceCallback<void(mojom::BrowserInitResult)>
FakeBrowser::GetInitReplyCallback() {
  return init_replied_.GetCallback();
}

base::OnceCallback<void(mojom::BrowserAuthResult)>
FakeBrowser::GetAuthReplyCallback() {
  return auth_replied_.GetCallback();
}

bool FakeBrowser::has_init_reply() const {
  return init_replied_.IsReady();
}

bool FakeBrowser::has_auth_reply() const {
  return auth_replied_.IsReady();
}

mojom::BrowserInitResult FakeBrowser::WaitForInitReply() {
  return init_replied_.Get();
}

mojom::BrowserAuthResult FakeBrowser::WaitForAuthReply() {
  return auth_replied_.Get();
}

void FakeBrowser::WatchHost() {
  host_remote_.set_disconnect_handler(host_closed_.GetCallback());
}

void FakeBrowser::WatchEndpoint() {
  endpoint_receiver_.set_disconnect_handler(endpoint_closed_.GetCallback());
}

bool FakeBrowser::WaitForHostClosed() {
  return host_closed_.Wait();
}

bool FakeBrowser::WaitForEndpointClosed() {
  return endpoint_closed_.Wait();
}

bool FakeBrowser::host_closed() const {
  return host_closed_.IsReady();
}

bool FakeBrowser::endpoint_closed() const {
  return endpoint_closed_.IsReady();
}

void FakeBrowser::DropHost() {
  host_remote_.reset();
}

void FakeBrowser::DropEndpoint() {
  endpoint_receiver_.reset();
}

void FakeBrowser::FlushHost() {
  host_remote_.FlushForTesting();
}

bool FakeBrowser::host_connected() const {
  return host_remote_.is_connected();
}

}  // namespace brave_vpn::v2
