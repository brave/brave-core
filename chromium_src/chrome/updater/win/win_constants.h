/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_UPDATER_WIN_WIN_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_UPDATER_WIN_WIN_CONSTANTS_H_

// Upstream derives the constants below from Google's Omaha 3 or from
// COMPANY_SHORTNAME. Our Omaha 3 fork uses different values. Omaha 4 needs the
// correct ones to shut down, take over from, and clean up Omaha 3.
#define kShutdownEvent kShutdownEvent_ChromiumImpl
#define kLegacyExeName kLegacyExeName_ChromiumImpl
#define kLegacyServiceDisplayNamePrefix \
  kLegacyServiceDisplayNamePrefix_ChromiumImpl

#include <chrome/updater/win/win_constants.h>  // IWYU pragma: export

#undef kShutdownEvent
#undef kLegacyExeName
#undef kLegacyServiceDisplayNamePrefix

namespace updater {

// `kShutdownEvent` in omaha/base/const_object_names.h of our Omaha 3 fork. The
// fork also prefixes it differently from upstream, see
// GetLegacyShutdownEventName in chromium_src/chrome/updater/util/win_util.cc.
inline constexpr wchar_t kShutdownEvent[] =
    L"{4613C8D6-D26E-4F10-B494-72CFF6F0BF0B}";

// MAIN_EXE_BASE_NAME + ".exe" in omaha/main.scons of our Omaha 3 fork.
inline constexpr wchar_t kLegacyExeName[] = L"BraveUpdate.exe";

// The display name of Omaha 3's services in English. Omaha 4 only deletes
// services whose display name starts with this. We believe this is an upstream
// bug: on non-English systems the name is translated, so the services stay
// behind. They then launch BraveUpdate.exe, which rejects their command line
// and logs an error. This is harmless, so we accept it rather than take the
// risk of changing upstream's critical migration code.
inline constexpr wchar_t kLegacyServiceDisplayNamePrefix[] =
    L"Brave Update Service";

}  // namespace updater

#endif  // BRAVE_CHROMIUM_SRC_CHROME_UPDATER_WIN_WIN_CONSTANTS_H_
