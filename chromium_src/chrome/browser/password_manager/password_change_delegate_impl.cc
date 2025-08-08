/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/partitioned_tabs/buildflags/buildflags.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_PARTITIONED_TABS)
#include "brave/components/partitioned_tabs/browser/partitioned_tabs_handler_registry.h"
#endif

#if BUILDFLAG(ENABLE_PARTITIONED_TABS)
#define GetSiteInstanceForNewTab(profile, url)                        \
  GetSiteInstanceForNewTab(                                           \
      profile, url,                                                   \
      partitioned_tabs::PartitionedTabsHandlerRegistry::GetInstance() \
          .MaybeInheritStoragePartition(originator.get()))
#endif

#include <chrome/browser/password_manager/password_change_delegate_impl.cc>

#if BUILDFLAG(ENABLE_PARTITIONED_TABS)
#undef GetSiteInstanceForNewTab
#endif
