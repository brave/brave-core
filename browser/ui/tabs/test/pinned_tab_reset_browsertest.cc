/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/test/pinned_tab_reset_browsertest.h"

#include <string>

#include "base/command_line.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/public/pinned_tab_reset_controller.h"
#include "brave/browser/ui/tabs/public/switches.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/pinned_tab_codec.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/common/root_tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_collection_node.h"
#include "chrome/browser/ui/views/tabs/common/tab_view.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/models/list_selection_model.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/views/view_utils.h"

namespace {

ui::MouseEvent CreateDoubleClick(const views::View& view, int extra_flags = 0) {
  return ui::MouseEvent(
      ui::EventType::kMousePressed,
      gfx::PointF(view.width() / 2.0f, view.height() / 2.0f), gfx::PointF(),
      base::TimeTicks::Now(),
      ui::EF_LEFT_MOUSE_BUTTON | ui::EF_IS_DOUBLE_CLICK | extra_flags,
      ui::EF_LEFT_MOUSE_BUTTON);
}

ui::MouseEvent CreateRelease(const views::View& view) {
  return ui::MouseEvent(ui::EventType::kMouseReleased,
                        gfx::PointF(view.width() / 2.0f, view.height() / 2.0f),
                        gfx::PointF(), base::TimeTicks::Now(),
                        ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
}

}  // namespace

PinnedTabResetBrowserTest::PinnedTabResetBrowserTest()
    : feature_list_(tabs::kBravePinnedTabReset) {}

PinnedTabResetBrowserTest::~PinnedTabResetBrowserTest() = default;

void PinnedTabResetBrowserTest::SetUpOnMainThread() {
  InProcessBrowserTest::SetUpOnMainThread();
  host_resolver()->AddRule("*", "127.0.0.1");
  ASSERT_TRUE(embedded_test_server()->Start());
  pinned_url_ = embedded_test_server()->GetURL("a.com", "/title1.html");
  other_url_ = embedded_test_server()->GetURL("a.com", "/title2.html");
  ASSERT_TRUE(controller());
}

TabStripModel* PinnedTabResetBrowserTest::model() {
  return browser()->tab_strip_model();
}

PinnedTabResetController* PinnedTabResetBrowserTest::controller() {
  return PinnedTabResetController::From(browser());
}

void PinnedTabResetBrowserTest::SetUpPinnedTabNavigatedAway() {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), pinned_url_));
  model()->SetTabPinned(0, true);
  chrome::AddTabAt(browser(), other_url_, /*index=*/-1, /*foreground=*/true);
  model()->ActivateTabAt(0);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), other_url_));
  ASSERT_EQ(other_url_, model()->GetWebContentsAt(0)->GetLastCommittedURL());
}

void PinnedTabResetBrowserTest::ExpectPinnedTabReset(int index,
                                                     base::Location location) {
  SCOPED_TRACE(location.ToString());
  content::WebContents* pinned = model()->GetWebContentsAt(index);
  EXPECT_TRUE(content::WaitForLoadStop(pinned));
  EXPECT_EQ(pinned_url_, pinned->GetLastCommittedURL());
  EXPECT_EQ(1, pinned->GetController().GetEntryCount());
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest, ShortcutResetsPinnedTab) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());
  ASSERT_EQ(2, model()->count());

  EXPECT_TRUE(controller()->HandleCloseTabShortcut());

  EXPECT_EQ(2, model()->count());
  EXPECT_TRUE(model()->IsTabPinned(0));
  EXPECT_EQ(1, model()->active_index());
  ExpectPinnedTabReset();
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       ShortcutOnUnpinnedTabIsNotHandled) {
  chrome::AddTabAt(browser(), other_url_, /*index=*/-1, /*foreground=*/true);
  ASSERT_EQ(2, model()->count());
  EXPECT_FALSE(controller()->HandleCloseTabShortcut());
  EXPECT_EQ(2, model()->count());
}

// A pinned tab without a pinned URL, here still on about:blank, closes as
// usual instead of the shortcut doing nothing.
IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       ShortcutOnPinnedTabWithoutPinnedUrlIsNotHandled) {
  ASSERT_TRUE(
      model()->GetWebContentsAt(0)->GetLastCommittedURL().IsAboutBlank());
  model()->SetTabPinned(0, true);
  chrome::AddTabAt(browser(), other_url_, /*index=*/-1, /*foreground=*/true);
  model()->ActivateTabAt(0);

  EXPECT_FALSE(controller()->HandleCloseTabShortcut());
  EXPECT_EQ(2, model()->count());
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       ShortcutOpensNewTabWhenAllTabsArePinned) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), pinned_url_));
  model()->SetTabPinned(0, true);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), other_url_));

  EXPECT_TRUE(controller()->HandleCloseTabShortcut());

  EXPECT_EQ(2, model()->count());
  EXPECT_TRUE(model()->IsTabPinned(0));
  EXPECT_FALSE(model()->IsTabPinned(1));
  EXPECT_EQ(1, model()->active_index());
  ExpectPinnedTabReset();
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       ShortcutResetsPinnedAndClosesUnpinnedSelection) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());
  chrome::AddTabAt(browser(), other_url_, /*index=*/-1, /*foreground=*/true);
  chrome::AddTabAt(browser(), other_url_, /*index=*/-1, /*foreground=*/true);
  ASSERT_EQ(4, model()->count());

  // Select the pinned tab and the first unpinned tab.
  model()->ActivateTabAt(0);
  model()->ExtendSelectionTo(1);
  ASSERT_TRUE(model()->IsTabSelected(0));
  ASSERT_TRUE(model()->IsTabSelected(1));

  EXPECT_TRUE(controller()->HandleCloseTabShortcut());

  EXPECT_EQ(3, model()->count());
  EXPECT_TRUE(model()->IsTabPinned(0));
  EXPECT_FALSE(model()->IsTabPinned(model()->active_index()));
  ExpectPinnedTabReset();
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       ShortcutResetsOnlyActiveHalfOfPinnedSplit) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), pinned_url_));
  chrome::AddTabAt(browser(), pinned_url_, /*index=*/-1, /*foreground=*/true);
  chrome::AddTabAt(browser(), other_url_, /*index=*/-1, /*foreground=*/true);
  ASSERT_TRUE(content::WaitForLoadStop(model()->GetWebContentsAt(1)));

  model()->ActivateTabAt(0);
  model()->AddToNewSplit({1}, split_tabs::SplitTabVisualData(),
                         split_tabs::SplitTabCreatedSource::kTabContextMenu);
  chrome::PinTab(browser());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return model()->IsTabPinned(0) && model()->IsTabPinned(1); }));

  for (int i : {0, 1}) {
    ASSERT_TRUE(
        content::NavigateToURL(model()->GetWebContentsAt(i), other_url_));
  }
  const int active_index = model()->active_index();
  ASSERT_TRUE(active_index == 0 || active_index == 1);
  const int other_half_index = 1 - active_index;

  EXPECT_TRUE(controller()->HandleCloseTabShortcut());

  EXPECT_EQ(3, model()->count());
  EXPECT_EQ(2, model()->active_index());
  ExpectPinnedTabReset(active_index);
  EXPECT_EQ(other_url_,
            model()->GetWebContentsAt(other_half_index)->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       ContextMenuCloseClosesPinnedTab) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());

  model()->ExecuteContextMenuCommand(0, TabStripModel::CommandCloseTab);

  EXPECT_EQ(1, model()->count());
  EXPECT_FALSE(model()->IsTabPinned(0));
}

// The close tab command not coming from the shortcut, e.g. the File menu on
// Mac, closes pinned tabs.
IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       CloseTabCommandClosesPinnedTab) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());

  chrome::ExecuteCommand(browser(), IDC_CLOSE_TAB);

  EXPECT_EQ(1, model()->count());
  EXPECT_FALSE(model()->IsTabPinned(0));
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest, UnpinningForgetsPinnedUrl) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());

  model()->SetTabPinned(0, false);
  EXPECT_FALSE(controller()->HandleCloseTabShortcut());

  // Pinning again remembers the page the tab is on now.
  model()->SetTabPinned(0, true);
  model()->ActivateTabAt(0);
  EXPECT_TRUE(controller()->HandleCloseTabShortcut());
  content::WebContents* pinned = model()->GetWebContentsAt(0);
  EXPECT_TRUE(content::WaitForLoadStop(pinned));
  EXPECT_EQ(other_url_, pinned->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       RestoredClosedTabKeepsPinnedUrl) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());
  model()->ExecuteContextMenuCommand(0, TabStripModel::CommandCloseTab);
  ASSERT_EQ(1, model()->count());

  chrome::RestoreTab(browser());
  ASSERT_TRUE(base::test::RunUntil([&]() { return model()->count() == 2; }));
  ASSERT_TRUE(model()->IsTabPinned(0));
  ASSERT_TRUE(content::WaitForLoadStop(model()->GetWebContentsAt(0)));
  ASSERT_EQ(other_url_, model()->GetWebContentsAt(0)->GetLastCommittedURL());

  model()->ActivateTabAt(0);
  EXPECT_TRUE(controller()->HandleCloseTabShortcut());
  ExpectPinnedTabReset();
}

// The startup pinned tabs pref keeps the last visited URL, so turning the
// feature off reopens it there, and adds the pinned URL used with it on.
IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       StartupPinnedTabsOpenAtPinnedUrl) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());
  Profile* profile = browser()->GetProfile();

  PinnedTabCodec::WritePinnedTabs(profile);

  const base::ListValue& pinned_tabs =
      profile->GetPrefs()->GetList(prefs::kPinnedTabs);
  ASSERT_EQ(1u, pinned_tabs.size());
  ASSERT_TRUE(pinned_tabs[0].is_dict());
  const std::string* url = pinned_tabs[0].GetDict().FindString("url");
  ASSERT_TRUE(url);
  EXPECT_EQ(other_url_.spec(), *url);

  const StartupTabs startup_tabs = PinnedTabCodec::ReadPinnedTabs(profile);
  ASSERT_EQ(1u, startup_tabs.size());
  EXPECT_EQ(pinned_url_, startup_tabs[0].url);
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest, DoubleClickResetsPinnedTab) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());

  TabStrip* tab_strip = BrowserView::GetBrowserViewForBrowser(browser())
                            ->horizontal_tab_strip_for_testing();
  tab_strip->StopAnimating();
  Tab* tab = tab_strip->tab_at(0);
  ASSERT_TRUE(tab);

  tab->OnMousePressed(CreateDoubleClick(*tab));
  tab->OnMouseReleased(CreateRelease(*tab));

  EXPECT_EQ(2, model()->count());
  ExpectPinnedTabReset();
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest,
                       DoubleClickWithModifierDoesNotReset) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());

  TabStrip* tab_strip = BrowserView::GetBrowserViewForBrowser(browser())
                            ->horizontal_tab_strip_for_testing();
  tab_strip->StopAnimating();
  Tab* tab = tab_strip->tab_at(0);
  ASSERT_TRUE(tab);

  tab->OnMousePressed(CreateDoubleClick(*tab, ui::EF_ALT_DOWN));
  tab->OnMouseReleased(CreateRelease(*tab));

  content::WebContents* pinned = model()->GetWebContentsAt(0);
  EXPECT_FALSE(pinned->IsLoading());
  EXPECT_EQ(other_url_, pinned->GetLastCommittedURL());
  EXPECT_TRUE(pinned->GetController().CanGoBack());
}

// Chromium's vertical tab strip, used with the vertical tab migration flag.
class PinnedTabResetUpstreamVerticalTabsBrowserTest
    : public PinnedTabResetBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    PinnedTabResetBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        tabs::switches::kVerticalTabMigrationSwitch,
        tabs::switches::kVerticalTabMigrationForceUpstreamValue);
  }

  void SetUpOnMainThread() override {
    PinnedTabResetBrowserTest::SetUpOnMainThread();
    auto* state_controller =
        tabs::VerticalTabStripStateController::From(browser());
    state_controller->SetVerticalTabsEnabled(true);
    ASSERT_TRUE(state_controller->ShouldDisplayVerticalTabs());
  }

 protected:
  TabView* GetTabViewAt(int index) {
    TabCollectionNode* node =
        BrowserView::GetBrowserViewForBrowser(browser())
            ->vertical_tab_strip_region_view_for_testing()
            ->root_node_for_testing()
            ->GetNodeForHandle(model()->GetTabAtIndex(index)->GetHandle());
    return node ? views::AsViewClass<TabView>(node->view()) : nullptr;
  }
};

IN_PROC_BROWSER_TEST_F(PinnedTabResetUpstreamVerticalTabsBrowserTest,
                       DoubleClickResetsPinnedTab) {
  ASSERT_TRUE(tabs::IsUpstreamVerticalTabsForceEnabled());
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());

  TabView* tab_view = nullptr;
  ASSERT_TRUE(base::test::RunUntil([&]() {
    tab_view = GetTabViewAt(0);
    return tab_view && tab_view->pinned();
  }));

  // TabView's mouse handlers are private, so call them through views::View.
  views::View* view = tab_view;
  view->OnMousePressed(CreateDoubleClick(*view));
  view->OnMouseReleased(CreateRelease(*view));

  EXPECT_EQ(2, model()->count());
  ExpectPinnedTabReset();
}

class PinnedTabResetSharedPinnedTabsBrowserTest
    : public PinnedTabResetBrowserTest {
 public:
  PinnedTabResetSharedPinnedTabsBrowserTest()
      : shared_pinned_tabs_feature_list_(tabs::kBraveSharedPinnedTabs) {}

  void SetUpOnMainThread() override {
    PinnedTabResetBrowserTest::SetUpOnMainThread();
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        brave_tabs::kSharedPinnedTab, true);
  }

 private:
  base::test::ScopedFeatureList shared_pinned_tabs_feature_list_;
};

// The placeholder a shared pinned tab leaves in other windows must not replace
// the URL the tab was pinned with when the tab moves between windows.
IN_PROC_BROWSER_TEST_F(PinnedTabResetSharedPinnedTabsBrowserTest,
                       MovingBetweenWindowsKeepsPinnedUrl) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());
  auto* service =
      SharedPinnedTabServiceFactory::GetForProfile(browser()->GetProfile());
  ASSERT_TRUE(service);
  content::WebContents* shared_contents = model()->GetWebContentsAt(0);
  ASSERT_TRUE(service->IsSharedContents(shared_contents));

  BrowserWindowInterface* browser_2 = chrome::OpenEmptyWindow(
      browser()->GetProfile(), /*should_trigger_session_restore=*/false);
  TabStripModel* model_2 = browser_2->GetTabStripModel();
  ASSERT_TRUE(base::test::RunUntil([&]() { return model_2->count() > 1; }));
  ASSERT_TRUE(service->IsDummyContents(model_2->GetWebContentsAt(0)));

  // Activating the placeholder brings the shared contents to `browser_2`.
  BrowserWindow::FromBrowser(browser_2)->Show();
  ui::ListSelectionModel selection;
  selection.set_active(0);
  selection.SetSelectedIndex(0);
  model_2->SetSelectionFromModel(selection);
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return model_2->GetWebContentsAt(0) == shared_contents; }));

  auto* controller_2 = PinnedTabResetController::From(browser_2);
  ASSERT_TRUE(controller_2);
  EXPECT_TRUE(controller_2->HandleCloseTabShortcut());

  EXPECT_TRUE(content::WaitForLoadStop(shared_contents));
  EXPECT_EQ(pinned_url_, shared_contents->GetLastCommittedURL());
}

// A placeholder selected with other tabs resets the shared tab it stands for
// instead of closing it in every window.
IN_PROC_BROWSER_TEST_F(PinnedTabResetSharedPinnedTabsBrowserTest,
                       ShortcutOnPlaceholderResetsSharedTab) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());
  auto* service =
      SharedPinnedTabServiceFactory::GetForProfile(browser()->GetProfile());
  ASSERT_TRUE(service);
  content::WebContents* shared_contents = model()->GetWebContentsAt(0);

  BrowserWindowInterface* browser_2 = chrome::OpenEmptyWindow(
      browser()->GetProfile(), /*should_trigger_session_restore=*/false);
  TabStripModel* model_2 = browser_2->GetTabStripModel();
  ASSERT_TRUE(base::test::RunUntil([&]() { return model_2->count() > 1; }));
  ASSERT_TRUE(service->IsDummyContents(model_2->GetWebContentsAt(0)));
  ASSERT_FALSE(model_2->IsTabPinned(1));

  // Select the placeholder along with the active unpinned tab.
  ui::ListSelectionModel selection;
  selection.SetSelectedIndex(0);
  selection.AddIndexToSelection(1);
  selection.set_active(1);
  selection.set_anchor(1);
  model_2->SetSelectionFromModel(selection);
  ASSERT_TRUE(model_2->IsTabSelected(0));
  ASSERT_EQ(1, model_2->active_index());

  auto* controller_2 = PinnedTabResetController::From(browser_2);
  ASSERT_TRUE(controller_2);
  EXPECT_TRUE(controller_2->HandleCloseTabShortcut());

  EXPECT_EQ(shared_contents, model()->GetWebContentsAt(0));
  EXPECT_TRUE(model()->IsTabPinned(0));
  ExpectPinnedTabReset();
  ASSERT_TRUE(model_2->IsTabPinned(0));
  EXPECT_TRUE(service->IsDummyContents(model_2->GetWebContentsAt(0)));
}

// Unpinning from a window that has the placeholder brings the shared contents
// there unpinned, and they must forget their pinned URL.
IN_PROC_BROWSER_TEST_F(PinnedTabResetSharedPinnedTabsBrowserTest,
                       UnpinningPlaceholderForgetsPinnedUrl) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());
  content::WebContents* shared_contents = model()->GetWebContentsAt(0);
  ASSERT_TRUE(pinned_tab_reset::GetPinnedUrl(shared_contents));

  BrowserWindowInterface* browser_2 = chrome::OpenEmptyWindow(
      browser()->GetProfile(), /*should_trigger_session_restore=*/false);
  TabStripModel* model_2 = browser_2->GetTabStripModel();
  ASSERT_TRUE(base::test::RunUntil([&]() { return model_2->count() > 1; }));

  model_2->SetTabPinned(0, false);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return model_2->GetIndexOfWebContents(shared_contents) !=
           TabStripModel::kNoTab;
  }));
  EXPECT_FALSE(
      model_2->IsTabPinned(model_2->GetIndexOfWebContents(shared_contents)));
  EXPECT_FALSE(pinned_tab_reset::GetPinnedUrl(shared_contents));
}

// Uses chrome:// URLs, as the test server's port changes between launches.
class PinnedTabResetSessionRestoreBrowserTest
    : public PinnedTabResetBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PinnedTabResetBrowserTest::SetUpOnMainThread();
    pinned_url_ = GURL("chrome://version/");
    other_url_ = GURL("chrome://credits/");
    SessionStartupPref::SetStartupPref(
        browser()->GetProfile(), SessionStartupPref(SessionStartupPref::LAST));
  }
};

IN_PROC_BROWSER_TEST_F(PinnedTabResetSessionRestoreBrowserTest,
                       PRE_RestartKeepsPinnedUrl) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());
  // Also covers rewriting the pinned URL when the session file is rebuilt.
  SessionServiceFactory::GetForProfile(browser()->GetProfile())
      ->ResetFromCurrentBrowsers();
}

IN_PROC_BROWSER_TEST_F(PinnedTabResetSessionRestoreBrowserTest,
                       RestartKeepsPinnedUrl) {
  ASSERT_EQ(2, model()->count());
  ASSERT_TRUE(model()->IsTabPinned(0));
  model()->ActivateTabAt(0);
  ASSERT_TRUE(content::WaitForLoadStop(model()->GetWebContentsAt(0)));
  ASSERT_EQ(other_url_, model()->GetWebContentsAt(0)->GetLastCommittedURL());

  EXPECT_TRUE(controller()->HandleCloseTabShortcut());
  ExpectPinnedTabReset();
}
