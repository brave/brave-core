/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/tab_contents/tab_util.h"

#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/site_instance.h"

#define BRAVE_GET_SITE_INSTANCE_FOR_NEW_TAB

#if BUILDFLAG(ENABLE_CONTAINERS)
// Extend GetSiteInstanceForNewTab to accept an optional StoragePartitionConfig.
// When a config is present it delegates to CreateForFixedStoragePartition;
// otherwise it falls back to CreateForURL.
#define GetSiteInstanceForNewTab(...) \
  GetSiteInstanceForNewTab(           \
      __VA_ARGS__,                    \
      std::optional<content::StoragePartitionConfig> storage_partition_config)

// Redirect CreateForURL to our helper which conditionally calls
// CreateForFixedStoragePartition when a StoragePartitionConfig is present,
// ensuring navigations respect the passed config.
#define CreateForURL(...)                                    \
  CreateForURLWithOptionalFixedStoragePartition(__VA_ARGS__, \
                                                storage_partition_config)

#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/tab_contents/tab_util.cc>  // IWYU pragma: export

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef CreateForURL
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
