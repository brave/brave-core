/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "chrome/common/pref_names.h"

namespace {

// Added guard to avoid build error from std::size with empty array.
// Remove this guard when we have any prefs for android.
#if !BUILDFLAG(IS_ANDROID)
const char* const kBravePersistentPrefNames[] = {
    prefs::kSidePanelHorizontalAlignment,
};
#endif

}  // namespace

#define GetIncognitoPersistentPrefsAllowlist \
  GetIncognitoPersistentPrefsAllowlist_ChromiumImpl

#include "src/chrome/browser/prefs/pref_service_incognito_allowlist.cc"

#undef GetIncognitoPersistentPrefsAllowlist

namespace prefs {

std::vector<const char*> GetIncognitoPersistentPrefsAllowlist() {
  std::vector<const char*> allowlist =
      GetIncognitoPersistentPrefsAllowlist_ChromiumImpl();
#if !BUILDFLAG(IS_ANDROID)
  allowlist.insert(
      allowlist.end(), kBravePersistentPrefNames,
      kBravePersistentPrefNames + std::size(kBravePersistentPrefNames));
#endif
  return allowlist;
}

}  // namespace prefs
