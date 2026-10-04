/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TAB_CONTENTS_TAB_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TAB_CONTENTS_TAB_UTIL_H_

#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/storage_partition_config.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#define GetSiteInstanceForNewTab(...) \
  GetSiteInstanceForNewTab(           \
      __VA_ARGS__,                    \
      std::optional<content::StoragePartitionConfig> storage_partition_config)
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/tab_contents/tab_util.h>  // IWYU pragma: export

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TAB_CONTENTS_TAB_UTIL_H_
