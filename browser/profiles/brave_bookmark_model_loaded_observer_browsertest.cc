/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/pref_names.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_service.h"

using BraveBookmarkModelLoadedObserverBrowserTest = InProcessBrowserTest;

// This test to mainly testing kOtherBookmarksMigrated, granular testing for
// migration is in BookmarkModelTest::BraveMigrateOtherNodeFolder

namespace {

void CreateOtherBookmarksFolder(bookmarks::BookmarkModel* model) {
  const bookmarks::BookmarkNode* other_node_folder = model->AddFolder(
      model->bookmark_bar_node(), model->bookmark_bar_node()->children().size(),
      model->other_node()->GetTitledUrlNodeTitle());
  model->AddFolder(other_node_folder, 0, base::ASCIIToUTF16("A"));
}

}  // namespace

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       PRE_OtherBookmarksMigration) {
  Profile* profile = browser()->profile();

  profile->GetPrefs()->SetBoolean(kOtherBookmarksMigrated, false);

  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile);
  CreateOtherBookmarksFolder(bookmark_model);
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       OtherBookmarksMigration) {
  Profile* profile = browser()->profile();
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(kOtherBookmarksMigrated));

  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile);

  ASSERT_EQ(bookmark_model->other_node()->children().size(), 1u);
  ASSERT_EQ(bookmark_model->bookmark_bar_node()->children().size(), 0u);
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       PRE_NoOtherBookmarksMigration) {
  Profile* profile = browser()->profile();

  profile->GetPrefs()->SetBoolean(kOtherBookmarksMigrated, true);

  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile);

  CreateOtherBookmarksFolder(bookmark_model);
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       NoOtherBookmarksMigration) {
  Profile* profile = browser()->profile();
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(kOtherBookmarksMigrated));

  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile);

  ASSERT_EQ(bookmark_model->other_node()->children().size(), 0u);
  ASSERT_EQ(bookmark_model->bookmark_bar_node()->children().size(), 1u);
}
