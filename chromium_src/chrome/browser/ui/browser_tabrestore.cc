/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_tabrestore.h"

#include "brave/components/partitioned_tabs/buildflags/buildflags.h"
#include "chrome/browser/tab_contents/tab_util.h"

#if BUILDFLAG(ENABLE_PARTITIONED_TABS)
#include "brave/components/partitioned_tabs/browser/storage_partition_session_info_handler.h"
#endif

#if BUILDFLAG(ENABLE_PARTITIONED_TABS)
#define GetSiteInstanceForNewTab(...)                                         \
  GetSiteInstanceForNewTab(                                                   \
      __VA_ARGS__,                                                            \
      partitioned_tabs::StoragePartitionSessionInfoHandler::                  \
          GetStoragePartitionConfigToRestore(browser->profile(), navigations, \
                                             selected_navigation))
#endif

#include <chrome/browser/ui/browser_tabrestore.cc>

#if BUILDFLAG(ENABLE_PARTITIONED_TABS)
#undef GetSiteInstanceForNewTab
#endif
