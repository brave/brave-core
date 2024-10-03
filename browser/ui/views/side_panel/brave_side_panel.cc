// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/brave_side_panel.h"

#include <optional>
#include <utility>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_resize_widget.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/views/background.h"
#include "ui/views/border.h"

BraveSidePanel::BraveSidePanel(BrowserView* browser_view,
                               HorizontalAlignment horizontal_alignment)
    : browser_view_(browser_view) {
  scoped_observation_.AddObservation(this);

  SetVisible(false);
  auto* prefs = browser_view_->GetProfile()->GetPrefs();
  if (prefs->FindPreference(sidebar::kSidePanelWidth)) {
    side_panel_width_.Init(
        sidebar::kSidePanelWidth, prefs,
        base::BindRepeating(&BraveSidePanel::OnSidePanelWidthChanged,
                            base::Unretained(this)));
    OnSidePanelWidthChanged();
  } else {
    CHECK_IS_TEST();
  }

  if (BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
          browser_view_->browser())) {
    shadow_ = BraveContentsViewUtil::CreateShadow(this);
    SetBackground(
        views::CreateThemedSolidBackground(kColorSidebarPanelHeaderBackground));
  }
}

BraveSidePanel::~BraveSidePanel() {
  scoped_observation_.RemoveObservation(this);
}

void BraveSidePanel::SetHorizontalAlignment(HorizontalAlignment alignment) {
  horizontal_alignment_ = alignment;
  UpdateBorder();
}

BraveSidePanel::HorizontalAlignment BraveSidePanel::GetHorizontalAlignment() {
  return horizontal_alignment_;
}

bool BraveSidePanel::IsRightAligned() {
  return horizontal_alignment_ == kAlignRight;
}

void BraveSidePanel::UpdateBorder() {
  if (BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
          browser_view_->browser())) {
    // Use a negative top border to hide the separator inserted by the upstream
    // side panel implementation.
    SetBorder(views::CreateEmptyBorder(gfx::Insets::TLBR(-1, 0, 0, 0)));
    return;
  }

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

bool BraveSidePanel::IsClosing() {
  return false;
}

void BraveSidePanel::AddedToWidget() {
  resize_widget_ = std::make_unique<SidePanelResizeWidget>(
      this, static_cast<BraveBrowserView*>(browser_view_), this);
}

void BraveSidePanel::Layout(PassKey) {
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

double BraveSidePanel::GetAnimationValue() const {
  return 1;
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
    starting_width_on_resize_ = std::nullopt;
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
  // Need to keep here because SidePanelCoordinator referes this |view|'s
  // child view(header_combobox_). We don't use this |header_view_|.
  // So just keep it here.
  header_view_ = std::move(view);
}

void BraveSidePanel::OnChildViewAdded(View* observed_view, View* child) {
  if (observed_view != this) {
    return;
  }
  if (!scoped_observation_.IsObservingSource(child)) {
    scoped_observation_.AddObservation(child);
  }
}

void BraveSidePanel::OnChildViewRemoved(View* observed_view, View* child) {
  if (observed_view != this) {
    return;
  }
  if (scoped_observation_.IsObservingSource(child)) {
    scoped_observation_.RemoveObservation(child);
  }
}

void BraveSidePanel::OnViewPropertyChanged(View* observed_view,
                                           const void* key,
                                           int64_t old_value) {
  if (key == kSidePanelContentStateKey) {
    SidePanelContentState new_value = static_cast<SidePanelContentState>(
        observed_view->GetProperty(kSidePanelContentStateKey));
    if (new_value != static_cast<SidePanelContentState>(old_value)) {
      SetVisible(new_value == SidePanelContentState::kReadyToShow ||
                 new_value == SidePanelContentState::kShowImmediately);
    }
  }
}

BEGIN_METADATA(BraveSidePanel)
END_METADATA
