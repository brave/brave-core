/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/tab_contents/tab_util.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#define GetSiteInstanceForNewTab(profile, url) \
  GetSiteInstanceForNewTab(profile, url, std::nullopt)
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/password_manager/password_change_delegate_impl.cc>

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
