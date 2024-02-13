/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sync/local_or_syncable_bookmark_sync_service_factory.h"
#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/bookmark/bookmark_prefs_service_factory.h"

// Adding BookmarkPrefsServiceFactory as dependency because kShowBookmarBar and
// kAlwaysShowBookmarkBarOnNTP manage bookmark bar state together and need to
// register both prefs at same time.
#define DependsOn                                        \
  DependsOn(BookmarkPrefsServiceFactory::GetInstance()); \
  DependsOn
#endif
#include "src/chrome/browser/sync/local_or_syncable_bookmark_sync_service_factory.cc"
#if !BUILDFLAG(IS_ANDROID)
#undef DependsOn
#endif
