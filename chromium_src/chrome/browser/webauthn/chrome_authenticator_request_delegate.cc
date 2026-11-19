/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/webauthn/chrome_authenticator_request_delegate.h"

#include <optional>

#include "content/public/browser/global_routing_id.h"
#include "device/fido/network_context_factory.h"

// Forward declared to avoid adding a compile-time dependency.
// impl target is //brave/browser/webauthn:chromium_impl.
std::optional<device::NetworkContextFactory>
BraveMaybeGetOffTheRecordCableNetworkContextFactory(
    content::GlobalRenderFrameHostId render_frame_host_id);

#include <chrome/browser/webauthn/chrome_authenticator_request_delegate.cc>
