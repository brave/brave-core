// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/page_action/page_action_view.h"

#include <algorithm>

#include "chrome/browser/ui/views/page_action/page_action_model.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/layout/proposed_layout.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/view_class_properties.h"

#define GetMinimumSize GetMinimumSize_Chromium
#define OnNewActiveController OnNewActiveController_Chromium
#define OnPageActionModelChanged OnPageActionModelChanged_Chromium

// Want to use default color even it's expanded.
#define SetUseTonalColorsWhenExpanded(...) SetUseTonalColorsWhenExpanded(false)
#include <chrome/browser/ui/views/page_action/page_action_view.cc>

#undef SetUseTonalColorsWhenExpanded
#undef OnPageActionModelChanged
#undef OnNewActiveController
#undef GetMinimumSize

namespace page_actions {

views::ProposedLayout PageActionView::CalculateProposedLayout(
    const views::SizeBounds& size_bounds) const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetOverrideHeight().has_value()) {
    int override_height = source->GetOverrideHeight().value();
    CHECK_EQ(GetMinimumSize().height(), override_height);

    // Note that we're intentionally ignore |size_bounds| passed from the caller
    // (usually the parent view's decision) so that we can force the height to
    // be the override height. This could be not an optimal solution, but it's
    // the efficient way to achieve the desired behavior.
    auto new_size_bounds = size_bounds;
    new_size_bounds.set_height(override_height);
    auto proposed_layout =
        IconLabelBubbleView::CalculateProposedLayout(new_size_bounds);
    CHECK_EQ(proposed_layout.host_size.height(), override_height);

    return proposed_layout;
  }

  return IconLabelBubbleView::CalculateProposedLayout(size_bounds);
}

void PageActionView::OnPageActionModelVisualRefresh(
    PageActionModelInterface* model) {
  // If model has specified foreground color, we don't dim this view when the
  // widget is inactive.
  if (model && model->GetOverrideForegroundColor().has_value()) {
    SetAppearDisabledInInactiveWidget(false);
  } else {
    SetAppearDisabledInInactiveWidget(
        views::PlatformStyle::kInactiveWidgetControlsAppearDisabled);
  }

  if (GetOverrideHeight()) {
    // When the view have a specified height, we center the view in the
    // container. The default behavior is stretch.
    SetProperty(views::kCrossAxisAlignmentKey, views::LayoutAlignment::kCenter);
  } else {
    ClearProperty(views::kCrossAxisAlignmentKey);
  }
}

gfx::Size PageActionView::GetSizeForLabelWidth(int label_width) const {
  auto size = IconLabelBubbleView::GetSizeForLabelWidth(label_width);
  if (auto override_height = GetOverrideHeight()) {
    size.set_height(*override_height);
  }
  return size;
}

gfx::Size PageActionView::GetMinimumSize() const {
  auto size = GetMinimumSize_Chromium();
  if (auto override_height = GetOverrideHeight()) {
    size.set_height(*override_height);
  }

  if (ShouldAlwaysShowLabel()) {
    size.set_width(
        GetSizeForLabelWidth(label()->GetPreferredSize().width()).width());
  }
  return size;
}

bool PageActionView::ShouldShowLabel() const {
  if (ShouldAlwaysShowLabel()) {
    return true;
  }
  return IconLabelBubbleView::ShouldShowLabel();
}

bool PageActionView::ShouldAlwaysShowLabel() const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetAlwaysShowLabel()) {
    return true;
  }

  return IconLabelBubbleView::ShouldAlwaysShowLabel();
}

SkColor PageActionView::GetBackgroundColor() const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetOverrideBackgroundColor()) {
    return *source->GetOverrideBackgroundColor();
  }

  // Brave doesn't use a background color for page action chips.
  return SK_ColorTRANSPARENT;
}

SkColor PageActionView::GetForegroundColor() const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetOverrideForegroundColor()) {
    return *source->GetOverrideForegroundColor();
  }

  return IconLabelBubbleView::GetForegroundColor();
}

std::optional<int> PageActionView::GetOverrideHeight() const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetOverrideHeight().has_value()) {
    return *source->GetOverrideHeight();
  }
  return std::nullopt;
}

void PageActionView::OnNewActiveController(PageActionController* controller) {
  OnNewActiveController_Chromium(controller);
  OnPageActionModelVisualRefresh(observation_.GetSource());
}

void PageActionView::OnPageActionModelChanged(
    const PageActionModelInterface& model) {
  PageActionView::OnPageActionModelChanged_Chromium(model);

  const PageActionModelInterface* source = observation_.GetSource();
  if (source) {
    // ui::EF_LEFT_MOUSE_BUTTON is the default triggerable event flags of Button
    // class.
    SetTriggerableEventFlags(source->GetOverrideTriggerableEvent().value_or(
        ui::EF_LEFT_MOUSE_BUTTON));
  }
}

}  // namespace page_actions
