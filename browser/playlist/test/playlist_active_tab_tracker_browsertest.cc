/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_active_tab_tracker.h"

#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/common/mojom/playlist.mojom-forward.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"

class PlaylistActiveTabTrackerBrowserTest : public InProcessBrowserTest {
 public:
  PlaylistActiveTabTrackerBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~PlaylistActiveTabTrackerBrowserTest() override = default;

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PlaylistActiveTabTrackerBrowserTest,
                       ShouldShowAddMediaFromPageUI) {
  content::WebContents* active_web_contents = GetActiveWebContents();
  ASSERT_TRUE(active_web_contents);

  std::unique_ptr<playlist::PlaylistActiveTabTracker> tracker =
      std::make_unique<playlist::PlaylistActiveTabTracker>(active_web_contents,
                                                           base::DoNothing());
  ASSERT_FALSE(tracker->ShouldShowAddMediaFromPageUI());

  // When playlist tab helper has found media
  auto* tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(active_web_contents);
  ASSERT_TRUE(tab_helper);

  std::vector<playlist::mojom::PlaylistItemPtr> items;
  items.push_back(playlist::mojom::PlaylistItem::New());
  tab_helper->OnMediaFilesUpdated(active_web_contents->GetLastCommittedURL(),
                                  std::move(items));

  // We should show "add media from page" UI
  EXPECT_TRUE(tracker->ShouldShowAddMediaFromPageUI());

  // When playlist tab helper has saved media
  auto saved_item = playlist::mojom::PlaylistItem::New();
  saved_item->page_source = active_web_contents->GetLastCommittedURL();
  tab_helper->OnItemCreated(std::move(saved_item));

  // We shouldn't show "add media from page" UI
  EXPECT_FALSE(tracker->ShouldShowAddMediaFromPageUI());
}

IN_PROC_BROWSER_TEST_F(PlaylistActiveTabTrackerBrowserTest,
                       Callback_OnActiveTabChanged) {
  // When active tab changes, callback should be invoked
  {
    base::MockCallback<playlist::PlaylistActiveTabTracker::Callback> callback;
    EXPECT_CALL(callback, Run(false)).Times(testing::AtLeast(1));

    std::unique_ptr<playlist::PlaylistActiveTabTracker> tracker =
        std::make_unique<playlist::PlaylistActiveTabTracker>(
            GetActiveWebContents(), callback.Get());
    chrome::AddTabAt(browser(), GURL(), /*index*/ -1, /*foreground*/ true);
  }

  // When active tab changes and the new active tab has found media, callback
  // should be invoked with |true| argument.
  {
    auto* first_contents = browser()->tab_strip_model()->GetWebContentsAt(0);
    auto* tab_helper =
        playlist::PlaylistTabHelper::FromWebContents(first_contents);
    ASSERT_TRUE(tab_helper);
    std::vector<playlist::mojom::PlaylistItemPtr> found_items;
    found_items.push_back(playlist::mojom::PlaylistItem::New());
    tab_helper->OnMediaFilesUpdated(first_contents->GetLastCommittedURL(),
                                    std::move(found_items));

    base::MockCallback<playlist::PlaylistActiveTabTracker::Callback> callback;
    EXPECT_CALL(callback, Run(false)).Times(1);  // when active tab changed
    EXPECT_CALL(callback, Run(true))
        .Times(1);  // playlist tab helper is updated

    std::unique_ptr<playlist::PlaylistActiveTabTracker> tracker =
        std::make_unique<playlist::PlaylistActiveTabTracker>(
            GetActiveWebContents(), callback.Get());

    browser()->tab_strip_model()->ActivateTabAt(0);

    EXPECT_TRUE(tracker->ShouldShowAddMediaFromPageUI());
  }
}

IN_PROC_BROWSER_TEST_F(PlaylistActiveTabTrackerBrowserTest,
                       Callback_PlaylistTabHelperIsUpdated) {
  // When PlaylistTabHelper is updated, callback should be invoked
  base::MockCallback<playlist::PlaylistActiveTabTracker::Callback> callback;
  EXPECT_CALL(callback, Run(false)).Times(1);  // from constructor
  EXPECT_CALL(callback, Run(true)).Times(1);   // when tab helper is updated

  std::unique_ptr<playlist::PlaylistActiveTabTracker> tracker =
      std::make_unique<playlist::PlaylistActiveTabTracker>(
          GetActiveWebContents(), callback.Get());
  auto* tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(GetActiveWebContents());

  ASSERT_TRUE(tab_helper);
  std::vector<playlist::mojom::PlaylistItemPtr> found_items;
  found_items.push_back(playlist::mojom::PlaylistItem::New());
  tab_helper->OnMediaFilesUpdated(GetActiveWebContents()->GetLastCommittedURL(),
                                  std::move(found_items));
  EXPECT_TRUE(tracker->ShouldShowAddMediaFromPageUI());
}
