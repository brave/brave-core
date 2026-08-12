/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_AGENT_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_AGENT_UTILS_H_

#include <stddef.h>

#include <optional>

#include "base/files/file_path.h"
#include "build/build_config.h"
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

#if BUILDFLAG(IS_POSIX)

// Longest socket path mojo::NamedPlatformChannel can bind. |sun_path| is 104
// bytes on macOS and 108 on Linux. The same code has to be correct on both, so
// the shorter limit applies.
inline constexpr size_t kMaxAgentSocketPathLength = 103;

// Builds the server name for a socket living in |socket_dir|, or nullopt if the
// directory is empty or the resulting path would exceed the sockaddr_un limit.
std::optional<mojo::NamedPlatformChannel::ServerName>
GetAgentServerNameForDirectory(const base::FilePath& socket_dir);

#endif  // BUILDFLAG(IS_POSIX)

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_V2_AGENT_UTILS_H_
