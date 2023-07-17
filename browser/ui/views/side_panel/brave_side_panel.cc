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

BraveSidePanel::BraveSidePanel(BrowserView* browser_view,
                               HorizontalAlignment horizontal_alignment)
    : browser_view_(browser_view) {
  SetVisible(false);
  side_panel_width_.Init(
      sidebar::kSidePanelWidth, browser_view_->GetProfile()->GetPrefs(),
      base::BindRepeating(&BraveSidePanel::OnSidePanelWidthChanged,
                          base::Unretained(this)));

  OnSidePanelWidthChanged();
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

void BraveSidePanel::OnSidePanelWidthChanged() {
  SetPanelWidth(side_panel_width_.GetValue());
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

void BraveSidePanel::Layout() {
  if (children().empty()) {
    return;
  }

  // Panel contents is the only child.
  DCHECK_EQ(1UL, children().size());

  if (fixed_contents_width_) {
    gfx::Rect panel_contents_rect(0, 0, *fixed_contents_width_, height());
    panel_contents_rect.Inset(GetInsets());
    children()[0]->SetBoundsRect(panel_contents_rect);
    return;
  }

  children()[0]->SetBoundsRect(GetContentsBounds());
}

void BraveSidePanel::SetPanelWidth(int width) {
  // Only the width is used by BrowserViewLayout.
  SetPreferredSize(gfx::Size(width, 0));
}

void BraveSidePanel::OnResize(int resize_amount, bool done_resizing) {
  if (!starting_width_on_resize_) {
    starting_width_on_resize_ = width();
  }
  int proposed_width = *starting_width_on_resize_ +
                       (IsRightAligned() ? -resize_amount : resize_amount);

  if (done_resizing) {
    starting_width_on_resize_ = absl::nullopt;
    // Before arriving resizing doen event, user could hide sidebar because
    // resizing done event is arrived a little bit later after user stops
    // resizing. And resizing done event is arrived as a result of
    // ResizeArea::OnMouseCaptureLost(). In this situation, just skip below
    // width caching.
    if (!GetVisible()) {
      return;
    }
  }

  const int minimum_width = GetMinimumSize().width();
  if (proposed_width < minimum_width) {
    proposed_width = minimum_width;
  }

  if (width() != proposed_width) {
    SetPanelWidth(proposed_width);
  }

  side_panel_width_.SetValue(proposed_width);
}

void BraveSidePanel::AddHeaderView(std::unique_ptr<views::View> view) {
  // Do nothing.
}

BEGIN_METADATA(BraveSidePanel, views::View)
END_METADATA
