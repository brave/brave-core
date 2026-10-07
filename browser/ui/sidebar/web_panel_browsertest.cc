/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstddef>

#include "base/check.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/sidebar/sidebar_browsertest_base.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/sidebar/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/view_utils.h"
#include "url/gurl.h"

namespace sidebar {

class WebPanelBrowserTest : public SidebarBrowserTest {
 public:
  WebPanelBrowserTest() {
    scoped_features_.InitAndEnableFeature(features::kSidebarWebPanel);
  }
  ~WebPanelBrowserTest() override = default;

  SidebarWebPanelController* web_panel_controller() {
    return controller()->GetWebPanelController();
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

// Regression test for a crash caused by BraveMultiContentsView's web-panel
// container never being registered in
// MultiContentsView::container_focusable_map_, which left
// BrowserView::MaybeUpdateStoredFocusForWebContents() dereferencing a null
// FocusableViewMap*.
IN_PROC_BROWSER_TEST_F(WebPanelBrowserTest,
                       ReactivatingWebPanelTabAfterFocusLossDoesNotCrash) {
  // Avoid the "item added" feedback bubble interfering with the test.
  browser()->GetProfile()->GetPrefs()->SetInteger(
      kSidebarItemAddedFeedbackBubbleShowCount, 3);

  // Add the current tab as a web panel sidebar item.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  ASSERT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  controller()->AddItemWithCurrentTab();
  const size_t web_panel_item_index = model()->GetAllSidebarItems().size() - 1;
  ASSERT_TRUE(
      model()->GetAllSidebarItems()[web_panel_item_index].is_web_panel_type());

  // Give ourselves another regular tab to switch to later.
  chrome::AddTabAt(browser(), GURL("about:blank"), -1,
                   /*foreground*/ true);

  auto* tab_strip_model = browser()->tab_strip_model();

  // Open the web panel. This creates a pinned tab for it at index 0, without
  // making it the active/foreground tab.
  controller()->OnItemPressed(web_panel_item_index);
  ASSERT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  ASSERT_EQ(3, tab_strip_model->count());
  ASSERT_EQ(tab_strip_model->GetTabAtIndex(0)->GetContents(),
            web_panel_controller()->panel_contents());

  // First activation of the web panel's pinned tab: this alone doesn't
  // trigger the crash because the panel's WebContents has never held focus
  // yet (matches the existing WebPanelTest coverage).
  tab_strip_model->ActivateTabAt(0);
  ASSERT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());

  // Force real UI focus onto the panel's contents view.
  GetBraveMultiContentsView()->GetActiveContentsView()->RequestFocus();
  ASSERT_TRUE(GetBraveMultiContentsView()->GetActiveContentsView()->HasFocus());

  // Switch to another tab. This stores the focused view against the panel's
  // WebContents via ChromeWebContentsViewFocusHelper::StoreFocus().
  tab_strip_model->ActivateTabAt(1);

  // Re-activate the web panel's pinned tab a second time. Pre-fix, this
  // dereferenced a null FocusableViewMap* and crashed the browser process.
  tab_strip_model->ActivateTabAt(0);

  // If we get here, the crash didn't happen. Sanity-check the panel still
  // works correctly afterwards.
  EXPECT_EQ(0, tab_strip_model->active_index());
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(
      web_panel_controller()->panel_contents(),
      GetBraveMultiContentsView()->GetActiveContentsView()->GetWebContents());
}

// A web panel item is the active sidebar item while its panel is open, and
// active state is mutually exclusive with the side panel's.
// See https://github.com/brave/brave-browser/issues/33533.
IN_PROC_BROWSER_TEST_F(WebPanelBrowserTest, WebPanelItemActiveStateTest) {
  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  auto items_contents_view = GetSidebarItemsContentsView(controller());
  auto* sidebar = GetSidebarContainerView();
  auto* panel_ui = SidePanelUI::From(browser());
  GetSidePanel()->DisableAnimationsForTesting();

  // To prevent item added bubble launching.
  browser()->GetProfile()->GetPrefs()->SetInteger(
      kSidebarItemAddedFeedbackBubbleShowCount, 3);

  auto ink_drop_state_at = [&](size_t index) {
    auto* item_view = views::AsViewClass<SidebarItemView>(
        items_contents_view->children()[index].get());
    CHECK(item_view);
    return views::InkDrop::Get(item_view)
        ->GetInkDrop()
        ->GetTargetInkDropState();
  };

  // Add two web panel type items.
  const GURL url_a("https://brave.com/");
  const GURL url_b("https://basicattentiontoken.com/");
  for (const auto& url : {url_a, url_b}) {
    service->AddItem(SidebarItem::Create(url, u"title",
                                         SidebarItem::Type::kTypeWeb,
                                         SidebarItem::BuiltInItemType::kNone,
                                         /*open_in_panel*/ true));
  }
  const size_t index_b = model()->GetAllSidebarItems().size() - 1;
  const size_t index_a = index_b - 1;
  RunScheduledLayouts();

  ASSERT_TRUE(sidebar->IsSidebarVisible());

  // Activating a web panel item highlights it.
  SimulateSidebarItemClickAt(index_a);
  EXPECT_EQ(model()->active_index(), index_a);
  EXPECT_EQ(views::InkDropState::ACTIVATED, ink_drop_state_at(index_a));
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());

  // Switching panels moves the highlight.
  SimulateSidebarItemClickAt(index_b);
  EXPECT_EQ(model()->active_index(), index_b);
  EXPECT_EQ(views::InkDropState::HIDDEN, ink_drop_state_at(index_a));
  EXPECT_EQ(views::InkDropState::ACTIVATED, ink_drop_state_at(index_b));

  // Clicking the active web panel item toggles it off.
  SimulateSidebarItemClickAt(index_b);
  EXPECT_FALSE(model()->active_index());
  EXPECT_FALSE(web_panel_controller()->panel_contents());
  EXPECT_EQ(views::InkDropState::HIDDEN, ink_drop_state_at(index_b));

  // Closing the panel's pinned tab clears the highlight.
  SimulateSidebarItemClickAt(index_b);
  ASSERT_EQ(views::InkDropState::ACTIVATED, ink_drop_state_at(index_b));
  tab_model()->GetTabAtIndex(0)->Close();
  EXPECT_FALSE(model()->active_index());
  EXPECT_EQ(views::InkDropState::HIDDEN, ink_drop_state_at(index_b));

  // Showing a side panel closes the web panel and takes over the active state.
  SimulateSidebarItemClickAt(index_b);
  ASSERT_EQ(model()->active_index(), index_b);
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return panel_ui->IsSidePanelShowing(); }));
  EXPECT_FALSE(web_panel_controller()->panel_contents());
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(views::InkDropState::HIDDEN, ink_drop_state_at(index_b));
  const auto side_panel_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(side_panel_item_index);
  EXPECT_EQ(model()->active_index(), side_panel_item_index);

  // And the other way around.
  SimulateSidebarItemClickAt(index_b);
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !panel_ui->IsSidePanelShowing(); }));
  EXPECT_EQ(model()->active_index(), index_b);
  EXPECT_EQ(views::InkDropState::ACTIVATED, ink_drop_state_at(index_b));
  EXPECT_EQ(views::InkDropState::HIDDEN,
            ink_drop_state_at(*side_panel_item_index));

  // The highlight survives hiding and re-showing the sidebar.
  service->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowNever);
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !sidebar->IsSidebarVisible(); }));
  EXPECT_EQ(views::InkDropState::HIDDEN, ink_drop_state_at(index_b));
  controller()->ToggleSidebarPinning();
  ASSERT_TRUE(sidebar->IsSidebarVisible());
  EXPECT_EQ(views::InkDropState::ACTIVATED, ink_drop_state_at(index_b));
}

}  // namespace sidebar
