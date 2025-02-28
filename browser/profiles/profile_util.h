/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_PROFILE_UTIL_H_
#define BRAVE_BROWSER_PROFILES_PROFILE_UTIL_H_

#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"

class Profile;

namespace brave {

// Used for capturing the value of kBraveCurrentDataVersion so that the
// default search engine for that version can be determined. New profiles
// will get locked into newer versions when created. Existing profiles
// missing this value are backfilled to the first version introduced.
void SetDefaultSearchVersion(Profile* profile, bool is_new_profile);

// Sets the default value for Cookie Controls Mode to 'Block Third Party'
// instead of 'Block Third Party in Incognito'
void SetDefaultThirdPartyCookieBlockValue(Profile* profile);

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
// Used for capturing the current value of WebTorrent.
// WebTorrent will be disabled for new users starting with
// https://github.com/brave/brave-browser/issues/44303
// Once the feature is deprecated, this code can be removed and the preference
// can be added under `RegisterProfilePrefsForMigration`.
void SetWebTorrentEnabled(Profile* profile, bool is_new_profile);
#endif

// Checks if the user previously had fingerprinting mode disabled.
// If so, set the new fingerprinting toggle to disabled.
void MigrateFingerprintingSettings(Profile* profile);

}  // namespace brave

#endif  // BRAVE_BROWSER_PROFILES_PROFILE_UTIL_H_
