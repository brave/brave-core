/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/bookmarks/browser/bookmark_model.h"

using BraveBookmarkClientTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveBookmarkClientTest, IsPermanentNodeVisible) {
  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(browser()->profile());
  EXPECT_TRUE(bookmark_model->bookmark_bar_node()->IsVisible());
  // Other node invisible by default
  EXPECT_FALSE(bookmark_model->other_node()->IsVisible());
  EXPECT_FALSE(bookmark_model->mobile_node()->IsVisible());

  bookmark_model->AddURL(bookmark_model->other_node(), 0,
                         base::ASCIIToUTF16("A"), GURL("https://A.com"));
  EXPECT_TRUE(bookmark_model->other_node()->IsVisible());
  BraveMigrateOtherNode(bookmark_model);
  EXPECT_FALSE(bookmark_model->other_node()->IsVisible());
}
