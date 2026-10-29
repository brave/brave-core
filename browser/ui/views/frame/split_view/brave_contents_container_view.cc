/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"

#include <memory>

#include "brave/browser/ui/views/brave_layout_provider.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_outline.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_mini_toolbar.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_container_outline.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "chrome/browser/ui/views/frame/scrim_view.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/border.h"
#include "ui/views/layout/layout_provider.h"

// static
BraveContentsContainerView* BraveContentsContainerView::From(
    ContentsContainerView* view) {
  CHECK(view);
  return static_cast<BraveContentsContainerView*>(view);
}

BraveContentsContainerView::BraveContentsContainerView(
    BrowserView* browser_view,
    bool for_web_panel)
    : ContentsContainerView(browser_view),
      browser_view_(*browser_view),
      for_web_panel_(for_web_panel) {
#if BUILDFLAG(ENABLE_SPEEDREADER)
  // Insert below the group upstream adds last (mini toolbar, outline, capture
  // border): GetChildrenInZOrder() DCHECKs that the capture border stays the
  // highest. The reader toolbar belongs below them anyway, since it insets the
  // contents rather than overlaying it.
  reader_mode_toolbar_ = AddChildViewAt(
      std::make_unique<ReaderModeToolbarView>(browser_view_->browser()),
      GetIndexOf(mini_toolbar()).value());
  reader_mode_toolbar_->SetDelegate(this);
#endif
}

BraveContentsContainerView::~BraveContentsContainerView() = default;

bool BraveContentsContainerView::IsActive() const {
  auto* web_contents = contents_view_->web_contents();
  if (!web_contents) {
    return false;
  }

  return tabs::TabInterface::GetFromContents(web_contents)->IsActivated();
}

void BraveContentsContainerView::SetContentsCornerRadii(
    const gfx::RoundedCornersF& corner_radii) {
  contents_corner_radii_ = corner_radii;
}

void BraveContentsContainerView::UpdateBorderAndOverlay(bool is_in_split,
                                                        bool is_active,
                                                        bool is_highlighted) {
  // Configure the mini toolbar before the base class updates its state.
  using ToolbarStyle = BraveMultiContentsViewMiniToolbar::Style;
  const bool show_domain = ShouldAlwaysShowDomain();
  auto* mini_toolbar = BraveMultiContentsViewMiniToolbar::From(mini_toolbar_);
  mini_toolbar->SetAlwaysShowDomain(show_domain);
  mini_toolbar->SetStyle(for_web_panel_ ? ToolbarStyle::kWebPanel
                         : is_in_split  ? ToolbarStyle::kSplit
                                        : ToolbarStyle::kStandalone);

  // We don't use highlighted state as we're always using thicker border
  // for highlighting active split tab.
  ContentsContainerView::UpdateBorderAndOverlay(is_in_split, is_active,
                                                /*is_highlighted*/ false);

  UpdateBorderRoundedCorners();

  if (!is_in_split && !for_web_panel_) {
    // Outside of a split, the mini toolbar is shown only to display the
    // domain.
    if (show_domain) {
      mini_toolbar_->UpdateState(is_active, /*is_highlighted*/ false);
    } else {
      mini_toolbar_->SetVisible(false);
    }
  } else if (for_web_panel_) {
    // Hide toolbar when active.
    // When web panel is active, there is no UI in toolbar as
    // web panel's toolbar doesn't have menu button.
    mini_toolbar_->UpdateState(IsActive(), /*is_highlighted*/ false);
    mini_toolbar_->SetVisible(!IsActive());
  }

  using OutlineStyle = BraveContentsContainerOutline::Style;
  OutlineStyle style;
  if (is_in_split || for_web_panel_) {
    style =
        is_active ? OutlineStyle::kSplitActive : OutlineStyle::kSplitInactive;
  } else if (!contents_corner_radii_.IsEmpty()) {
    style = OutlineStyle::kStandalone;
  } else if (mini_toolbar_->GetVisible()) {
    // Rounded corners are off, so the contents aren't outlined, but the mini
    // toolbar showing the domain in focus mode still is.
    style = OutlineStyle::kMiniToolbarOnly;
  } else {
    style = OutlineStyle::kHidden;
  }

  // The border only reserves the inset that BraveContentsContainerOutline
  // strokes the ring into.
  SetBorder(views::CreateEmptyBorder(
      BraveContentsContainerOutline::GetBorderThickness(style)));
  BraveContentsContainerOutline::From(container_outline_)
      ->UpdateStyle(style, contents_corner_radii_);
}

void BraveContentsContainerView::UpdateBorderRoundedCorners() {
  contents_view_->layer()->SetRoundedCornerRadius(contents_corner_radii_);
  contents_view_->holder()->SetNativeViewCornerRadii(contents_corner_radii_);
  contents_scrim_view_->SetRoundedCorners(contents_corner_radii_);

  devtools_web_view_->holder()->SetNativeViewCornerRadii(
      contents_corner_radii_);
  devtools_scrim_view_->SetRoundedCorners(contents_corner_radii_);

#if BUILDFLAG(ENABLE_SPEEDREADER)
  if (reader_mode_toolbar_) {
    reader_mode_toolbar_->SetCornerRadius(
        views::LayoutProvider::Get()->GetCornerRadiusMetric(
            ExtendedShapeContextTokens::kRoundedCornersBorderRadius));
  }
#endif
}

views::ProposedLayout BraveContentsContainerView::CalculateProposedLayout(
    const views::SizeBounds& size_bounds) const {
  views::ProposedLayout layouts;
  if (!size_bounds.is_fully_bounded()) {
    return layouts;
  }

  layouts = ContentsContainerView::CalculateProposedLayout(size_bounds);

#if BUILDFLAG(ENABLE_SPEEDREADER)
  auto* contents_layout = layouts.GetLayoutFor(contents_view_);
  if (contents_layout && reader_mode_toolbar_->GetVisible()) {
    gfx::Rect toolbar_bounds = contents_layout->bounds;
    toolbar_bounds.set_height(
        reader_mode_toolbar_->GetPreferredSize().height());
    contents_layout->bounds.Inset(
        gfx::Insets::TLBR(toolbar_bounds.height(), 0, 0, 0));

    layouts.child_layouts.emplace_back(
        reader_mode_toolbar_.get(), /*visible=*/true,
        GetMirroredRect(toolbar_bounds), views::SizeBounds(layouts.host_size));
  } else {
    layouts.child_layouts.emplace_back(
        reader_mode_toolbar_.get(), /*visible=*/false,
        GetMirroredRect(gfx::Rect()), views::SizeBounds(layouts.host_size));
  }
#endif

  return layouts;
}

void BraveContentsContainerView::ChildVisibilityChanged(views::View* child) {
  ContentsContainerView::ChildVisibilityChanged(child);
  InvalidateLayout();
}

#if BUILDFLAG(ENABLE_SPEEDREADER)
void BraveContentsContainerView::OnReaderModeToolbarActivate(
    ReaderModeToolbarView* toolbar) {
  CHECK_EQ(reader_mode_toolbar_, toolbar);
  auto* web_contents = contents_view_->web_contents();
  CHECK(web_contents && web_contents->GetDelegate());
  web_contents->GetDelegate()->ActivateContents(web_contents);
}
#endif

bool BraveContentsContainerView::ShouldAlwaysShowDomain() const {
  if (for_web_panel_ || !IsActive() || IsTabFullscreen()) {
    return false;
  }

  auto* brave_browser_view = BraveBrowserView::From(&browser_view_.get());
  return brave_browser_view &&
         brave_browser_view->show_active_contents_domain_in_mini_toolbar();
}

bool BraveContentsContainerView::IsTabFullscreen() const {
  auto* exclusive_access_manager =
      ExclusiveAccessManager::From(browser_view_->browser());
  return exclusive_access_manager &&
         exclusive_access_manager->fullscreen_controller()->IsTabFullscreen();
}

BEGIN_METADATA(BraveContentsContainerView)
END_METADATA
