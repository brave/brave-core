/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/contained_tab_handler_registry.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "content/public/browser/web_contents.h"

#define GetSiteInstanceForNewTab(profile, url)               \
  GetSiteInstanceForNewTab(                                  \
      profile, url,                                          \
      containers::ContainedTabHandlerRegistry::GetInstance() \
          .MaybeInheritStoragePartition(originator.get()))
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/password_manager/password_change_delegate_impl.cc>

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
