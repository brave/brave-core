/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/tab_contents/tab_util.h"

#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/site_instance.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#define GetSiteInstanceForNewTab(...) \
  GetSiteInstanceForNewTab(           \
      __VA_ARGS__,                    \
      std::optional<content::StoragePartitionConfig> storage_partition_config)

#define CreateForURL(...)                                    \
  CreateForURLWithOptionalFixedStoragePartition(__VA_ARGS__, \
                                                storage_partition_config)

#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/tab_contents/tab_util.cc>  // IWYU pragma: export

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef CreateForURL
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
