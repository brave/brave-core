/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_SYNC_UTIL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_SYNC_UTIL_H_

#include "brave/components/brave_sync/buildflags.h"

namespace syncer::internal {
inline constexpr char kSyncServerUrl[] = BUILDFLAG(BRAVE_SYNC_ENDPOINT);
inline constexpr char kSyncDevServerUrl[] = BUILDFLAG(BRAVE_SYNC_ENDPOINT);
}  // namespace syncer::internal

#define kSyncServerUrl kSyncServerUrl_ChromiumImpl
#define kSyncDevServerUrl kSyncDevServerUrl_ChromiumImpl

#include "src/components/sync/base/sync_util.h"  // IWYU pragma: export

#undef kSyncDevServerUrl
#undef kSyncServerUrl

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BASE_SYNC_UTIL_H_
