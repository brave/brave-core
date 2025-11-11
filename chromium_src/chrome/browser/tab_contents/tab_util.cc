/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/tab_contents/tab_util.h"

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#define GetSiteInstanceForNewTab(...) \
  GetSiteInstanceForNewTab(           \
      __VA_ARGS__,                    \
      std::optional<content::StoragePartitionConfig> storage_partition_config)

#define BRAVE_GET_SITE_INSTANCE_FOR_NEW_TAB              \
  if (storage_partition_config) {                        \
    return SiteInstance::CreateForFixedStoragePartition( \
        profile, url, *storage_partition_config);        \
  }
#else
#define BRAVE_GET_SITE_INSTANCE_FOR_NEW_TAB
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/tab_contents/tab_util.cc>  // IWYU pragma: export

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#undef BRAVE_GET_SITE_INSTANCE_FOR_NEW_TAB
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
