/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
#define GetSiteInstanceForNewTab(profile, url) \
  GetSiteInstanceForNewTab(                    \
      profile, url,                            \
      containers::InheritContainerStoragePartition(originator.get()))
#endif

#include "src/chrome/browser/password_manager/password_change_delegate_impl.cc"

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif
