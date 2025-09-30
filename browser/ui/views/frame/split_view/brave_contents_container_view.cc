/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"

#include <memory>

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_mini_toolbar.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
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
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/border.h"

namespace {

// We dont' let this outline visible always but it sets mini toolbar's clip path
// even it's hidden. We set mini toolbar's clip path from
// BraveMultiContentsViewMiniToolbar.
class BraveContentsContainerOutline : public ContentsContainerOutline {
  METADATA_HEADER(BraveContentsContainerOutline, ContentsContainerOutline)
 public:
  using ContentsContainerOutline::ContentsContainerOutline;
  ~BraveContentsContainerOutline() override = default;

  // ContentsContainerOutline:
  void OnViewBoundsChanged(views::View* observed_view) override {
    // Ignore chromium's mini toolbar path clipping.
  }
};

BEGIN_METADATA(BraveContentsContainerOutline)
END_METADATA

}  // namespace

// static
BraveContentsContainerView* BraveContentsContainerView::From(
    ContentsContainerView* view) {
  CHECK(view);
  return static_cast<BraveContentsContainerView*>(view);
}

BraveContentsContainerView::BraveContentsContainerView(
    BrowserView* browser_view)
    : ContentsContainerView(browser_view), browser_view_(*browser_view) {
#if BUILDFLAG(ENABLE_SPEEDREADER)
  auto* browser = browser_view_->browser();
  reader_mode_toolbar_ =
      AddChildView(std::make_unique<ReaderModeToolbarView>(browser->profile()));
  reader_mode_toolbar_->SetDelegate(this);
#endif

  if (base::FeatureList::IsEnabled(features::kSideBySide)) {
    // To prevent |mini_toolbar_| becomes dangling pointer.
    {
      auto old_toolbar = RemoveChildViewT(mini_toolbar_);
      mini_toolbar_ = nullptr;
      auto old_outline = RemoveChildViewT(container_outline_);
      container_outline_ = nullptr;
    }
    mini_toolbar_ =
        AddChildView(std::make_unique<BraveMultiContentsViewMiniToolbar>(
            browser_view, contents_view_));
    container_outline_ = AddChildView(
        std::make_unique<BraveContentsContainerOutline>(mini_toolbar_));
  }
}

BraveContentsContainerView::~BraveContentsContainerView() = default;

void BraveContentsContainerView::UpdateBorderAndOverlay(bool is_in_split,
                                                        bool is_active,
                                                        bool is_highlighted) {
  // We don't use highlighted state as we're always using thicker border
  // for highlighting active split tab.
  ContentsContainerView::UpdateBorderAndOverlay(is_in_split, is_active,
                                                /*is_highlighted*/ false);

  // Don't draw any borders from outline.
  container_outline_->SetVisible(false);
  UpdateBorderRoundedCorners();

  if (!is_in_split) {
    return;
  }

  // Draw active/inactive outlines around the contents areas and updates mini
  // toolbar visibility.
  const auto border_corner_radius(GetCornerRadius(true));
  if (is_active) {
    SetBorder(views::CreateBorderPainter(
        views::Painter::CreateSolidRoundRectPainterWithVariableRadius(
            GetColorProvider()->GetColor(
                kColorBraveSplitViewActiveWebViewBorder),
            border_corner_radius, gfx::Insets(), SkBlendMode::kSrc),
        gfx::Insets(kBorderThickness)));
  } else {
    SetBorder(views::CreateBorderPainter(
        views::Painter::CreateRoundRectWith1PxBorderPainter(
            GetColorProvider()->GetColor(
                kColorBraveSplitViewInactiveWebViewBorder),
            GetColorProvider()->GetColor(kColorToolbar),
            gfx::RoundedCornersF(border_corner_radius), SkBlendMode::kSrc,
            /*anti_alias*/ true,
            /*should_border_scale*/ true),
        gfx::Insets(kBorderThickness)));
  }
}

void BraveContentsContainerView::UpdateBorderRoundedCorners() {
  const auto contents_corner_radius(GetCornerRadius(false));

  contents_view_->layer()->SetRoundedCornerRadius(contents_corner_radius);
  contents_view_->holder()->SetCornerRadii(contents_corner_radius);
  contents_scrim_view_->SetRoundedCorners(contents_corner_radius);

  devtools_web_view_->holder()->SetCornerRadii(contents_corner_radius);
  devtools_scrim_view_->SetRoundedCorners(contents_corner_radius);

#if BUILDFLAG(ENABLE_SPEEDREADER)
  if (reader_mode_toolbar_) {
    reader_mode_toolbar_->SetCornerRadius(
        BraveContentsViewUtil::GetBorderRadius());
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

gfx::RoundedCornersF BraveContentsContainerView::GetCornerRadius(
    bool for_border) const {
  auto* exclusive_access_manager =
      browser_view_->browser()->GetFeatures().exclusive_access_manager();
  if (exclusive_access_manager &&
      exclusive_access_manager->fullscreen_controller()->IsTabFullscreen()) {
    return {};
  }

  if (!BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser_view_->browser())) {
    return {};
  }

  tabs::TabInterface* tab = nullptr;
  if (is_in_split_ && contents_view_->web_contents()) {
    tab = tabs::TabInterface::GetFromContents(contents_view_->web_contents());
  }

  auto rounded_corners =
      BraveContentsViewUtil::GetRoundedCornersForContentsView(
          browser_view_->browser(), tab);
  if (for_border) {
    return {rounded_corners.upper_left() + kBorderThickness,
            rounded_corners.upper_right() + kBorderThickness,
            rounded_corners.lower_right() + kBorderThickness,
            rounded_corners.lower_left() + kBorderThickness};
  }

  return rounded_corners;
}

BEGIN_METADATA(BraveContentsContainerView)
END_METADATA
