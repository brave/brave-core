// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/tab_search_bubble_host.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/button_test_api.h"

class VerticalTabSearchButtonBrowserTest : public InProcessBrowserTest {
 public:
  BraveBrowserView* browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  TabSearchButton* tab_search_button() {
    return browser_view()
        ->vertical_tab_strip_widget_delegate_view()
        ->vertical_tab_strip_region_view()
        ->GetTabSearchButtonForTesting();
  }

  TabSearchBubbleHost* tab_search_bubble_host() {
    return browser_view()->GetTabSearchBubbleHost();
  }

  WebUIBubbleManager* bubble_manager() {
    return tab_search_bubble_host()->webui_bubble_manager_for_testing();
  }

  void RunUntilBubbleWidgetDestroyed() {
    ASSERT_NE(nullptr, bubble_manager()->GetBubbleWidget());
    base::RunLoop run_loop;
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, run_loop.QuitClosure());
    run_loop.Run();
    ASSERT_EQ(nullptr, bubble_manager()->GetBubbleWidget());
  }
};

IN_PROC_BROWSER_TEST_F(VerticalTabSearchButtonBrowserTest,
                       ButtonClickCreatesBubble) {
  brave::ToggleVerticalTabStrip(browser());
  ASSERT_TRUE(tabs::utils::ShouldShowVerticalTabs(browser()));

  ASSERT_EQ(nullptr, bubble_manager()->GetBubbleWidget());

  ui::MouseEvent dummy_event(ui::MouseEvent(ui::EventType::kMousePressed,
                                            gfx::PointF(), gfx::PointF(),
                                            base::TimeTicks::Now(), 0, 0));
  views::test::ButtonTestApi(tab_search_button()).NotifyClick(dummy_event);
  ASSERT_NE(nullptr, bubble_manager()->GetBubbleWidget());

  tab_search_bubble_host()->CloseTabSearchBubble();
  ASSERT_TRUE(bubble_manager()->GetBubbleWidget()->IsClosed());

  RunUntilBubbleWidgetDestroyed();
}
