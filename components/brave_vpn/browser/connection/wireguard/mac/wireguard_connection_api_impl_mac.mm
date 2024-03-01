/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/mac/wireguard_connection_api_impl_mac.h"

#include <memory>

#include "base/base64.h"
#include "base/notreached.h"

namespace brave_vpn {

WireguardConnectionAPIImplMac::~WireguardConnectionAPIImplMac() = default;

void WireguardConnectionAPIImplMac::PlatformConnectImpl(
    const wireguard::WireguardProfileCredentials& credentials) {
  NOTIMPLEMENTED();
}

void WireguardConnectionAPIImplMac::Disconnect() {
  NOTIMPLEMENTED();
}

void WireguardConnectionAPIImplMac::CheckConnection() {
  NOTIMPLEMENTED();
}

}  // namespace brave_vpn
