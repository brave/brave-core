/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_PROFILE_PREFS_H_
#define BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_PROFILE_PREFS_H_

class Profile;

namespace brave_origin {

// Sets up default preferences for Brave Origin profiles
void SetupBraveOriginProfilePrefs(Profile* profile);

}  // namespace brave_origin

#endif  // BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_PROFILE_PREFS_H_
