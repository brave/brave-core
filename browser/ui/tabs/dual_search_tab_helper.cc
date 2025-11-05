/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/dual_search_tab_helper.h"

#include "brave/browser/infobars/dual_search_infobar_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

DualSearchTabHelper::DualSearchTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<DualSearchTabHelper>(*web_contents) {}

DualSearchTabHelper::~DualSearchTabHelper() = default;

void DualSearchTabHelper::SetPairedTab(content::WebContents* paired_tab) {
  paired_tab_ = paired_tab;
}

void DualSearchTabHelper::ResetForNewSearch() {
  is_first_navigation_ = true;
}

void DualSearchTabHelper::SetShouldShowInfobar(PrefService* prefs,
                                                bool is_brave_search_tab) {
  should_show_infobar_ = true;
  is_brave_search_tab_ = is_brave_search_tab;
  prefs_ = prefs;
}

void DualSearchTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Only handle main frame navigations that are not same-document
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  const GURL& url = navigation_handle->GetURL();

  // Check if this is a search URL (search engine or Brave Search URL)
  bool is_search_url = false;
  if (url.SchemeIsHTTPOrHTTPS()) {
    std::string host = url.host();
    // Check for common search engines and Brave Search
    if (host.find("google.") != std::string::npos ||
        host.find("bing.") != std::string::npos ||
        host.find("duckduckgo.") != std::string::npos ||
        host.find("search.brave.com") != std::string::npos ||
        host.find("yahoo.") != std::string::npos) {
      is_search_url = true;
    }
  }

  // If this is the first navigation (initial search load)
  if (is_first_navigation_) {
    if (is_search_url) {
      // Mark that we've navigated to a search result
      is_first_navigation_ = false;

      // Show infobar if this is the first dual search
      if (should_show_infobar_ && prefs_) {
        auto* infobar_manager =
            infobars::ContentInfoBarManager::FromWebContents(&GetWebContents());
        if (infobar_manager) {
          DualSearchInfoBarDelegate::Create(infobar_manager, prefs_,
                                            &GetWebContents());
        }
        should_show_infobar_ = false;
        prefs_ = nullptr;
      }
    }
    return;
  }

  // If this is a search URL (new search), allow it
  if (is_search_url) {
    return;
  }

  // User navigated away from the search results - close the paired tab
  if (paired_tab_) {
    Browser* browser = chrome::FindBrowserWithTab(&GetWebContents());
    if (browser) {
      TabStripModel* tab_strip = browser->tab_strip_model();
      int paired_index = tab_strip->GetIndexOfWebContents(paired_tab_);
      if (paired_index != TabStripModel::kNoTab) {
        // Clear the paired tab's helper to prevent recursive closing
        auto* paired_helper = DualSearchTabHelper::FromWebContents(paired_tab_);
        if (paired_helper) {
          paired_helper->paired_tab_ = nullptr;
        }

        // Close the paired tab
        tab_strip->CloseWebContentsAt(paired_index,
                                       TabCloseTypes::CLOSE_USER_GESTURE);
      }
    }
    paired_tab_ = nullptr;
  }
}

void DualSearchTabHelper::WebContentsDestroyed() {
  // Clear the paired tab's reference to this tab
  if (paired_tab_) {
    auto* paired_helper = DualSearchTabHelper::FromWebContents(paired_tab_);
    if (paired_helper) {
      paired_helper->paired_tab_ = nullptr;
    }
    paired_tab_ = nullptr;
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(DualSearchTabHelper);
