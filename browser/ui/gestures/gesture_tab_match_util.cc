// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_tab_match_util.h"

#include "base/strings/pattern.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_collection.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace gestures {

namespace {

// A plain glob (see `base::MatchPattern`) only ever uses `*`/`?` as special
// characters; anything else here almost certainly means the pattern was
// written as a regex instead.
bool LooksLikeRegex(const std::string& pattern) {
  return pattern.find_first_of("^$()[]{}|+\\") != std::string::npos;
}

bool MatchesPattern(const std::string& text, const std::string& pattern) {
  if (!LooksLikeRegex(pattern)) {
    return base::MatchPattern(text, pattern);
  }
  const re2::RE2 regex(pattern);
  return regex.ok() && re2::RE2::PartialMatch(text, regex);
}

}  // namespace

std::vector<TabMatchInfo> FindMatchingTabs(Profile& profile,
                                           const std::string& pattern) {
  std::vector<TabMatchInfo> tab_matches;
  ProfileBrowserCollection* const browser_collection =
      ProfileBrowserCollection::GetForProfile(&profile);
  if (!browser_collection) {
    return tab_matches;
  }
  browser_collection->ForEach(
      [&](BrowserWindowInterface* browser_window) {
        TabStripModel* const tab_strip_model =
            browser_window->GetTabStripModel();
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
            tab_matches.push_back(TabMatchInfo{browser_window, i});
          }
        }
        return true;
      },
      BrowserCollection::Order::kCreation);
  return tab_matches;
}

}  // namespace gestures
