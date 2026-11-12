/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cstddef>
#include <optional>

#include "base/check.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
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
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/sidebar/common/features.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/sessions/core/tab_restore_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/view_utils.h"
#include "url/gurl.h"

namespace sidebar {

namespace {

// Shared by the PRE_/main restore pairs, which run in separate processes.
constexpr char kTestWebPanelURL[] = "https://brave.com/";
// A WebUI URL commits reliably without a test server and is distinguishable
// from the tabs a launch creates on its own.
constexpr char kRegularTabURL[] = "chrome://version/";
// A tab the user pinned themselves, as opposed to the web panel's pinned tab.
// Like kRegularTabURL this has to commit without redirecting, since the pinned
// tab pref records the last committed URL.
constexpr char kUserPinnedTabURL[] = "chrome://gpu/";

}  // namespace

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

class WebPanelDisabledBrowserTest : public SidebarBrowserTest {
 public:
  WebPanelDisabledBrowserTest() {
    scoped_features_.InitAndDisableFeature(features::kSidebarWebPanel);
  }
  ~WebPanelDisabledBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_features_;
};

// Setting the startup preference to LAST makes the relaunch in the non-PRE_
// test restore the previous session.
class WebPanelSessionRestoreBrowserTest : public WebPanelBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    WebPanelBrowserTest::SetUpOnMainThread();
    SessionStartupPref::SetStartupPref(
        browser()->GetProfile(), SessionStartupPref(SessionStartupPref::LAST));
  }
};

#if BUILDFLAG(ENABLE_BRAVE_TALK)
// Brave Talk is the only built-in item declared as a web panel.
IN_PROC_BROWSER_TEST_F(WebPanelBrowserTest, BraveTalkOpensWebPanel) {
  const auto talk_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBraveTalk);
  ASSERT_TRUE(talk_index);
  const auto talk_item = model()->GetAllSidebarItems()[*talk_index];
  EXPECT_EQ(GURL(kBraveTalkURL), talk_item.url);
  EXPECT_EQ(SidebarItem::PanelType::kWebPanel, talk_item.panel_type);
  EXPECT_TRUE(talk_item.is_web_panel_type());
  EXPECT_TRUE(talk_item.opens_in_panel());
  // A web panel item has no side panel entry to resolve.
  EXPECT_FALSE(talk_item.is_side_panel_type());

  auto* tab_strip_model = browser()->tab_strip_model();
  ASSERT_EQ(1, tab_strip_model->count());
  const GURL initial_url =
      tab_strip_model->GetActiveWebContents()->GetVisibleURL();

  // Pressing opens Talk in the web panel's own pinned tab and leaves the
  // active tab alone.
  controller()->OnItemPressed(*talk_index);
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  ASSERT_EQ(2, tab_strip_model->count());
  auto* panel_contents = tab_strip_model->GetTabAtIndex(0)->GetContents();
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(0)->IsPinned());
  EXPECT_EQ(panel_contents, web_panel_controller()->panel_contents());
  EXPECT_EQ(GURL(kBraveTalkURL), panel_contents->GetVisibleURL());
  EXPECT_EQ(initial_url,
            tab_strip_model->GetActiveWebContents()->GetVisibleURL());
  EXPECT_EQ(talk_index, model()->active_index());

  // Pressing the active item toggles the panel off.
  controller()->OnItemPressed(*talk_index);
  EXPECT_FALSE(web_panel_controller()->panel_contents());
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(1, tab_strip_model->count());
  EXPECT_FALSE(model()->active_index());
}

// Without the feature, Talk keeps its declared type but behaves like a
// shortcut item.
IN_PROC_BROWSER_TEST_F(WebPanelDisabledBrowserTest, BraveTalkOpensInTab) {
  const auto talk_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBraveTalk);
  ASSERT_TRUE(talk_index);
  const auto talk_item = model()->GetAllSidebarItems()[*talk_index];
  EXPECT_EQ(SidebarItem::PanelType::kWebPanel, talk_item.panel_type);
  EXPECT_FALSE(talk_item.is_web_panel_type());
  EXPECT_FALSE(talk_item.is_side_panel_type());
  EXPECT_FALSE(talk_item.opens_in_panel());

  // There is no web panel to open into.
  EXPECT_FALSE(
      GetBraveMultiContentsView()->GetWebPanelContentsViewForTesting());

  auto* tab_strip_model = browser()->tab_strip_model();
  ASSERT_EQ(1, tab_strip_model->count());
  ASSERT_NE(GURL(kBraveTalkURL),
            tab_strip_model->GetActiveWebContents()->GetVisibleURL());

  controller()->OnItemPressed(*talk_index);
  EXPECT_EQ(GURL(kBraveTalkURL),
            tab_strip_model->GetActiveWebContents()->GetVisibleURL());
  EXPECT_EQ(1, tab_strip_model->count());
  EXPECT_FALSE(model()->active_index());
}

// Hiding Talk puts a web panel item into the hidden built-in list, which
// AddItemForSidePanelIdIfNeeded() walks on every side panel show. It must not
// be mistaken for a side panel item there, which would terminate in
// SidePanelIdFromSideBarItemType().
IN_PROC_BROWSER_TEST_F(WebPanelBrowserTest, SidePanelAfterHidingBraveTalk) {
  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  const auto talk_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBraveTalk);
  ASSERT_TRUE(talk_index);
  ASSERT_TRUE(model()->GetAllSidebarItems()[*talk_index].is_web_panel_type());

  service->RemoveItemAt(*talk_index);
  ASSERT_FALSE(model()->GetIndexOf(SidebarItem::BuiltInItemType::kBraveTalk));
  ASSERT_FALSE(service->GetHiddenDefaultSidebarItems().empty());

  const auto bookmarks_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmarks_index);
  SidePanelUI::From(browser())->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return GetSidePanel()->GetVisible(); }));
  EXPECT_EQ(bookmarks_index, model()->active_index());
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_TALK)

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
    service->AddItem(SidebarItem::CreateWebPanelItem(url, u"title"));
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

// The side panel must not animate when it swaps with the web panel, as the two
// share the same space and should exchange in a single frame.
//
// Both directions are asserted through SidePanel::state(), which outlives the
// call that starts the animation: a suppressed transition lands on kOpen /
// kClosed, an animated one on kOpening / kClosing.
IN_PROC_BROWSER_TEST_F(WebPanelBrowserTest,
                       SidePanelDoesNotAnimateWhenSwappingWithWebPanel) {
  // Suppression is only observable if the side panel would otherwise animate.
  // SidebarBrowserTest force-disables rich animations, and
  // SetRichAnimationRenderMode() silently does nothing while another force is
  // in effect, so the base scoper has to be dropped first.
  animation_resetter_.reset();
  auto scoped_mode = gfx::AnimationTestApi::SetRichAnimationRenderMode(
      gfx::Animation::RichAnimationRenderMode::FORCE_ENABLED);
  ASSERT_TRUE(scoped_mode);

  auto* panel_ui = SidePanelUI::From(browser());
  // Resolve side panel entries synchronously, so Show() reaches
  // SidePanel::Open() before it returns.
  panel_ui->SetNoDelaysForTesting(true);

  auto* side_panel = GetSidePanel();
  SidebarServiceFactory::GetForProfile(browser()->GetProfile())
      ->AddItem(SidebarItem::CreateWebPanelItem(GURL("https://brave.com/"),
                                                u"title"));
  const size_t web_panel_index = model()->GetAllSidebarItems().size() - 1;

  // Baseline: with no web panel on screen the open animates. Having the item
  // is not enough - only a visible panel suppresses.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_EQ(SidePanel::State::kOpening, side_panel->state());
  ASSERT_TRUE(side_panel->GetVisible());

  // The web panel replaces it, so the close must not animate. An animated
  // close would land on kClosing and stay on screen instead.
  controller()->OnItemPressed(web_panel_index);
  EXPECT_EQ(SidePanel::State::kClosed, side_panel->state());
  EXPECT_FALSE(side_panel->GetVisible());
  ASSERT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());

  // The same open now skips the animation, because it replaces the web panel.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  EXPECT_EQ(SidePanel::State::kOpen, side_panel->state());
  ASSERT_TRUE(side_panel->GetVisible());
  // Showing the side panel is what closed the web panel.
  ASSERT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());

  // Closing on its own animates out and stays on screen meanwhile.
  panel_ui->Close();
  ASSERT_EQ(SidePanel::State::kClosing, side_panel->state());
  ASSERT_TRUE(side_panel->GetVisible());

  // A panel already animating closed has to be forced shut rather than left
  // animating behind the new web panel - suppression also bypasses the
  // IsClosing() early return in SidePanelCoordinator::Close().
  controller()->OnItemPressed(web_panel_index);
  EXPECT_EQ(SidePanel::State::kClosed, side_panel->state());
  EXPECT_FALSE(side_panel->GetVisible());
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
}

// The web panel is backed by a pinned tab, so it has to be closed while the
// window is closing - before the session service and the tab restore service
// snapshot the window - or it reappears as an ordinary pinned tab. This covers
// the UnloadController hook; quitting the browser takes a different path, see
// WebPanelIsNotRestoredAfterShutdown.
IN_PROC_BROWSER_TEST_F(WebPanelBrowserTest, WebPanelClosesWithItsWindow) {
  auto* profile = browser()->GetProfile();
  auto* service = SidebarServiceFactory::GetForProfile(profile);

  const GURL panel_url(kTestWebPanelURL);
  service->AddItem(SidebarItem::CreateWebPanelItem(panel_url, u"title"));
  controller()->OnItemPressed(model()->GetAllSidebarItems().size() - 1);
  ASSERT_TRUE(web_panel_controller()->HasOpenPanel());
  ASSERT_EQ(2, tab_model()->count());

  // The panel's navigation has to commit, or the tab restore service would have
  // no URL to compare against and this test would pass even without the
  // window-closing hook. The load itself fails - there is no network - but the
  // error page still commits |panel_url|.
  auto* panel_contents = tab_model()->GetTabAtIndex(0)->GetContents();
  ASSERT_EQ(panel_contents, web_panel_controller()->panel_contents());
  content::WaitForLoadStopWithoutSuccessCheck(panel_contents);
  ASSERT_EQ(panel_url, panel_contents->GetLastCommittedURL());

  // Keep the browser process alive so the closing window gets recorded.
  ASSERT_TRUE(CreateBrowser(profile));

  auto* tab_restore_service = TabRestoreServiceFactory::GetForProfile(profile);
  ASSERT_TRUE(tab_restore_service);
  CloseBrowserSynchronously(browser());

  ASSERT_FALSE(tab_restore_service->entries().empty());
  const auto& entry = *tab_restore_service->entries().front();
  ASSERT_EQ(sessions::tab_restore::WINDOW, entry.type);
  for (const auto& tab :
       static_cast<const sessions::tab_restore::Window&>(entry).tabs) {
    const auto& navigation =
        tab->navigations[tab->normalized_navigation_index()];
    EXPECT_NE(panel_url, navigation.virtual_url())
        << "the web panel's pinned tab is restorable";
  }
}

// Quitting shuts the session services down before any window starts closing, so
// the per-window UnloadController hook covered by WebPanelClosesWithItsWindow
// never runs too late to affect the persisted session. Leaves an open web panel
// behind for WebPanelIsNotRestoredAfterShutdown to look for after a real
// relaunch.
IN_PROC_BROWSER_TEST_F(WebPanelSessionRestoreBrowserTest,
                       PRE_WebPanelIsNotRestoredAfterShutdown) {
  // Navigate the active tab so the session records a committed entry for it.
  // WebPanelIsNotRestoredAfterShutdown looks for this URL to tell that the
  // restore has landed.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kRegularTabURL)));
  ASSERT_FALSE(web_panel_controller()->HasOpenPanel());

  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());

  const GURL panel_url(kTestWebPanelURL);
  service->AddItem(SidebarItem::CreateWebPanelItem(panel_url, u"title"));
  controller()->OnItemPressed(model()->GetAllSidebarItems().size() - 1);

  ASSERT_TRUE(web_panel_controller()->HasOpenPanel());
  ASSERT_EQ(2, tab_model()->count());
  ASSERT_TRUE(tab_model()->GetTabAtIndex(0)->IsPinned());
  auto* panel_contents = tab_model()->GetTabAtIndex(0)->GetContents();
  ASSERT_EQ(panel_contents, web_panel_controller()->panel_contents());

  // The panel's navigation has to commit, or the session would have nothing to
  // restore and WebPanelIsNotRestoredAfterShutdown would pass even without the
  // shutdown hook. The load itself fails - there is no network - but the error
  // page still commits the panel URL.
  content::WaitForLoadStopWithoutSuccessCheck(panel_contents);
  ASSERT_EQ(panel_url, panel_contents->GetLastCommittedURL());

  // The session is written by the fixture's teardown, which quits via
  // chrome::AttemptExit() - the path under test. Don't snapshot it here.
}

IN_PROC_BROWSER_TEST_F(WebPanelSessionRestoreBrowserTest,
                       WebPanelIsNotRestoredAfterShutdown) {
  auto has_tab_with_url = [&](const GURL& url) {
    for (int i = 0; i < tab_model()->count(); ++i) {
      if (tab_model()->GetWebContentsAt(i)->GetVisibleURL() == url) {
        return true;
      }
    }
    return false;
  };

  // Session restore fills a window's tabs in one pass, so once the regular tab
  // from the previous session is here, the window's tab set is final. Don't
  // assert on the tab count instead: a launch appends its own startup tabs to
  // the restored window (see SessionRestoreImpl::AppendURLsToBrowser), so the
  // total depends on plumbing this test doesn't care about.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !SessionRestore::IsRestoring(browser()->GetProfile()) &&
           has_tab_with_url(GURL(kRegularTabURL));
  }));

  EXPECT_EQ(0, tab_model()->IndexOfFirstNonPinnedTab())
      << "nothing should have been restored as a pinned tab";
  EXPECT_FALSE(has_tab_with_url(GURL(kTestWebPanelURL)))
      << "the web panel's pinned tab was restored";

  // Only the panel's tab is dropped - the sidebar item itself persists.
  const auto& items = model()->GetAllSidebarItems();
  ASSERT_FALSE(items.empty());
  EXPECT_TRUE(items.back().is_web_panel_type());
  EXPECT_EQ(GURL(kTestWebPanelURL), items.back().url);
}

// The shutdown hooks snapshot the session on the statement right after they
// close the panel, so the panel's pinned tab has to be out of the strip by the
// time the close returns. chrome::CloseWebContents() does not guarantee that:
// CloseWebContentses() skips the detach and waits for the renderer whenever any
// frame has a beforeunload, unload, pagehide or visibilitychange handler, which
// covers most real pages. Any one of them reproduces, so this uses `pagehide`:
// unlike `beforeunload` it never puts up a dialog that could hang the test.
IN_PROC_BROWSER_TEST_F(WebPanelBrowserTest,
                       CloseAllWebPanelsForShutdownIsSynchronous) {
  ASSERT_TRUE(embedded_test_server()->Start());

  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  const GURL panel_url = embedded_test_server()->GetURL("/title1.html");
  service->AddItem(SidebarItem::CreateWebPanelItem(panel_url, u"title"));
  controller()->OnItemPressed(model()->GetAllSidebarItems().size() - 1);

  ASSERT_TRUE(web_panel_controller()->HasOpenPanel());
  auto* panel_contents = tab_model()->GetTabAtIndex(0)->GetContents();
  ASSERT_TRUE(content::WaitForLoadStop(panel_contents));

  ASSERT_TRUE(content::ExecJs(
      panel_contents, "window.addEventListener('pagehide', () => {});"));
  // The sudden-termination-disabler state reaches the browser process on its
  // own IPC, so wait for it rather than assuming ExecJs() was enough.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_contents->NeedToFireBeforeUnloadOrUnloadEvents();
  }));

  const int tab_count_before = tab_model()->count();
  CloseAllWebPanelsForShutdown();

  // Deliberately no RunLoop here - the point is that the tab is already gone.
  EXPECT_EQ(tab_count_before - 1, tab_model()->count());
  EXPECT_FALSE(web_panel_controller()->HasOpenPanel());
}

// prefs::kPinnedTabs is a second, independent restore path: PinnedTabService
// snapshots the live tab strips from chrome::OnClosingAllBrowsers(true), which
// fires at the very start of a quit - long before the hooks that
// WebPanelIsNotRestoredAfterShutdown covers get to close the panel. That pref
// is only replayed when the startup pref is *not* LAST, which is why these
// tests can't use WebPanelSessionRestoreBrowserTest: it would make
// GetPinnedTabsForState() discard the pref unread.
class WebPanelPinnedTabPrefBrowserTest : public WebPanelBrowserTest {
 protected:
  // Writing the startup pref here only takes effect for the *next* launch,
  // since the main run restores before SetUpOnMainThread(). That is what makes
  // it load-bearing: the PRE_ run persists DEFAULT for the main run to launch
  // with.
  void SetUpOnMainThread() override {
    WebPanelBrowserTest::SetUpOnMainThread();
    SessionStartupPref::SetStartupPref(
        browser()->GetProfile(),
        SessionStartupPref(SessionStartupPref::DEFAULT));
  }

  bool PinnedTabsPrefHasURL(const GURL& url) {
    return testing::Value(
        browser()->GetProfile()->GetPrefs()->GetList(prefs::kPinnedTabs),
        testing::Contains(
            base::test::DictionaryHasValue("url", base::Value(url.spec()))));
  }

  bool HasTabWithURL(const GURL& url) {
    for (int i = 0; i < tab_model()->count(); ++i) {
      if (tab_model()->GetWebContentsAt(i)->GetVisibleURL() == url) {
        return true;
      }
    }
    return false;
  }
};

IN_PROC_BROWSER_TEST_F(WebPanelPinnedTabPrefBrowserTest,
                       PRE_WebPanelIsNotRestoredAsPinnedTab) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kRegularTabURL)));
  ASSERT_FALSE(web_panel_controller()->HasOpenPanel());

  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());

  const GURL panel_url(kTestWebPanelURL);
  service->AddItem(SidebarItem::CreateWebPanelItem(panel_url, u"title"));
  controller()->OnItemPressed(model()->GetAllSidebarItems().size() - 1);

  ASSERT_TRUE(web_panel_controller()->HasOpenPanel());
  ASSERT_TRUE(tab_model()->GetTabAtIndex(0)->IsPinned());
  auto* panel_contents = tab_model()->GetTabAtIndex(0)->GetContents();
  ASSERT_EQ(panel_contents, web_panel_controller()->panel_contents());

  // A pinned tab the user really did pin, which must survive the filtering that
  // drops the panel. It also keeps the main test honest: if the pinned tab
  // restore path were not running at all, the panel's absence there would prove
  // nothing.
  auto* user_pinned_contents = chrome::AddAndReturnTabAt(
      browser(), GURL(kUserPinnedTabURL), 1, false, std::nullopt, true);
  ASSERT_TRUE(content::WaitForLoadStop(user_pinned_contents));

  // EncodePinnedTabs() skips tabs with no committed entry, so without this the
  // pref would stay clean for the wrong reason. The load fails - there is no
  // network - but the error page still commits the panel URL.
  content::WaitForLoadStopWithoutSuccessCheck(panel_contents);
  ASSERT_EQ(panel_url, panel_contents->GetLastCommittedURL());

  // The pref is written by the fixture's teardown, which quits via
  // chrome::AttemptExit() - the path under test. Don't snapshot it here.
}

IN_PROC_BROWSER_TEST_F(WebPanelPinnedTabPrefBrowserTest,
                       WebPanelIsNotRestoredAsPinnedTab) {
  EXPECT_TRUE(PinnedTabsPrefHasURL(GURL(kUserPinnedTabURL)))
      << "the user's own pinned tab should still be persisted";
  EXPECT_FALSE(PinnedTabsPrefHasURL(GURL(kTestWebPanelURL)))
      << "the web panel was persisted as one of the user's pinned tabs";

  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return HasTabWithURL(GURL(kUserPinnedTabURL)); }));

  EXPECT_EQ(1, tab_model()->IndexOfFirstNonPinnedTab())
      << "only the user's own pinned tab should have been restored as pinned";
  EXPECT_FALSE(HasTabWithURL(GURL(kTestWebPanelURL)))
      << "the web panel was relaunched as a pinned tab";
}

}  // namespace sidebar
