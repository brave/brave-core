/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"

#include "base/check_op.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "ui/base/hit_test.h"
#include "ui/views/window/hit_test_utils.h"

namespace brave {

int NonClientHitTest(BrowserView* browser_view,
                     const gfx::Point& point_in_widget) {
  if (!browser_view->toolbar() || !browser_view->toolbar()->GetVisible()) {
    return HTNOWHERE;
  }

  int hit_test_result =
      views::GetHitTestComponent(browser_view->toolbar(), point_in_widget);
  if (hit_test_result == HTNOWHERE || hit_test_result == HTCLIENT) {
    // The |point_in_widget| is out of toolbar or on toolbar's sub views.
    return hit_test_result;
  }

  DCHECK_EQ(hit_test_result, HTCAPTION);
  // Users are interacting with empty area in toolbar. Check if the area should
  // be resize area for the window.
  if (!browser_view->CanResize()) {
    return hit_test_result;
  }

  constexpr int kResizableArea = 8;
  const auto widget_bounds =
      browser_view->GetWidget()->GetRootView()->GetLocalBounds();
  auto non_resizable_area = widget_bounds;
  non_resizable_area.Inset(kResizableArea);
  if (non_resizable_area.Contains(point_in_widget)) {
    return hit_test_result;
  }

  // Below checking is only for dragging with tab when vertical tab is
  // enabled and title is hidden.
  if (!tabs::utils::ShouldShowBraveVerticalTabs(browser_view->browser())) {
    return HTNOWHERE;
  }

  // Don't need to give resize area when maximized.
  // Having resize area prevents window dragging by grab top border.
  if (browser_view->IsMaximized()) {
    return hit_test_result;
  }

  // Now we have only resizable areas.
  if (point_in_widget.x() <= kResizableArea &&
      point_in_widget.y() <= kResizableArea) {
    return HTTOPLEFT;
  }

  if (point_in_widget.x() >= (widget_bounds.right() - kResizableArea) &&
      point_in_widget.y() <= kResizableArea) {
    return HTTOPRIGHT;
  }

  if (point_in_widget.y() <= kResizableArea) {
    return HTTOP;
  }

  return HTNOWHERE;
}

}  // namespace brave
