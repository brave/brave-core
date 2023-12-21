/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/components/constants/pref_names.h"
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

  std::vector<raw_ptr<const bookmarks::BookmarkNode, VectorExperimental>>
      nodes = bookmark_model->GetNodesByURL(url);
  EXPECT_EQ(0UL, nodes.size());

  // We need to pass a non-empty title when creating a bookmark so that an
  // accessible name is also available, otherwise we'll hit a CHECK() and
  // the test will crash (see accessibility_paint_checks.cc).
  bookmarks::AddIfNotBookmarked(bookmark_model, url, u"brave");
  nodes = bookmark_model->GetNodesByURL(url);
  EXPECT_EQ(1UL, nodes.size());
}

}  // namespace

IN_PROC_BROWSER_TEST_F(BookmarkTabHelperBrowserTest, BookmarkBarOnNTPTest) {
  auto* profile = browser()->profile();
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();

  // Check Bookmark bar is hidden by default for non NTP.
  EXPECT_FALSE(IsNTP(contents));
  EXPECT_EQ(BookmarkBar::HIDDEN, browser()->bookmark_bar_state());

  // Check show bookmarks on NTP is on by default.
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
  EXPECT_EQ(BookmarkBar::HIDDEN, browser()->bookmark_bar_state());
  EXPECT_FALSE(profile->GetPrefs()->GetBoolean(kAlwaysShowBookmarkBarOnNTP));
  // Turn off showing bookmark bar on NTP.
  profile->GetPrefs()->SetBoolean(kAlwaysShowBookmarkBarOnNTP, true);

  // Check bookmark bar on NTP is visible when kAlwaysShowBookmarkBarOnNTP pref
  // is on.
  EXPECT_EQ(BookmarkBar::SHOW, browser()->bookmark_bar_state());

  // Check bookmark bar on NTP is visible when kBookmarkBar pref is on.
  chrome::ToggleBookmarkBar(browser());
  EXPECT_EQ(BookmarkBar::SHOW, browser()->bookmark_bar_state());
}

IN_PROC_BROWSER_TEST_F(BookmarkTabHelperBrowserTest,
                       BookmarkBarOnNTPTestIncognito) {
  Browser* incognito = CreateIncognitoBrowser();
  auto* profile = incognito->profile();
  auto* contents = incognito->tab_strip_model()->GetActiveWebContents();

  // Check Bookmark bar is hidden by default for non NTP.
  EXPECT_FALSE(IsNTP(contents));
  EXPECT_EQ(BookmarkBar::HIDDEN, incognito->bookmark_bar_state());

  // Check show bookmarks on NTP is on by default.
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(kAlwaysShowBookmarkBarOnNTP));

  // Loading NTP.
  EXPECT_TRUE(
      content::NavigateToURL(contents, GURL(chrome::kChromeUINewTabURL)));
  EXPECT_TRUE(IsNTP(contents));

  // Check bookmark bar on NTP is shown even if bookmark bar is empty.
  EXPECT_EQ(BookmarkBar::SHOW, incognito->bookmark_bar_state());

  AddBookmarkNode(profile);

  // Check bookmark is also visible on NTP after adding bookmark regardless of
  // show bookmark bar option value.
  chrome::ToggleBookmarkBar(incognito);
  EXPECT_EQ(BookmarkBar::SHOW, incognito->bookmark_bar_state());
  chrome::ToggleBookmarkBar(incognito);
  EXPECT_EQ(BookmarkBar::HIDDEN, incognito->bookmark_bar_state());
  EXPECT_FALSE(profile->GetPrefs()->GetBoolean(kAlwaysShowBookmarkBarOnNTP));

  // Turn on showing bookmark bar on NTP.
  profile->GetPrefs()->SetBoolean(kAlwaysShowBookmarkBarOnNTP, true);

  // Check bookmark bar on NTP is visible when kAlwaysShowBookmarkBarOnNTP pref
  // is on.
  EXPECT_EQ(BookmarkBar::SHOW, incognito->bookmark_bar_state());

  // Check bookmark bar on NTP is visible when kBookmarkBar pref is on.
  chrome::ToggleBookmarkBar(incognito);
  EXPECT_EQ(BookmarkBar::SHOW, incognito->bookmark_bar_state());
}
