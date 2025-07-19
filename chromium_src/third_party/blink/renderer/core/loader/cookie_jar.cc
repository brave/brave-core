/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/cookie_jar.h"

#include "mojo/public/cpp/base/shared_memory_version.h"

// CookieJar::IPCNeeded() calls SharedVersionGreaterThan(last_version_) to
// determine if the cookie string has changed, in which case it determines that
// IPC is needed. We adjust this to use an invalid version, because Ephemeral
// Storage can switch the cookie storage backend at runtime, so we can't use
// reduced IPCs when accessing cookies. This was previously handled by disabling
// kReduceCookieIPCs, but that feature flag is no longer available.
#define SharedVersionIsGreaterThan(VERSION) \
  SharedVersionIsGreaterThan(mojo::shared_memory_version::kInvalidVersion)

#include <third_party/blink/renderer/core/loader/cookie_jar.cc>  // IWYU pragma: export

#undef SharedVersionIsGreaterThan
