// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_TEST_TAB_MATCHER_TEST_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_TEST_TAB_MATCHER_TEST_UTIL_H_

#include "brave/browser/ui/gestures/dispatch/matched_tab_info.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {
class WebContents;
}  // namespace content

namespace gestures::test {

// Matches a `MatchedTabInfo` whose window/index resolve to `web_contents`.
testing::Matcher<const MatchedTabInfo&> MatchesTab(
    content::WebContents* web_contents);

}  // namespace gestures::test

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_TEST_TAB_MATCHER_TEST_UTIL_H_
