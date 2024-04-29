/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"

#include "brave/browser/ui/color/brave_color_id.h"
#include "chrome/browser/ui/color/chrome_color_provider_utils.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/color_palette.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

SidebarButtonView::SidebarButtonView(const std::u16string& accessible_name) {
  // Locate image at center of the button.
  SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);

  // In order to make use of margin collapsing sets the margin keys.
  // But at the same time, we want the sidebar buttons fill the entire width
  // of sidebar control so that users can easily click buttons by throwing
  // the mouse cursor to the edge.
  SetProperty(views::kMarginsKey, gfx::Insets::VH(kMargin, 0));
  SetHasInkDropActionOnClick(true);
  SetShowInkDropWhenHotTracked(true);

  // Views resulting in focusable nodes later on in the accessibility tree need
  // to have an accessible name for screen readers to see what they are about.
  SetAccessibleName(accessible_name);
}

SidebarButtonView::~SidebarButtonView() = default;

void SidebarButtonView::OnThemeChanged() {
  ImageButton::OnThemeChanged();

  // Apply toolbar button's ink drop config.
  // Reset ink drop config as inkdrop has different config per themes.
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, {});
  ConfigureInkDropForToolbar(this);
  views::InstallRoundRectHighlightPathGenerator(
      this, gfx::Insets::VH(0, kMargin), radii);
}

gfx::Size SidebarButtonView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return {kSidebarButtonSize + kMargin * 2, kSidebarButtonSize};
}

std::u16string SidebarButtonView::GetTooltipText(const gfx::Point& p) const {
  return GetAccessibleName();
}

BEGIN_METADATA(SidebarButtonView)
END_METADATA
