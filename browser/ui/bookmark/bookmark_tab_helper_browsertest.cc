/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/common/pref_names.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

using BookmarkTabHelperBrowserTest = InProcessBrowserTest;

namespace {

bool IsNTP(content::WebContents* web_contents) {
  // Use the committed entry so the bookmarks bar disappears at the same time
  // the page does.
  content::NavigationEntry* entry =
      web_contents->GetController().GetLastCommittedEntry();
  if (!entry)
    entry = web_contents->GetController().GetVisibleEntry();
  return (entry && NewTabUI::IsNewTab(entry->GetURL())) ||
         search::NavEntryIsInstantNTP(web_contents, entry);
}

void AddBookmarkNode(Profile* profile) {
  const GURL url = GURL("https://www.brave.com");
  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile);

  std::vector<const bookmarks::BookmarkNode*> nodes;
  bookmark_model->GetNodesByURL(url, &nodes);
  EXPECT_EQ(0UL, nodes.size());

  bookmarks::AddIfNotBookmarked(bookmark_model, url, std::u16string());
  bookmark_model->GetNodesByURL(url, &nodes);
  EXPECT_EQ(1UL, nodes.size());
}

}  // namespace

IN_PROC_BROWSER_TEST_F(BookmarkTabHelperBrowserTest, BookmarkBarOnNTPTest) {
  auto* profile = browser()->profile();
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();

  // Check Bookmark bar is hidden by default for non NTP.
  EXPECT_FALSE(IsNTP(contents));
  EXPECT_EQ(BookmarkBar::HIDDEN, browser()->bookmark_bar_state());

  // Check show bookarks on NTP is on by default.
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(kAlwaysShowBookmarkBarOnNTP));

  // Loading NTP.
  EXPECT_TRUE(content::NavigateToURL(contents,
                                     GURL(chrome::kChromeUINewTabURL)));
  EXPECT_TRUE(IsNTP(contents));

  // Check bookmark bar on NTP is shown even if bookmark bar is empty.
  EXPECT_EQ(BookmarkBar::SHOW, browser()->bookmark_bar_state());

  AddBookmarkNode(profile);

  // Check bookmark is also visible on NTP after adding bookmark regardless of
  // show bookmark bar option value.
  chrome::ToggleBookmarkBar(browser());
  EXPECT_EQ(BookmarkBar::SHOW, browser()->bookmark_bar_state());
  chrome::ToggleBookmarkBar(browser());
  EXPECT_EQ(BookmarkBar::SHOW, browser()->bookmark_bar_state());

  // Turn off showing bookmark bar on NTP.
  profile->GetPrefs()->SetBoolean(kAlwaysShowBookmarkBarOnNTP, false);

  // Check bookmark bar on NTP is hidden.
  EXPECT_EQ(BookmarkBar::HIDDEN, browser()->bookmark_bar_state());

  // Check bookmark bar on NTP is visible when kBookmarkBar pref is on.
  chrome::ToggleBookmarkBar(browser());
  EXPECT_EQ(BookmarkBar::SHOW, browser()->bookmark_bar_state());

  // Check bookmark bar on NTP is hidden when kBookmarkBar pref is off.
  chrome::ToggleBookmarkBar(browser());
  EXPECT_EQ(BookmarkBar::HIDDEN, browser()->bookmark_bar_state());
}
