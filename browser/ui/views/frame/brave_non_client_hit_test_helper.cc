/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"

#include "base/check_deref.h"
#include "base/check_op.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/hit_test.h"
#include "ui/views/window/hit_test_utils.h"

BraveNonClientHitTestHelper::BraveNonClientHitTestHelper() = default;
BraveNonClientHitTestHelper::~BraveNonClientHitTestHelper() = default;

int BraveNonClientHitTestHelper::NonClientHitTest(
    BrowserView* browser_view,
    const gfx::Point& point_in_widget) {
  if (!browser_view) {
    return HTNOWHERE;
  }

  auto get_hit_test_component = [&](const raw_ptr<views::View>& view) {
    if (!CHECK_DEREF(view).GetVisible()) {
      return HTNOWHERE;
    }

    // The view must live in the same widget as the browser view to make
    // this method work properly. If it has been reparented to a different
    // widget (e.g. overlay_widget_ during macOS immersive fullscreen),
    // GetHitTestComponent() will convert |point_in_widget| using the wrong
    // widget's coordinate space, producing spurious HTCAPTION results.
    // Callers must guard against this before calling NonClientHitTest().
    CHECK_EQ(view->GetWidget(), browser_view->GetWidget());

#if BUILDFLAG(IS_WIN)
    auto hit_test_result = views::GetHitTestComponent(view, point_in_widget);
#else
    HitTestCompat hit_test_result = static_cast<HitTestCompat>(
        views::GetHitTestComponent(view, point_in_widget));
#endif

    if (hit_test_result == HTNOWHERE || hit_test_result == HTCLIENT) {
      // The |point_in_widget| is out of toolbar or on toolbar's sub views.
      return hit_test_result;
    }

    return HTCAPTION;
  };

  // Find if any view returned HTCAPTION
  auto caption_view =
      std::ranges::find_if(observation_.sources(), [&](const auto& view) {
        return get_hit_test_component(view) == HTCAPTION;
      });

  if (caption_view == observation_.sources().end()) {
    return HTNOWHERE;
  }

  // Now we check if HTCAPTION is considered as resize area for the window
  if (!browser_view->CanResize()) {
    return HTCAPTION;
  }

  constexpr int kResizableArea = 8;
  const auto widget_bounds =
      browser_view->GetWidget()->GetRootView()->GetLocalBounds();
  auto non_resizable_area = widget_bounds;
  non_resizable_area.Inset(kResizableArea);
  if (non_resizable_area.Contains(point_in_widget)) {
    return HTCAPTION;
  }

  // Don't need to give resize area when maximized.
  // Having resize area prevents window dragging by grab top border.
  if (browser_view->IsMaximized()) {
    return HTCAPTION;
  }

  // Now we check if the point is which side of resize area.
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

void BraveNonClientHitTestHelper::RegisterCaptionArea(views::View* view) {
  views::SetHitTestComponent(view, HTCAPTION);
  observation_.AddObservation(view);

  for (auto& child : view->children()) {
    OnChildViewAdded(view, child);
  }
}

void BraveNonClientHitTestHelper::OnChildViewAdded(views::View* observed_view,
                                                   views::View* child) {
  views::SetHitTestComponent(child, HTCLIENT);
}

void BraveNonClientHitTestHelper::OnViewIsDeleting(views::View* observed_view) {
  observation_.RemoveObservation(observed_view);
}
