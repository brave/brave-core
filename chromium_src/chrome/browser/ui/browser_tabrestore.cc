/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_tabrestore.h"

#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/tab_contents/tab_util.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
// TODO(https://github.com/brave/brave-browser/issues/47306): Pass a container
// storage partition config here instead of std::nullopt.
#define GetSiteInstanceForNewTab(...) \
  GetSiteInstanceForNewTab(__VA_ARGS__, std::nullopt)
#endif

#include "src/chrome/browser/ui/browser_tabrestore.cc"

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif
