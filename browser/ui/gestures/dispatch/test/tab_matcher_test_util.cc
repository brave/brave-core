// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/test/tab_matcher_test_util.h"

#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"

namespace gestures::test {

testing::Matcher<const MatchedTabInfo&> MatchesTab(
    content::WebContents* web_contents) {
  return testing::ResultOf(
      [](const MatchedTabInfo& matched_tab) {
        return matched_tab.browser_window->GetTabStripModel()->GetWebContentsAt(
            matched_tab.index);
      },
      web_contents);
}

}  // namespace gestures::test
