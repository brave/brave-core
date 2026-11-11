/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_LINUX_BROWSER_IDENTITY_LINUX_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_LINUX_BROWSER_IDENTITY_LINUX_INTERNAL_H_

#include <sys/types.h>

#include <cstdint>
#include <optional>
#include <string_view>

#include "base/files/scoped_file.h"

// Process helpers for the Linux BrowserIdentity. Not for use outside of it and
// its tests.
namespace brave_vpn::v2::internal {

// Returns a pidfd referring to |pid|, or an invalid fd with errno set on
// failure. The pidfd keeps referring to the same process even if the pid number
// is later reused.
base::ScopedFD OpenPidfd(pid_t pid);

// Returns true if the process |pidfd| refers to has not been reaped yet;
// zombies count as alive. While this holds, its pid cannot have been reused.
bool IsProcessAlive(const base::ScopedFD& pidfd);

// Returns the time at which process |pid| started, in clock ticks since boot,
// or nullopt if it cannot be read. Fixed for the life of the process and
// independent of the wall clock. Does not block, so it is safe to call on any
// sequence.
std::optional<uint64_t> ReadProcessStartTimeTicks(pid_t pid);

// Parses the start time out of the contents of a "/proc/<pid>/stat" file, or
// returns nullopt if it is malformed.
std::optional<uint64_t> ParseProcessStartTimeTicks(
    std::string_view stat_contents);

// Returns the effective uid of process |pid|, or nullopt if it cannot be read.
// Does not block, so it is safe to call on any sequence.
std::optional<uid_t> ReadProcessEffectiveUid(pid_t pid);

// Parses the effective uid out of the contents of a "/proc/<pid>/status" file,
// or returns nullopt if it is malformed.
std::optional<uid_t> ParseProcessEffectiveUid(std::string_view status_contents);

}  // namespace brave_vpn::v2::internal

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_LINUX_BROWSER_IDENTITY_LINUX_INTERNAL_H_
