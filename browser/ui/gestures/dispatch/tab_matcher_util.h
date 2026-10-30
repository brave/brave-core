// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_MATCHER_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_MATCHER_UTIL_H_

#include <string_view>

#include "brave/browser/ui/gestures/dispatch/matched_tab_info.h"

class Profile;

namespace gestures {

// Every open tab in `profile` whose URL/title matches `pattern`, in
// `kCreation` order; never crosses into another profile or Incognito/Guest.
MatchedTabs FindMatchingTabs(Profile& profile, std::string_view pattern);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_TAB_MATCHER_UTIL_H_
