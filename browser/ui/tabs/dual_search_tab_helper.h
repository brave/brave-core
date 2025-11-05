/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_DUAL_SEARCH_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_TABS_DUAL_SEARCH_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PrefService;

namespace content {
class WebContents;
}

// Tracks tabs that are part of a dual search split view and automatically
// closes the paired tab when the user navigates away from the search results.
class DualSearchTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<DualSearchTabHelper> {
 public:
  ~DualSearchTabHelper() override;

  DualSearchTabHelper(const DualSearchTabHelper&) = delete;
  DualSearchTabHelper& operator=(const DualSearchTabHelper&) = delete;

  // Mark this tab as part of a dual search split with the given paired tab.
  void SetPairedTab(content::WebContents* paired_tab);

  // Get the paired tab.
  content::WebContents* paired_tab() const { return paired_tab_; }

  // Reset the helper for a new search (allows another navigation before closing).
  void ResetForNewSearch();

  // Mark this as a new dual search that should show the infobar.
  // is_brave_search_tab: true if this is the Brave Search tab (right side)
  void SetShouldShowInfobar(PrefService* prefs, bool is_brave_search_tab);

  // WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

 private:
  explicit DualSearchTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<DualSearchTabHelper>;

  // The paired tab in the dual search split view.
  raw_ptr<content::WebContents> paired_tab_ = nullptr;

  // Track if this is the first navigation (the initial search).
  bool is_first_navigation_ = true;

  // Track if we should show the infobar on first load.
  bool should_show_infobar_ = false;
  bool is_brave_search_tab_ = false;
  raw_ptr<PrefService> prefs_ = nullptr;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_TABS_DUAL_SEARCH_TAB_HELPER_H_
