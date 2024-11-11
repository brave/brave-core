/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_root_view.h"

#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/drop_target_event.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom.h"
#include "ui/base/dragdrop/os_exchange_data.h"

class VerticalTabStripRootViewBrowserTest : public InProcessBrowserTest {
 public:
  VerticalTabStripRootViewBrowserTest() = default;
  VerticalTabStripRootViewBrowserTest(
      const VerticalTabStripRootViewBrowserTest&) = delete;
  VerticalTabStripRootViewBrowserTest& operator=(
      const VerticalTabStripRootViewBrowserTest&) = delete;
  ~VerticalTabStripRootViewBrowserTest() override = default;

  Tab* GetTabAt(int index) { return tab_strip()->tab_at(index); }

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  TabStrip* tab_strip() { return browser_view()->tabstrip(); }

  VerticalTabStripRootView* vtab_strip_root_view() {
    if (vtab_tab_strip_widget_delegate_view()) {
      return static_cast<VerticalTabStripRootView*>(
          vtab_tab_strip_widget_delegate_view()->GetWidget()->GetRootView());
    }
    return nullptr;
  }

  BrowserNonClientFrameView* browser_non_client_frame_view() {
    return browser_view()->frame()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  VerticalTabStripWidgetDelegateView* vtab_tab_strip_widget_delegate_view() {
    auto* browser_view = static_cast<BraveBrowserView*>(
        BrowserView::GetBrowserViewForBrowser(browser()));
    if (browser_view) {
      return browser_view->vertical_tab_strip_widget_delegate_view();
    }
    return nullptr;
  }

  void StartAndFinishDrag(const ui::OSExchangeData& data,
                          const gfx::Point& location,
                          ui::mojom::DragOperation& out_drag_op) {
    ui::DropTargetEvent event(data, gfx::PointF(location),
                              gfx::PointF(location),
                              ui::DragDropTypes::DRAG_COPY);
    VerticalTabStripRootView* root_view = vtab_strip_root_view();
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

IN_PROC_BROWSER_TEST_F(VerticalTabStripRootViewBrowserTest,
                       DragAfterCurrentTab) {
  ToggleVerticalTabStrip();

  ASSERT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));

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

IN_PROC_BROWSER_TEST_F(VerticalTabStripRootViewBrowserTest, DragOnCurrentTab) {
  ToggleVerticalTabStrip();

  ASSERT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));

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

IN_PROC_BROWSER_TEST_F(VerticalTabStripRootViewBrowserTest,
                       ContextMenuInUnobscuredRegion) {
  ToggleVerticalTabStrip();

  ASSERT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));

  auto* vtab_strip_root_view =
      vtab_tab_strip_widget_delegate_view()->vertical_tab_strip_region_view();

  EXPECT_FALSE(vtab_strip_root_view->IsMenuShowing());

  vtab_strip_root_view->ShowContextMenuForView(
      vtab_strip_root_view, gfx::Point(), ui::MENU_SOURCE_MOUSE);

  EXPECT_TRUE(vtab_strip_root_view->IsMenuShowing());
}
