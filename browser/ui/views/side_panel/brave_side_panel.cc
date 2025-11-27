// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/brave_side_panel.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_resize_widget.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/view_class_properties.h"

namespace {

// ContentParentView is the parent view for views hosted in the
// side panel.
class ContentParentView : public views::View {
  METADATA_HEADER(ContentParentView, views::View)

 public:
  ContentParentView() {
    SetUseDefaultFillLayout(true);
    SetBackground(views::CreateSolidBackground(kColorSidePanelBackground));
    SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                                 views::MaximumFlexSizeRule::kUnbounded));
  }

  ~ContentParentView() override = default;
};

BEGIN_METADATA(ContentParentView)
END_METADATA

}  // namespace

SidePanel::SidePanel(BrowserView* browser_view,
                     SidePanelEntry::PanelType type,
                     bool has_border,
                     HorizontalAlignment horizontal_alignment)
    : browser_view_(browser_view), type_(type) {
  // If panel has layer by default, adjust its radius whenever
  // updating shadow at UpdateBorder() instead of destroying layer.
  CHECK(!layer());

  scoped_observation_.AddObservation(this);

  SetVisible(false);
  auto* prefs = browser_view_->GetProfile()->GetPrefs();
  if (prefs->FindPreference(sidebar::kSidePanelWidth)) {
    side_panel_width_.Init(
        sidebar::kSidePanelWidth, prefs,
        base::BindRepeating(&SidePanel::OnSidePanelWidthChanged,
                            base::Unretained(this)));
    OnSidePanelWidthChanged();
  } else {
    CHECK_IS_TEST();
  }

  content_parent_view_ = AddChildView(std::make_unique<ContentParentView>());
  content_parent_view_->SetVisible(false);
}

SidePanel::~SidePanel() {
  scoped_observation_.RemoveObservation(this);
}

void SidePanel::UpdateWidthOnEntryChanged() {
  // Do nothing.
}

bool SidePanel::ShouldRestrictMaxWidth() const {
  return false;
}

void SidePanel::SetHorizontalAlignment(HorizontalAlignment alignment) {
  horizontal_alignment_ = alignment;
  UpdateBorder();
}

SidePanel::HorizontalAlignment SidePanel::GetHorizontalAlignment() {
  return horizontal_alignment_;
}

bool SidePanel::IsRightAligned() {
  return horizontal_alignment_ == HorizontalAlignment::kRight;
}

void SidePanel::UpdateBorder() {
  // Border and shadow should be updated together when rounded corner enabled
  // condition is changed.
  if (BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser_view_->browser())) {
    // Use a negative top border to hide the separator inserted by the upstream
    // side panel implementation.
    SetBorder(views::CreateEmptyBorder(gfx::Insets::TLBR(-1, 0, 0, 0)));

    shadow_ = BraveContentsViewUtil::CreateShadow(this);
    SetBackground(
        views::CreateSolidBackground(kColorSidebarPanelHeaderBackground));

    return;
  }

  if (shadow_) {
    shadow_.reset();
    DestroyLayer();
  }

  SetBackground(nullptr);

  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    constexpr int kBorderThickness = 1;
    // Negative top border so panel is flush with main tab content
    SetBorder(views::CreateSolidSidedBorder(
        gfx::Insets::TLBR(-1, IsRightAligned() ? kBorderThickness : 0, 0,
                          IsRightAligned() ? 0 : kBorderThickness),
        color_provider->GetColor(kColorToolbarContentAreaSeparator)));
  }
}

void SidePanel::OnSidePanelWidthChanged() {
  SetPanelWidth(side_panel_width_.GetValue());
}

void SidePanel::OnThemeChanged() {
  View::OnThemeChanged();
  UpdateBorder();
}

gfx::Size SidePanel::GetMinimumSize() const {
  // Use default width as a minimum width.
  return gfx::Size(sidebar::kDefaultSidePanelWidth, 0);
}

bool SidePanel::IsClosing() {
  return false;
}

void SidePanel::AddedToWidget() {
  resize_widget_ = std::make_unique<SidePanelResizeWidget>(
      this, static_cast<BraveBrowserView*>(browser_view_), this);
}

void SidePanel::Layout(PassKey) {
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

double SidePanel::GetAnimationValue() const {
  return 1;
}

void SidePanel::SetPanelWidth(int width) {
  // Only the width is used by BrowserViewLayout.
  SetPreferredSize(gfx::Size(width, 0));
}

void SidePanel::OnResize(int resize_amount, bool done_resizing) {
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

void SidePanel::AddHeaderView(std::unique_ptr<views::View> view) {}

void SidePanel::RemoveHeaderView() {}

void SidePanel::SetOutlineVisibility(bool visible) {}

void SidePanel::OnChildViewAdded(View* observed_view, View* child) {
  if (observed_view != this) {
    return;
  }
  if (!scoped_observation_.IsObservingSource(child)) {
    scoped_observation_.AddObservation(child);
  }
}

void SidePanel::OnChildViewRemoved(View* observed_view, View* child) {
  if (observed_view != this) {
    return;
  }
  if (scoped_observation_.IsObservingSource(child)) {
    scoped_observation_.RemoveObservation(child);
  }
}

void SidePanel::Open(bool animated) {
  UpdateVisibility(/*should_be_open=*/true);
}

void SidePanel::Close(bool animated) {
  UpdateVisibility(/*should_be_open=*/false);
}

void SidePanel::UpdateVisibility(bool should_be_open) {
  state_ = should_be_open ? State::kOpen : State::kClosed;
  SetVisible(should_be_open);
}

views::View* SidePanel::GetContentParentView() {
  return content_parent_view_;
}

BEGIN_METADATA(SidePanel)
END_METADATA
