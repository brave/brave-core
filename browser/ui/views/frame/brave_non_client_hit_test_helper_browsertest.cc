/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/reload_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/hit_test.h"
#include "ui/views/view.h"

using BraveNonClientHitTestHelperBrowserTest = InProcessBrowserTest;

// TODO(sko) It might be good to have resizable area tests. But testing it
// is pretty flaky depending on platforms.
IN_PROC_BROWSER_TEST_F(BraveNonClientHitTestHelperBrowserTest, Toolbar) {
  auto* browser_view =
      static_cast<BrowserView*>(BrowserWindow::FromBrowser(browser()));
  auto* toolbar = browser_view->toolbar();
  auto* frame_view = browser_view->browser_widget()->GetFrameView();

  for (views::View* view : toolbar->GetChildrenInZOrder()) {
    // When a point is on a child view, hit test result will be HTCLIENT (see
    // BraveToolbarView::ViewHierarchyChanged where we set children that way).
    // To test the ability to drag by the toolbar hide the children.
    view->SetVisible(false);
  }

  gfx::Point point = toolbar->GetLocalBounds().CenterPoint();
  views::View::ConvertPointToWidget(toolbar, &point);

  // Dragging a window by the empty toolbar should work.
  EXPECT_EQ(HTCAPTION, frame_view->NonClientHitTest(point));

  // Any point within a child of the toolbar shouldn't be HTCAPTION so that
  // users can interact with the child elements. Checks a typical child of
  // toolbar as a sanity check.
  toolbar->SetVisible(true);
  for (views::View* view : toolbar->GetChildrenInZOrder()) {
    view->SetVisible(true);
  }

  point = gfx::Point();
  views::View::ConvertPointToWidget(toolbar->reload_button(), &point);
  EXPECT_NE(HTCAPTION, frame_view->NonClientHitTest(point));
}

IN_PROC_BROWSER_TEST_F(BraveNonClientHitTestHelperBrowserTest,
                       VerticalTabStripRegionView) {
  brave::ToggleVerticalTabStrip(browser());

  auto* browser_view = static_cast<BrowserView*>(browser()->window());
  auto* frame_view = browser_view->browser_widget()->GetFrameView();
  frame_view->DeprecatedLayoutImmediately();

  auto* region_view = BraveBrowserView::From(browser_view)
                          ->vertical_tab_strip_container_view()
                          ->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_TRUE(region_view->GetVisible());

  // Hide all children so the region view's own surface is exposed.
  for (views::View* child : region_view->GetChildrenInZOrder()) {
    child->SetVisible(false);
  }

  gfx::Point point = region_view->GetLocalBounds().CenterPoint();
  views::View::ConvertPointToWidget(region_view, &point);

  // Dragging a window by the empty vertical tab strip region should work.
  EXPECT_EQ(HTCAPTION, frame_view->NonClientHitTest(point));

  // Restore children. A point covered by a direct child view should not be
  // HTCAPTION so that users can interact with the child elements.
  for (views::View* child : region_view->GetChildrenInZOrder()) {
    child->SetVisible(true);
  }

  // The region view only occupies a small portion of the available height
  // (e.g. one tab-height per tab), so the center of the full region view is
  // typically empty space with no child underneath. Find a child with non-zero
  // bounds and verify its center returns non-HTCAPTION. Children's bounds are
  // preserved from the layout run before hiding, so no extra layout step is
  // needed.
  for (views::View* child : region_view->GetChildrenInZOrder()) {
    if (child->bounds().IsEmpty()) {
      continue;
    }
    // Child bounds are in region_view's local coordinate space.
    gfx::Point child_center = child->bounds().CenterPoint();
    views::View::ConvertPointToWidget(region_view, &child_center);
    EXPECT_NE(HTCAPTION, frame_view->NonClientHitTest(child_center));
    break;
  }
}
