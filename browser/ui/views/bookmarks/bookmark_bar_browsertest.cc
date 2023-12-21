/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_view_ids.h"
#include "brave/browser/ui/views/bookmarks/bookmark_bar_instructions_view.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

class BookmarkBarTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    browser()->profile()->GetPrefs()->SetBoolean(
        bookmarks::prefs::kShowBookmarkBar, true);
  }

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  BookmarkBarView* bookmark_bar() { return browser_view()->bookmark_bar(); }

  BookmarkBarInstructionsView* GetInstructionView() {
    for (views::View* child : bookmark_bar()->children()) {
      if (child->GetID() == BRAVE_VIEW_ID_BOOKMARK_IMPORT_INSTRUCTION_VIEW)
        return static_cast<BookmarkBarInstructionsView*>(child);
    }
    return nullptr;
  }
};

IN_PROC_BROWSER_TEST_F(BookmarkBarTest, InstructionsViewTest) {
  EXPECT_TRUE(GetInstructionView()->GetVisible());

  bookmarks::BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(browser()->profile());
  bookmarks::AddIfNotBookmarked(model, GURL("http://example.com/"), u"bookmark",
                                model->bookmark_bar_node());

  EXPECT_FALSE(GetInstructionView()->GetVisible());
}
