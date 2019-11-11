/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_profile_prefs.h"
#include "brave/browser/brave_local_state_prefs.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) || BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "brave/browser/widevine/widevine_utils.h"
#endif

#define MigrateObsoleteProfilePrefs MigrateObsoleteProfilePrefs_ChromiumImpl
#include "../../../../chrome/browser/prefs/browser_prefs.cc"  // NOLINT
#undef MigrateObsoleteProfilePrefs

void MigrateObsoleteProfilePrefs(Profile* profile) {
  MigrateObsoleteProfilePrefs_ChromiumImpl(profile);

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) || BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  MigrateWidevinePrefs(profile);
#endif
}

