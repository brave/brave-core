/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/no_destructor.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/common/bookmark_pref_names.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/components/sidebar/pref_names.h"
#endif

namespace {

const std::vector<const char*>& GetBravePersistentPrefNames() {
  static base::NoDestructor<std::vector<const char*>> brave_allowlist({
#if !BUILDFLAG(IS_ANDROID)
    prefs::kSidePanelHorizontalAlignment, kTabMuteIndicatorNotClickable,
#endif
#if defined(TOOLKIT_VIEWS)
        sidebar::kSidePanelWidth,
#endif
  });

  return *brave_allowlist;
}

}  // namespace

#define GetIncognitoPersistentPrefsAllowlist \
  GetIncognitoPersistentPrefsAllowlist_ChromiumImpl
#define kShowBookmarkBar kShowBookmarkBar, kAlwaysShowBookmarkBarOnNTP
#include "src/chrome/browser/prefs/pref_service_incognito_allowlist.cc"
#undef kShowBookmarkBar
#undef GetIncognitoPersistentPrefsAllowlist

namespace prefs {

std::vector<const char*> GetIncognitoPersistentPrefsAllowlist() {
  std::vector<const char*> allowlist =
      GetIncognitoPersistentPrefsAllowlist_ChromiumImpl();
  allowlist.insert(allowlist.end(), GetBravePersistentPrefNames().begin(),
                   GetBravePersistentPrefNames().end());
  return allowlist;
}

}  // namespace prefs
