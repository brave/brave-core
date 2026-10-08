// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/page_action/page_action_view.h"

#include <optional>

#include "chrome/browser/ui/page_action/page_action_model.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/layout/proposed_layout.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/view_class_properties.h"

namespace {

void MaybeOverrideBorder(const page_actions::PageActionModelInterface* source,
                         gfx::Insets& border_insets) {
  if (source && source->GetOverrideBorder().has_value()) {
    border_insets = source->GetOverrideBorder().value();
  }
}

// Brave doesn't use a background color for page action chips: honour the
// model's SkColor override if set, otherwise keep the chip transparent.
std::optional<SkColor> GetBraveBackgroundColor(
    const page_actions::PageActionModelInterface* source) {
  if (source && source->GetOverrideBackgroundColor()) {
    return *source->GetOverrideBackgroundColor();
  }
  return SK_ColorTRANSPARENT;
}

// Honour the model's SkColor override for the chip foreground color, falling
// through to Chromium's own logic (including internal callers such as
// UpdateIconImage()) otherwise.
std::optional<SkColor> GetBraveForegroundColor(
    const page_actions::PageActionModelInterface* source) {
  if (source && source->GetOverrideForegroundColor()) {
    return *source->GetOverrideForegroundColor();
  }
  return std::nullopt;
}

}  // namespace

#include <chrome/browser/ui/views/page_action/page_action_view.cc>

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

  UpdateBorder();
}

gfx::Size PageActionView::GetSizeForLabelWidth(int label_width) const {
  auto size = IconLabelBubbleView::GetSizeForLabelWidth(label_width);
  if (auto override_height = GetOverrideHeight()) {
    size.set_height(*override_height);
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

std::optional<int> PageActionView::GetOverrideHeight() const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetOverrideHeight().has_value()) {
    return *source->GetOverrideHeight();
  }
  return std::nullopt;
}

}  // namespace page_actions
