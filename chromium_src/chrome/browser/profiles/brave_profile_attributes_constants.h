/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_BRAVE_PROFILE_ATTRIBUTES_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_BRAVE_PROFILE_ATTRIBUTES_CONSTANTS_H_

namespace brave {

// Filename (within the profile directory) used to store the user-uploaded
// custom profile avatar PNG bytes on disk. Shared between the chromium_src
// `ProfileAttributesEntry` override (which writes/reads it) and tests that
// need to verify the file is created or removed.
inline constexpr char kBraveCustomAvatarFileName[] = "Brave Custom Avatar.png";

}  // namespace brave

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_BRAVE_PROFILE_ATTRIBUTES_CONSTANTS_H_
