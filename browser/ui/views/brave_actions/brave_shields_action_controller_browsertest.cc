// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_controller.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"
#include "url/gurl.h"

namespace {

class WidgetDestroyedWaiter : public views::WidgetObserver {
 public:
  explicit WidgetDestroyedWaiter(views::Widget* widget) {
    observation_.Observe(widget);
  }

  WidgetDestroyedWaiter(const WidgetDestroyedWaiter&) = delete;
  WidgetDestroyedWaiter& operator=(const WidgetDestroyedWaiter&) = delete;

  ~WidgetDestroyedWaiter() override = default;

  void Wait() { run_loop_.Run(); }

 private:
  void OnWidgetDestroyed(views::Widget* widget) override {
    observation_.Reset();
    run_loop_.Quit();
  }

  base::RunLoop run_loop_;
  base::ScopedObservation<views::Widget, views::WidgetObserver> observation_{
      this};
};

class BraveShieldsActionControllerBrowserTest : public InProcessBrowserTest {
 public:
  BraveShieldsActionControllerBrowserTest() = default;

  BraveShieldsActionControllerBrowserTest(
      const BraveShieldsActionControllerBrowserTest&) = delete;
  BraveShieldsActionControllerBrowserTest& operator=(
      const BraveShieldsActionControllerBrowserTest&) = delete;

  ~BraveShieldsActionControllerBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/empty.html")));

    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    ASSERT_TRUE(browser_view);

    anchor_view_ = browser_view->GetLocationBarView();
    ASSERT_TRUE(anchor_view_);
  }

 protected:
  std::unique_ptr<BraveShieldsActionController> CreateController() {
    auto controller = std::make_unique<BraveShieldsActionController>(
        browser(),
        base::BindRepeating(
            &BraveShieldsActionControllerBrowserTest::CreateBubbleManager,
            base::Unretained(this)));

    controller->SetAnchorView(anchor_view_);
    return controller;
  }

  std::unique_ptr<WebUIBubbleManager> CreateBubbleManager(
      views::View* anchor_view,
      BrowserWindowInterface* browser_window_interface,
      const GURL& webui_url,
      int task_manager_string_id,
      bool force_load_on_create) {
    EXPECT_EQ(anchor_view_, anchor_view);

    ++created_bubble_manager_count_;
    created_webui_urls_.push_back(webui_url);
    force_load_on_create_values_.push_back(force_load_on_create);

    auto manager = WebUIBubbleManager::Create<ShieldsPanelUI>(
        anchor_view, browser_window_interface, webui_url,
        task_manager_string_id, force_load_on_create);
    last_created_bubble_manager_ = manager.get();
    return manager;
  }

  WebUIBubbleManager* last_bubble_manager() const {
    return last_created_bubble_manager_;
  }

  size_t created_bubble_manager_count() const {
    return created_bubble_manager_count_;
  }

  void CloseBubble(BraveShieldsActionController* controller) {
    views::Widget* widget = controller->GetBubbleWidget();
    ASSERT_TRUE(widget);

    WidgetDestroyedWaiter waiter(widget);
    controller->OnButtonPressed();
    waiter.Wait();

    EXPECT_FALSE(controller->GetBubbleWidget());
  }

  void ExpectForceLoadOnCreateWasNeverUsed() {
    ASSERT_FALSE(force_load_on_create_values_.empty());
    for (bool force_load_on_create : force_load_on_create_values_) {
      EXPECT_FALSE(force_load_on_create);
    }
  }

  raw_ptr<views::View> anchor_view_ = nullptr;
  raw_ptr<WebUIBubbleManager> last_created_bubble_manager_ = nullptr;
  size_t created_bubble_manager_count_ = 0;
  std::vector<GURL> created_webui_urls_;
  std::vector<bool> force_load_on_create_values_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(BraveShieldsActionControllerBrowserTest,
                       ReopenUsesCachedContentsUntilThemeChange) {
  auto controller = CreateController();

  controller->OnButtonPressed();
  ASSERT_TRUE(last_bubble_manager());
  EXPECT_EQ(1u, created_bubble_manager_count());
  EXPECT_FALSE(last_bubble_manager()->bubble_using_cached_web_contents());
  CloseBubble(controller.get());

  // Normal reopen should keep WebUIBubbleManager's short-lived WebContents
  // cache hot.
  controller->OnButtonPressed();
  ASSERT_TRUE(last_bubble_manager());
  EXPECT_EQ(1u, created_bubble_manager_count());
  EXPECT_TRUE(last_bubble_manager()->bubble_using_cached_web_contents());
  CloseBubble(controller.get());

  // A theme change invalidates the closed cached Shields WebUI document. The
  // next open should create fresh contents instead of using the stale cache.
  controller->OnThemeChanged();

  controller->OnButtonPressed();
  ASSERT_TRUE(last_bubble_manager());
  EXPECT_EQ(2u, created_bubble_manager_count());
  EXPECT_FALSE(last_bubble_manager()->bubble_using_cached_web_contents());

  ExpectForceLoadOnCreateWasNeverUsed();
}

IN_PROC_BROWSER_TEST_F(BraveShieldsActionControllerBrowserTest,
                       ThemeChangeWhileVisibleReloadsWithoutClosingBubble) {
  auto controller = CreateController();

  controller->OnButtonPressed();
  ASSERT_TRUE(last_bubble_manager());
  EXPECT_EQ(1u, created_bubble_manager_count());

  views::Widget* visible_widget = controller->GetBubbleWidget();
  ASSERT_TRUE(visible_widget);

  WebUIContentsWrapper* contents_wrapper =
      last_bubble_manager()->GetContentsWrapper();
  ASSERT_TRUE(contents_wrapper);
  ASSERT_TRUE(contents_wrapper->web_contents());

  content::TestNavigationObserver reload_observer(
      contents_wrapper->web_contents());

  controller->OnThemeChanged();

  // A visible theme-change update should reload the WebUI document in place,
  // not destroy/recreate the bubble widget.
  EXPECT_EQ(visible_widget, controller->GetBubbleWidget());

  reload_observer.Wait();

  EXPECT_EQ(visible_widget, controller->GetBubbleWidget());
  EXPECT_EQ(1u, created_bubble_manager_count());

  CloseBubble(controller.get());

  // The visible WebUI was already reloaded. A normal reopen can keep using the
  // same manager/cache.
  controller->OnButtonPressed();
  ASSERT_TRUE(last_bubble_manager());
  EXPECT_EQ(1u, created_bubble_manager_count());
  EXPECT_TRUE(last_bubble_manager()->bubble_using_cached_web_contents());

  ExpectForceLoadOnCreateWasNeverUsed();
}

IN_PROC_BROWSER_TEST_F(BraveShieldsActionControllerBrowserTest,
                       DifferentPanelUrlCreatesNewBubbleManager) {
  auto controller = CreateController();

  controller->OnButtonPressed();
  ASSERT_TRUE(last_bubble_manager());
  EXPECT_EQ(1u, created_bubble_manager_count());
  EXPECT_FALSE(last_bubble_manager()->bubble_using_cached_web_contents());
  CloseBubble(controller.get());

  controller->OnRepeatedReloadsDetected();
  ASSERT_TRUE(last_bubble_manager());
  EXPECT_EQ(2u, created_bubble_manager_count());
  EXPECT_FALSE(last_bubble_manager()->bubble_using_cached_web_contents());

  ASSERT_EQ(2u, created_webui_urls_.size());
  EXPECT_NE(created_webui_urls_[0], created_webui_urls_[1]);

  ExpectForceLoadOnCreateWasNeverUsed();
}
