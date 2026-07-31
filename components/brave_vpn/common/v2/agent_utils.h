/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_AGENT_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_AGENT_UTILS_H_

#include <optional>

#include "mojo/public/cpp/platform/named_platform_channel.h"

namespace brave_vpn::v2 {

// Returns the NamedMojoIpcServer endpoint name for the VPN agent serving the
// current OS session. The browser and the agent call this independently and
// must arrive at the same value.
//
// The result is a socket path on POSIX and a bare pipe leaf name on Windows.
// The name is not a secret and is discoverable by other local processes;
// authentication of peers is expected.
std::optional<mojo::NamedPlatformChannel::ServerName> GetAgentServerName();

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_AGENT_UTILS_H_
