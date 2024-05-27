/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"

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

  const auto children_count = browser_view->toolbar()->children().size();
  int container_view_index = 0;
  if (features::IsChromeRefresh2023()) {
    // Upstream has two more children |background_view_left_| and
    // |background_view_right_| behind the container view.
    DCHECK_EQ(3u, children_count);
    container_view_index = 2;
  } else {
    // All toolbar elements are children of the container view in the toolbar.
    DCHECK_EQ(1u, children_count);
  }

  int hit_test_result = views::GetHitTestComponent(
      browser_view->toolbar()->children()[container_view_index],
      point_in_widget);
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

  // Now we have only resizable areas.
  if (point_in_widget.x() <= kResizableArea &&
      point_in_widget.y() <= kResizableArea) {
    return HTTOPLEFT;
  }

  if (point_in_widget.x() >= (widget_bounds.right() - kResizableArea) &&
      point_in_widget.y() <= kResizableArea) {
    return HTTOPRIGHT;
  }

  if (point_in_widget.x() <= kResizableArea &&
      point_in_widget.y() >= (widget_bounds.bottom() - kResizableArea)) {
    return HTBOTTOMLEFT;
  }

  if (point_in_widget.x() >= (widget_bounds.right() - kResizableArea) &&
      point_in_widget.y() >= (widget_bounds.bottom() - kResizableArea)) {
    return HTBOTTOMRIGHT;
  }

  if (point_in_widget.x() <= kResizableArea) {
    return HTLEFT;
  }

  if (point_in_widget.x() >= (widget_bounds.right() - kResizableArea)) {
    return HTRIGHT;
  }

  if (point_in_widget.y() <= kResizableArea) {
    return HTTOP;
  }

  if (point_in_widget.y() <= (widget_bounds.bottom() - kResizableArea)) {
    return HTBOTTOM;
  }

  NOTREACHED_IN_MIGRATION()
      << "This shouldn't happen. Maybe due to inclusive/exclusive comparison?";
  return hit_test_result;
}

}  // namespace brave
