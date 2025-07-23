/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_SESSION_INFO_UTILS_H_
#define BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_SESSION_INFO_UTILS_H_

namespace partitioned_tabs {

// The key for the session info that stores the storage partition config.
inline constexpr char kStoragePartitionSessionInfoKey[] =
    "brave_storage_partition";

}  // namespace partitioned_tabs

#endif  // BRAVE_COMPONENTS_PARTITIONED_TABS_BROWSER_SESSION_INFO_UTILS_H_
