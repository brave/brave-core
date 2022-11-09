// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/brave_side_panel.h"

#include <memory>

#include "base/ranges/algorithm.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/views/border.h"
#include "ui/views/layout/fill_layout.h"

BraveSidePanel::BraveSidePanel(BrowserView* browser_view,
                               HorizontalAlignment horizontal_alignment) {
  SetVisible(false);
  SetLayoutManager(std::make_unique<views::FillLayout>());

  // TODO(pbos): Reconsider if SetPanelWidth() should add borders, if so move
  // accounting for the border into SetPanelWidth(), otherwise remove this TODO.
  constexpr int kDefaultWidth = 320;
  SetPreferredSize(gfx::Size(kDefaultWidth, 1));
  AddObserver(this);
}

BraveSidePanel::~BraveSidePanel() {
  RemoveObserver(this);
}

void BraveSidePanel::SetHorizontalAlignment(HorizontalAlignment alignment) {
  horizontal_alighment_ = alignment;
  UpdateBorder();
}

BraveSidePanel::HorizontalAlignment BraveSidePanel::GetHorizontalAlignment() {
  return horizontal_alighment_;
}

bool BraveSidePanel::IsRightAligned() {
  return horizontal_alighment_ == kHorizontalAlignRight;
}

void BraveSidePanel::UpdateVisibility() {
  const bool any_child_visible = base::ranges::any_of(
      children(), [](const auto* view) { return view->GetVisible(); });
  SetVisible(any_child_visible);
}

void BraveSidePanel::UpdateBorder() {
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    constexpr int kBorderThickness = 1;
    // Negative top border so panel is flush with main tab content
    SetBorder(views::CreateSolidSidedBorder(
        gfx::Insets::TLBR(-1, IsRightAligned() ? kBorderThickness : 0, 0,
                          IsRightAligned() ? 0 : kBorderThickness),
        color_provider->GetColor(kColorToolbarContentAreaSeparator)));
  }
}

void BraveSidePanel::ChildVisibilityChanged(View* child) {
  UpdateVisibility();
}

void BraveSidePanel::OnThemeChanged() {
  View::OnThemeChanged();
  UpdateBorder();
}

void BraveSidePanel::OnChildViewAdded(View* observed_view, View* child) {
  UpdateVisibility();
}

void BraveSidePanel::OnChildViewRemoved(View* observed_view, View* child) {
  UpdateVisibility();
}

void BraveSidePanel::SetPanelWidth(int width) {
  // Only the width is used by BrowserViewLayout.
  SetPreferredSize(gfx::Size(width, 1));
}

void BraveSidePanel::OnResize(int resize_amount, bool done_resizing) {
  // Do Nothing.
}

BEGIN_METADATA(BraveSidePanel, views::View)
END_METADATA
