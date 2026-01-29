// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/browser_tabrestore.h"

#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "chrome/browser/tab_contents/tab_util.h"

#define GetSiteInstanceForNewTab(...) \
  GetSiteInstanceForNewTab(__VA_ARGS__, std::nullopt)

#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/ui/browser_tabrestore.cc>

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
