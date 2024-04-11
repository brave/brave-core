/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TPCD_HEURISTICS_REDIRECT_HEURISTIC_TAB_HELPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TPCD_HEURISTICS_REDIRECT_HEURISTIC_TAB_HELPER_H_

#define RedirectHeuristicTabHelper RedirectHeuristicTabHelper_ChromiumImpl
#define MaybeRecordRedirectHeuristic virtual MaybeRecordRedirectHeuristic

#include "src/chrome/browser/tpcd/heuristics/redirect_heuristic_tab_helper.h"  // IWYU pragma: export
#undef MaybeRecordRedirectHeuristic
#undef RedirectHeuristicTabHelper

// Disable RedirectHeuristicTabHelper functionality since we disable kDIPS, so
// the upstream code would not work (and crash because it doesn't check that
// dips_service_ can be a null).
class RedirectHeuristicTabHelper
    : public content::WebContentsUserData<RedirectHeuristicTabHelper> {
 public:
  static std::set<std::string> AllSitesFollowingFirstParty(
      content::WebContents* web_contents,
      const GURL& first_party_url) {
    return {};
  }

 private:
  explicit RedirectHeuristicTabHelper(content::WebContents* web_contents);
  // So WebContentsUserData::CreateForWebContents() can call the constructor.
  friend class content::WebContentsUserData<RedirectHeuristicTabHelper>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TPCD_HEURISTICS_REDIRECT_HEURISTIC_TAB_HELPER_H_
