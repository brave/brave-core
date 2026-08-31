// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstdint>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_browsertest_util.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/events/event_constants.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)

namespace {

// Waits a bounded duration and records whether a context menu was shown in the
// meantime. Unlike the upstream ContextMenuWaiter (which blocks until a menu
// is shown), this returns false on timeout, making it usable for negative
// assertions.
class NoContextMenuWaiter {
 public:
  explicit NoContextMenuWaiter(bool* menu_shown) : menu_shown_(menu_shown) {}
  ~NoContextMenuWaiter() = default;

  NoContextMenuWaiter(const NoContextMenuWaiter&) = delete;
  NoContextMenuWaiter& operator=(const NoContextMenuWaiter&) = delete;

  // Returns true if a context menu was shown before the timeout.
  bool WaitForMenu(base::TimeDelta timeout) {
    RenderViewContextMenu::RegisterMenuShownCallbackForTesting(base::BindOnce(
        &NoContextMenuWaiter::MenuShown, base::Unretained(this)));
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&NoContextMenuWaiter::Timeout, base::Unretained(this)),
        timeout);
    loop_.Run();
    return *menu_shown_;
  }

 private:
  void MenuShown(RenderViewContextMenu* context_menu) {
    *menu_shown_ = true;
    // Defer cancelling the menu so we don't reinvoke the run loop from inside
    // the menu-shown callback (mirrors ContextMenuWaiter::MenuShown).
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&NoContextMenuWaiter::Cancel,
                                  base::Unretained(this), context_menu));
  }

  void Cancel(RenderViewContextMenu* context_menu) {
    context_menu->Cancel();
    loop_.Quit();
  }

  void Timeout() {
    if (!*menu_shown_) {
      loop_.Quit();
    }
  }

  raw_ptr<bool> menu_shown_;
  base::RunLoop loop_;
};

// Browser test for the ForceContextMenuOnShiftRightClick feature: a page's
// preventDefault() on a contextmenu event is bypassed when Shift is held,
// except when the event target is a <canvas> element.
class ShiftRightClickBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  // Simulates a right-click (down + up) at |point| with optional Shift held.
  void SendRightClick(const gfx::Point& point, bool shift) {
    content::SimulateMouseClickAt(
        browser()->tab_strip_model()->GetActiveWebContents(),
        shift ? ui::EF_SHIFT_DOWN : 0, blink::WebMouseEvent::Button::kRight,
        point);
  }
};

// Test page: a full-page <div> and a 200x200 <canvas> in the top-left corner.
// The page always suppresses the context menu via preventDefault(), which is
// required for the feature's bypass to have an observable effect.
constexpr char kTestPage[] = "contextmenu_prevent_default.html";

}  // namespace

// When the feature is enabled, Shift + right-click on a non-canvas element
// shows the (default) context menu even if the page called preventDefault().
IN_PROC_BROWSER_TEST_F(ShiftRightClickBrowserTest,
                       ShiftRightClickOnDivFeatureEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      blink::features::kForceContextMenuOnShiftRightClick);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(kTestPage)));

  // Point clearly outside the 200x200 canvas in the top-left corner.
  SendRightClick(gfx::Point(500, 500), /*shift=*/true);

  // The bypass fires: the default context menu is shown.
  ContextMenuWaiter waiter;
  waiter.WaitForMenuOpenAndClose();
}

// The <canvas> exception: when the feature is enabled, Shift + right-click
// on a <canvas> element does NOT bypass the page's preventDefault().
IN_PROC_BROWSER_TEST_F(ShiftRightClickBrowserTest,
                       ShiftRightClickOnCanvasFeatureEnabledNoMenu) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      blink::features::kForceContextMenuOnShiftRightClick);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(kTestPage)));

  // Point inside the 200x200 canvas in the top-left corner.
  SendRightClick(gfx::Point(100, 100), /*shift=*/true);

  // No context menu should be shown.
  bool menu_shown = false;
  NoContextMenuWaiter waiter(&menu_shown);
  EXPECT_FALSE(waiter.WaitForMenu(base::Milliseconds(500)));
}

// Without Shift, the bypass does not fire and the page's preventDefault()
// stands, even when the feature is enabled.
IN_PROC_BROWSER_TEST_F(ShiftRightClickBrowserTest,
                       RightClickWithoutShiftFeatureEnabledNoMenu) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      blink::features::kForceContextMenuOnShiftRightClick);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(kTestPage)));

  SendRightClick(gfx::Point(500, 500), /*shift=*/false);

  bool menu_shown = false;
  NoContextMenuWaiter waiter(&menu_shown);
  EXPECT_FALSE(waiter.WaitForMenu(base::Milliseconds(500)));
}

// When the feature is disabled, Shift + right-click has no effect and the
// page's preventDefault() stands.
IN_PROC_BROWSER_TEST_F(ShiftRightClickBrowserTest,
                       ShiftRightClickOnDivFeatureDisabledNoMenu) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      blink::features::kForceContextMenuOnShiftRightClick);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(kTestPage)));

  SendRightClick(gfx::Point(500, 500), /*shift=*/true);

  bool menu_shown = false;
  NoContextMenuWaiter waiter(&menu_shown);
  EXPECT_FALSE(waiter.WaitForMenu(base::Milliseconds(500)));
}

#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
