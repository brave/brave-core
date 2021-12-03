/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

// This test to mainly testing whether migration only runs once,
// granular testing for is in BookmarkModelTest

namespace {

void CreateOtherBookmarksFolder(bookmarks::BookmarkModel* model) {
  const bookmarks::BookmarkNode* other_node_folder = model->AddFolder(
      model->bookmark_bar_node(), model->bookmark_bar_node()->children().size(),
      model->other_node()->GetTitledUrlNodeTitle());
  model->AddFolder(other_node_folder, 0, u"A");
}

void CreateBraveSyncV1MetaInfo(bookmarks::BookmarkModel* model) {
  const bookmarks::BookmarkNode* node = model->AddURL(
      model->bookmark_bar_node(), model->bookmark_bar_node()->children().size(),
      u"Brave", GURL("https://brave.com"));
  model->SetNodeMetaInfo(node, "object_id", "object_id_value");
}

}  // namespace

class BraveBookmarkModelLoadedObserverBrowserTest
    : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    profile_ = browser()->profile();
    bookmark_model_ = BookmarkModelFactory::GetForBrowserContext(profile_);
  }

  PrefService* prefs() { return profile_->GetPrefs(); }

  bookmarks::BookmarkModel* bookmark_model() { return bookmark_model_; }

 private:
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<bookmarks::BookmarkModel> bookmark_model_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       PRE_OtherBookmarksMigration) {
  prefs()->SetBoolean(kOtherBookmarksMigrated, false);

  CreateOtherBookmarksFolder(bookmark_model());
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       OtherBookmarksMigration) {
  EXPECT_TRUE(prefs()->GetBoolean(kOtherBookmarksMigrated));

  ASSERT_EQ(bookmark_model()->other_node()->children().size(), 1u);
  ASSERT_EQ(bookmark_model()->bookmark_bar_node()->children().size(), 0u);
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       PRE_NoOtherBookmarksMigration) {
  prefs()->SetBoolean(kOtherBookmarksMigrated, true);

  CreateOtherBookmarksFolder(bookmark_model());
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       NoOtherBookmarksMigration) {
  EXPECT_TRUE(prefs()->GetBoolean(kOtherBookmarksMigrated));

  ASSERT_EQ(bookmark_model()->other_node()->children().size(), 0u);
  ASSERT_EQ(bookmark_model()->bookmark_bar_node()->children().size(), 1u);
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       PRE_ClearSyncV1MetaInfo) {
  brave_sync::Prefs brave_sync_prefs(prefs());
  brave_sync_prefs.SetSyncV1MetaInfoCleared(false);

  CreateBraveSyncV1MetaInfo(bookmark_model());
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       ClearSyncV1MetaInfo) {
  brave_sync::Prefs brave_sync_prefs(prefs());
  EXPECT_TRUE(brave_sync_prefs.IsSyncV1MetaInfoCleared());

  const bookmarks::BookmarkNode* node =
      bookmark_model()->bookmark_bar_node()->children()[0].get();
  std::string object_id;
  ASSERT_EQ(node->GetMetaInfoMap(), nullptr);
  ASSERT_FALSE(node->GetMetaInfo("object_id", &object_id));
  ASSERT_TRUE(object_id.empty());
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       PRE_NoClearSyncV1MetaInfo) {
  brave_sync::Prefs brave_sync_prefs(prefs());
  brave_sync_prefs.SetSyncV1MetaInfoCleared(true);

  CreateBraveSyncV1MetaInfo(bookmark_model());
}

IN_PROC_BROWSER_TEST_F(BraveBookmarkModelLoadedObserverBrowserTest,
                       NoClearSyncV1MetaInfo) {
  brave_sync::Prefs brave_sync_prefs(prefs());
  EXPECT_TRUE(brave_sync_prefs.IsSyncV1MetaInfoCleared());

  const bookmarks::BookmarkNode* node =
      bookmark_model()->bookmark_bar_node()->children()[0].get();
  std::string object_id;
  ASSERT_NE(node->GetMetaInfoMap(), nullptr);
  ASSERT_TRUE(node->GetMetaInfo("object_id", &object_id));
  ASSERT_EQ(object_id, "object_id_value");
}
