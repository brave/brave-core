// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_MATCHER_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_MATCHER_UTIL_H_

#include <string>

#include "brave/browser/ui/gestures/dispatch/matched_tab_info.h"

class Profile;

namespace gestures {

// Every open tab belonging to `profile`, whose URL or title matches
// `pattern`, in `kCreation` order (rather than the activation order some
// iterators default to), since cycling through repeated matches needs a
// stable order that doesn't shift as windows are focused. Scoped to
// `profile` so a gesture never matches, surfaces, or leaks the existence of
// a tab in a different profile or an unrelated Incognito/Guest window.
MatchedTabs FindMatchingTabs(Profile& profile, const std::string& pattern);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_MATCHER_UTIL_H_
