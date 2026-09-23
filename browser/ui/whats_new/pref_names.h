/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WHATS_NEW_PREF_NAMES_H_
#define BRAVE_BROWSER_UI_WHATS_NEW_PREF_NAMES_H_

namespace whats_new::prefs {

// Store lastly shown whats-new version - major version.
// Ex, "1.50" means browser launched whats-new page for v1.50 update already.
inline constexpr char kWhatsNewLastVersion[] = "brave.whats_new.last_version";

}  // namespace whats_new::prefs

#endif  // BRAVE_BROWSER_UI_WHATS_NEW_PREF_NAMES_H_
