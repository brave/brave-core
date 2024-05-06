/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

// On Android we want to have enable_feed_v2 parameter enabled to
// provide linking with feed::FetchRssLinks at
// BraveNewsTabHelper::DOMContentLoaded, but kEnableSnippets and
// kArticlesListVisible must be defaulted to false to avoid failed assertion at
// BraveNewTabPage.initializeMainView. So override
// feed::prefs::RegisterFeedSharedProfilePrefs for Android only. Related
// Chromium's commit: d3500b942cde04737bc13021173b6ffa11aaf1b9.

#if BUILDFLAG(IS_ANDROID)

#define RegisterFeedSharedProfilePrefs \
  RegisterFeedSharedProfilePrefs_ChromiumImpl
#include "src/components/feed/core/shared_prefs/pref_names.cc"
#undef RegisterFeedSharedProfilePrefs

namespace feed {
namespace prefs {

void RegisterFeedSharedProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kEnableSnippets, false);
  registry->RegisterBooleanPref(kArticlesListVisible, false);
  registry->RegisterBooleanPref(kEnableSnippetsByDse, false);
}

}  // namespace prefs
}  // namespace feed

#else
#include "src/components/feed/core/shared_prefs/pref_names.cc"
#endif  // BUILDFLAG(IS_ANDROID)
