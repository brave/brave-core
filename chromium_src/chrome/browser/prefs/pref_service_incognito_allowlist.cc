/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/prefs/brave_pref_service_incognito_allowlist.h"

#define GetIncognitoPersistentPrefsAllowlist \
  GetIncognitoPersistentPrefsAllowlist_ChromiumImpl

#include <chrome/browser/prefs/pref_service_incognito_allowlist.cc>

#undef GetIncognitoPersistentPrefsAllowlist

namespace prefs {

std::vector<const char*> GetIncognitoPersistentPrefsAllowlist() {
  std::vector<const char*> allowlist =
      GetIncognitoPersistentPrefsAllowlist_ChromiumImpl();
  for (auto pref : brave::GetBravePersistentPrefNames()) {
    allowlist.push_back(pref.data());
  }
  return allowlist;
}

}  // namespace prefs
