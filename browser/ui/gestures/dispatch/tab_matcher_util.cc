// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/tab_matcher_util.h"

#include "base/memory/raw_ref.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/gestures/core/common/match_pattern_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_collection.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace gestures {

MatchedTabs FindMatchingTabs(Profile& profile, std::string_view pattern) {
  MatchedTabs matched_tabs;
  ProfileBrowserCollection* const browser_collection =
      ProfileBrowserCollection::GetForProfile(&profile);
  if (!browser_collection) {
    return matched_tabs;
  }
  browser_collection->ForEach(
      [&](BrowserWindowInterface* browser_window_ptr) {
        BrowserWindowInterface& browser_window = *browser_window_ptr;
        TabStripModel* const tab_strip_model =
            browser_window.GetTabStripModel();
        if (!tab_strip_model) {
          return true;
        }
        for (int i = 0; i < tab_strip_model->count(); ++i) {
          content::WebContents* const web_contents =
              tab_strip_model->GetWebContentsAt(i);
          if (web_contents &&
              (MatchesPattern(web_contents->GetLastCommittedURL().spec(),
                              pattern) ||
               MatchesPattern(base::UTF16ToUTF8(web_contents->GetTitle()),
                              pattern))) {
            matched_tabs.push_back(MatchedTabInfo{raw_ref(browser_window), i});
          }
        }
        return true;
      },
      BrowserCollection::Order::kCreation);
  return matched_tabs;
}

}  // namespace gestures
