// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/brave_side_panel.h"

#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_resize_widget.h"
#include "brave/components/sidebar/constants.h"
#include "brave/components/sidebar/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/views/border.h"
#include "ui/views/layout/fill_layout.h"

BraveSidePanel::BraveSidePanel(BrowserView* browser_view,
                               HorizontalAlignment horizontal_alignment)
    : browser_view_(browser_view) {
  SetVisible(false);
  SetLayoutManager(std::make_unique<views::FillLayout>());
  sidebar_width_.Init(
      sidebar::kSidePanelWidth, browser_view_->GetProfile()->GetPrefs(),
      base::BindRepeating(&BraveSidePanel::OnSidebarWidthChanged,
                          base::Unretained(this)));

  OnSidebarWidthChanged();
  AddObserver(this);
}

BraveSidePanel::~BraveSidePanel() {
  RemoveObserver(this);
}

void BraveSidePanel::SetHorizontalAlignment(HorizontalAlignment alignment) {
  horizontal_alignment_ = alignment;
  UpdateBorder();
}

BraveSidePanel::HorizontalAlignment BraveSidePanel::GetHorizontalAlignment() {
  return horizontal_alignment_;
}

bool BraveSidePanel::IsRightAligned() {
  return horizontal_alignment_ == kHorizontalAlignRight;
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

void BraveSidePanel::OnSidebarWidthChanged() {
  SetPanelWidth(sidebar_width_.GetValue());
}

void BraveSidePanel::OnThemeChanged() {
  View::OnThemeChanged();
  UpdateBorder();
}

gfx::Size BraveSidePanel::GetMinimumSize() const {
  // Use default width as a minimum width.
  return gfx::Size(sidebar::kDefaultSidePanelWidth, 0);
}

void BraveSidePanel::AddedToWidget() {
  resize_widget_ = std::make_unique<SidePanelResizeWidget>(
      this, static_cast<BraveBrowserView*>(browser_view_), this);
}

void BraveSidePanel::SetPanelWidth(int width) {
  // Only the width is used by BrowserViewLayout.
  SetPreferredSize(gfx::Size(width, 0));
}

void BraveSidePanel::OnResize(int resize_amount, bool done_resizing) {
  DCHECK(GetVisible());

  if (!starting_width_on_resize_) {
    starting_width_on_resize_ = width();
  }
  int proposed_width = *starting_width_on_resize_ +
                       (IsRightAligned() ? -resize_amount : resize_amount);
  if (done_resizing) {
    starting_width_on_resize_ = absl::nullopt;
  }

  const int minimum_width = GetMinimumSize().width();
  if (proposed_width < minimum_width) {
    proposed_width = minimum_width;
  }

  if (width() != proposed_width) {
    SetPanelWidth(proposed_width);
  }

  sidebar_width_.SetValue(proposed_width);
}

BEGIN_METADATA(BraveSidePanel, views::View)
END_METADATA
