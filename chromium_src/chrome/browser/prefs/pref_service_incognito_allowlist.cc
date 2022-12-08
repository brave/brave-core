/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/no_destructor.h"
#include "build/build_config.h"
#include "chrome/common/pref_names.h"

namespace {

const std::vector<const char*>& GetBravePersistentPrefNames() {
  static base::NoDestructor<std::vector<const char*>> brave_allowlist({
#if !BUILDFLAG(IS_ANDROID)
    prefs::kSidePanelHorizontalAlignment,
#endif
  });

  return *brave_allowlist;
}

}  // namespace

#define GetIncognitoPersistentPrefsAllowlist \
  GetIncognitoPersistentPrefsAllowlist_ChromiumImpl

#include "src/chrome/browser/prefs/pref_service_incognito_allowlist.cc"

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
