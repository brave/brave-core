/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

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
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
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

}  // namespace

IN_PROC_BROWSER_TEST_F(BookmarkTabHelperBrowserTest,
                       BookmarkBarOnNTPToggleTest) {
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::NavigateToURL(contents,
                                     GURL(chrome::kChromeUINewTabURL)));
  EXPECT_TRUE(IsNTP(contents));
  chrome::ToggleBookmarkBar(browser());
  EXPECT_EQ(BookmarkBar::SHOW, browser()->bookmark_bar_state());

  const GURL url = GURL("https://www.brave.com");
  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(browser()->profile());

  std::vector<const bookmarks::BookmarkNode*> nodes;
  bookmark_model->GetNodesByURL(url, &nodes);
  EXPECT_EQ(0UL, nodes.size());

  bookmarks::AddIfNotBookmarked(bookmark_model, url, base::string16());
  bookmark_model->GetNodesByURL(url, &nodes);
  EXPECT_EQ(1UL, nodes.size());

  chrome::ToggleBookmarkBar(browser());
  EXPECT_EQ(BookmarkBar::HIDDEN, browser()->bookmark_bar_state());
}
