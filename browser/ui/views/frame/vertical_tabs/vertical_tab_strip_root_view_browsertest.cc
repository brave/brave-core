/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_root_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/drop_target_event.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/mojom/menu_source_type.mojom.h"

class VerticalTabStripRootViewBrowserTest : public InProcessBrowserTest {
 public:
  VerticalTabStripRootViewBrowserTest() = default;
  VerticalTabStripRootViewBrowserTest(
      const VerticalTabStripRootViewBrowserTest&) = delete;
  VerticalTabStripRootViewBrowserTest& operator=(
      const VerticalTabStripRootViewBrowserTest&) = delete;
  ~VerticalTabStripRootViewBrowserTest() override = default;

  Tab* GetTabAt(int index) {
    return horizontal_tab_strip_for_testing()->tab_at(index);
  }

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  TabStrip* horizontal_tab_strip_for_testing() {
    return browser_view()->horizontal_tab_strip_for_testing();
  }

  BrowserRootView* browser_root_view() {
    return static_cast<BrowserRootView*>(
        browser_view()->GetWidget()->GetRootView());
  }

  BrowserFrameView* browser_non_client_frame_view() {
    return browser_view()->browser_widget()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  BraveVerticalTabStripContainerView* vertical_tab_strip_container_view() {
    return static_cast<BraveBrowserView*>(browser_view())
        ->vertical_tab_strip_container_view();
  }

  void StartAndFinishDrag(const ui::OSExchangeData& data,
                          const gfx::Point& location,
                          ui::mojom::DragOperation& out_drag_op) {
    ui::DropTargetEvent event(data, gfx::PointF(location),
                              gfx::PointF(location),
                              ui::DragDropTypes::DRAG_COPY);
    BrowserRootView* root_view = browser_root_view();
    EXPECT_NE(nullptr, root_view);

    base::RunLoop run_loop;
    root_view->SetOnFilteringCompleteClosureForTesting(run_loop.QuitClosure());
    root_view->OnDragEntered(event);

    // At this point, the drag information will have been set, and a background
    // task will have been posted to process the dragged URLs
    // (`GetURLMimeTypes()` -> `FilterURLs()`). Ensure that all background
    // processing is complete before checking the drag operation or invoking the
    // drag callback.
    run_loop.Run();

    EXPECT_NE(ui::DragDropTypes::DRAG_NONE, root_view->OnDragUpdated(event));

    auto drop_cb = root_view->GetDropCallback(event);
    std::move(drop_cb).Run(event, out_drag_op,
                           /*drag_image_layer_owner=*/nullptr);
  }
};

// Before we have our own interactive ui tests, we need to disable this test as
// it's flaky when running test suits.
#define MAYBE_DragAfterCurrentTab DISABLED_DragAfterCurrentTab

IN_PROC_BROWSER_TEST_F(VerticalTabStripRootViewBrowserTest,
                       MAYBE_DragAfterCurrentTab) {
  ToggleVerticalTabStrip();

  ASSERT_TRUE(browser()
                  ->GetFeatures()
                  .vertical_tab_controller()
                  ->ShouldShowBraveVerticalTabs());

  auto* tab_strip_model = browser()->tab_strip_model();
  EXPECT_EQ(tab_strip_model->count(), 1);

  ui::OSExchangeData data;
  GURL url("https://brave.com/");
  data.SetURL(url, std::u16string());

  Tab* current_tab = GetTabAt(0);
  gfx::Point location;
  views::View::ConvertPointToWidget(current_tab, &location);

  // To drag after current tab.
  location.Offset(0, current_tab->height());
  ui::mojom::DragOperation output_drag_op = ui::mojom::DragOperation::kNone;
  StartAndFinishDrag(data, location, output_drag_op);

  EXPECT_EQ(output_drag_op, ui::mojom::DragOperation::kCopy);
  EXPECT_EQ(tab_strip_model->count(), 2);
  EXPECT_TRUE(browser()
                  ->tab_strip_model()
                  ->GetWebContentsAt(1)
                  ->GetURL()
                  .EqualsIgnoringRef(url));
}

// Before we have our own interactive ui tests, we need to disable this test as
// it's flaky when running test suits.
#define MAYBE_DragOnCurrentTab DISABLED_DragOnCurrentTab

IN_PROC_BROWSER_TEST_F(VerticalTabStripRootViewBrowserTest,
                       MAYBE_DragOnCurrentTab) {
  ToggleVerticalTabStrip();

  ASSERT_TRUE(browser()
                  ->GetFeatures()
                  .vertical_tab_controller()
                  ->ShouldShowBraveVerticalTabs());

  auto* tab_strip_model = browser()->tab_strip_model();
  EXPECT_EQ(tab_strip_model->count(), 1);

  ui::OSExchangeData data;
  GURL url("https://brave.com/");
  data.SetURL(url, std::u16string());

  Tab* current_tab = GetTabAt(0);
  gfx::Point location;
  views::View::ConvertPointToWidget(current_tab, &location);

  // To drag on the same tab.
  location.Offset(0, current_tab->height() / 2);
  ui::mojom::DragOperation output_drag_op = ui::mojom::DragOperation::kNone;
  StartAndFinishDrag(data, location, output_drag_op);

  EXPECT_EQ(output_drag_op, ui::mojom::DragOperation::kCopy);
  EXPECT_EQ(tab_strip_model->count(), 1);
  EXPECT_TRUE(browser()
                  ->tab_strip_model()
                  ->GetWebContentsAt(0)
                  ->GetURL()
                  .EqualsIgnoringRef(url));
}

// System menu is used on Windows, so the menu_runner_ path under test does not
// apply there. On macOS, `views::MenuRunner` shows a native NSMenu whose
// tracking event loop (`-[NSMenu popUpContextMenu:]` via `ui::ShowContextMenu`)
// runs synchronously and blocks `RunMenuAt()` until the menu is dismissed. A
// browser test has no user to dismiss it, so the run loop hangs until the test
// harness terminates it (SIGTERM), making the test flaky. See
// https://github.com/brave/brave-browser/issues/56403.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#define MAYBE_ContextMenuInUnobscuredRegion \
  DISABLED_ContextMenuInUnobscuredRegion
#else
#define MAYBE_ContextMenuInUnobscuredRegion ContextMenuInUnobscuredRegion
#endif
IN_PROC_BROWSER_TEST_F(VerticalTabStripRootViewBrowserTest,
                       MAYBE_ContextMenuInUnobscuredRegion) {
  ToggleVerticalTabStrip();

  ASSERT_TRUE(browser()
                  ->GetFeatures()
                  .vertical_tab_controller()
                  ->ShouldShowBraveVerticalTabs());

  auto* region_view =
      vertical_tab_strip_container_view()->vertical_tab_strip_region_view();

  EXPECT_FALSE(region_view->IsMenuShowing());

  region_view->ShowContextMenuForView(region_view, gfx::Point(),
                                      ui::mojom::MenuSourceType::kMouse);

  EXPECT_TRUE(region_view->IsMenuShowing());
}

// Testing menu tear down for `menu_runner_`, which is only used on non-Windows
// platforms. Disabled on macOS for the same reason as the
// MAYBE_ContextMenuInUnobscuredRegion test: the native NSMenu tracking loop
// blocks `RunMenuAt()` synchronously until dismissed, hanging the test until it
// is terminated (SIGTERM). See
// https://github.com/brave/brave-browser/issues/56402.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#define MAYBE_CloseBrowserWithContextMenuOpen \
  DISABLED_CloseBrowserWithContextMenuOpen
#else
#define MAYBE_CloseBrowserWithContextMenuOpen CloseBrowserWithContextMenuOpen
#endif
IN_PROC_BROWSER_TEST_F(VerticalTabStripRootViewBrowserTest,
                       MAYBE_CloseBrowserWithContextMenuOpen) {
  ToggleVerticalTabStrip();

  ASSERT_TRUE(browser()
                  ->GetFeatures()
                  .vertical_tab_controller()
                  ->ShouldShowBraveVerticalTabs());

  auto* region_view =
      vertical_tab_strip_container_view()->vertical_tab_strip_region_view();
  ASSERT_FALSE(region_view->IsMenuShowing());

  region_view->ShowContextMenuForView(region_view, gfx::Point(),
                                      ui::mojom::MenuSourceType::kMouse);
  ASSERT_TRUE(region_view->IsMenuShowing());

  // Keep the browser process alive after the target browser is closed.
  ASSERT_TRUE(CreateBrowser(browser()->profile()));

  // Closing the browser must not use the system menu model after it has been
  // freed.
  CloseBrowserSynchronously(browser());
}
