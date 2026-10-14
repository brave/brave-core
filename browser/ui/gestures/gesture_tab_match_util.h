// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_TAB_MATCH_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_TAB_MATCH_UTIL_H_

#include <string>
#include <vector>

#include "brave/browser/ui/gestures/gesture_tab_match_info.h"

class Profile;

namespace gestures {

// Every open tab belonging to `profile`, whose URL or title matches
// `pattern`, in `kCreation` order (rather than the activation order some
// iterators default to), since cycling through repeated matches needs a
// stable order that doesn't shift as windows are focused. Scoped to
// `profile` so a gesture never matches, surfaces, or leaks the existence of
// a tab in a different profile or an unrelated Incognito/Guest window.
std::vector<TabMatchInfo> FindMatchingTabs(Profile& profile,
                                           const std::string& pattern);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_TAB_MATCH_UTIL_H_
