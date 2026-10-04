// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/side_panel/side_panel.h"

#include "base/i18n/rtl.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/side_panel/side_panel_utils.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/border.h"
#include "ui/views/painter.h"

// Rename upstream methods so we can provide thin wrappers that reapply
// rounded-corner and border state.
#define AddHeaderView AddHeaderView_ChromiumImpl
#define RemoveHeaderView RemoveHeaderView_ChromiumImpl

#include <chrome/browser/ui/views/side_panel/side_panel.cc>

#undef RemoveHeaderView
#undef AddHeaderView

namespace {

// Applies the current rounded-corner values to every direct child of the
// content parent view. Used when the pref changes or the panel opens with
// existing content (so OnChildViewAdded never fired with the new values).
void UpdateContentWrapperChildCorners(views::View* content_parent_view,
                                      BrowserView* browser_view) {
  // ContentParentView hosts multiple content view and shows at once.
  CHECK(content_parent_view->GetUseDefaultFillLayout());

  auto corners = brave::GetPanelContentsRoundedCorners(browser_view);
  for (views::View* child : content_parent_view->children()) {
    // If the child is a WebView or paints to a layer, round its corners.
    if (views::IsViewClass<views::WebView>(child)) {
      views::AsViewClass<views::WebView>(child)
          ->holder()
          ->SetNativeViewCornerRadii(corners);
    }
    // Try to detect if the child is a views::View wrapper of a WebView. If so,
    // round its corners.
    if (child->children().size() == 1 &&
        views::IsViewClass<views::WebView>(child->children()[0])) {
      views::AsViewClass<views::WebView>(child->children()[0])
          ->holder()
          ->SetNativeViewCornerRadii(corners);
    }
    if (child->layer()) {
      child->layer()->SetIsFastRoundedCorner(true);
      child->layer()->SetRoundedCornerRadius(corners);
    }
  }
}

// Paints a 1px rounded outline that wraps the header (if any) and content
// together.
//
// Why this needs two separate Insets (|layout_insets_| for GetInsets(),
// |outline_insets_| for Paint()) instead of one: `header_view_` has no layout
// slot of its own. BraveSidePanelHeader::Layout()
// (brave/browser/ui/views/side_panel/brave_side_panel_header.cc) derives its
// bounds purely by reading `parent()->GetContentsBounds()` -- i.e. this
// SidePanel's content rect, which is `GetLocalBounds().Inset(GetInsets())`
// (ui/views/view.cc) -- and placing itself directly *above* that:
// `y = contents_bounds.y() - header_height`. So the top inset returned by
// GetInsets() does double duty: it reserves `header_height` rows for the
// header to back-fill by positioning itself within them, *plus* one more
// outline gap so there's room left over above the header for the ring. The
// ring itself should wrap the header, not sit below it, so its own top needs
// just the gap -- not `header_height` again. That's the entire reason the two
// Insets diverge, and only on top: the other three sides are identical
// because nothing else derives its position from GetContentsBounds() and
// "borrows" part of the reserved margin the way the header does.
class SidePanelOutlineBorder : public views::Border {
 public:
  SidePanelOutlineBorder(const gfx::Insets& layout_insets,
                         const gfx::Insets& outline_insets,
                         const gfx::RoundedCornersF& shape_corner_radii,
                         const ui::ColorProvider* color_provider)
      : layout_insets_(layout_insets),
        outline_insets_(outline_insets),
        outline_corner_radii_(shape_corner_radii.upper_left() +
                                  kRoundedCornersContentsOutlineThickness,
                              shape_corner_radii.upper_right() +
                                  kRoundedCornersContentsOutlineThickness,
                              shape_corner_radii.lower_right() +
                                  kRoundedCornersContentsOutlineThickness,
                              shape_corner_radii.lower_left() +
                                  kRoundedCornersContentsOutlineThickness) {
    RebuildPainter(color_provider);
  }
  SidePanelOutlineBorder(const SidePanelOutlineBorder&) = delete;
  SidePanelOutlineBorder& operator=(const SidePanelOutlineBorder&) = delete;
  ~SidePanelOutlineBorder() override = default;

  // views::Border:
  void Paint(const views::View& view, gfx::Canvas* canvas) override {
    if (!painter_) {
      return;
    }
    gfx::Rect outline_bounds = view.GetLocalBounds();
    outline_bounds.Inset(outline_insets_ -
                         gfx::Insets(kRoundedCornersContentsOutlineThickness));
    views::Painter::PaintPainterAt(canvas, painter_.get(), outline_bounds);
  }
  gfx::Insets GetInsets() const override { return layout_insets_; }
  gfx::Size GetMinimumSize() const override { return gfx::Size(); }
  void OnViewThemeChanged(views::View* view) override {
    RebuildPainter(view->GetColorProvider());
  }

 private:
  void RebuildPainter(const ui::ColorProvider* color_provider) {
    if (!color_provider) {
      return;
    }
    painter_ = views::Painter::CreateRoundRectWith1PxBorderPainter(
        color_provider->GetColor(kColorToolbar),
        color_provider->GetColor(kColorBraveContentsOutline),
        outline_corner_radii_, SkBlendMode::kSrc, /*antialias=*/true,
        /*should_border_scale=*/true);
  }

  const gfx::Insets layout_insets_;
  const gfx::Insets outline_insets_;
  const gfx::RoundedCornersF outline_corner_radii_;
  std::unique_ptr<views::Painter> painter_;
};

}  // namespace

void SidePanel::SetResizeArea(std::unique_ptr<views::View> resize_area) {
  CHECK(resize_area);
  auto old_resize_area = RemoveChildViewT(resize_area_);
  // Add at the end so the resize area is above content_parent_view_ in
  // z-order. The strip always overlaps the content edge and must win the
  // hit-test to receive drag events.
  resize_area_ = AddChildView(std::move(resize_area));
  resize_area_->InsertBeforeInFocusList(content_parent_view_);
}

void SidePanel::SetRoundedBorderEnabled(bool enabled) {
  if (rounded_border_enabled_ == enabled) {
    return;
  }

  rounded_border_enabled_ = enabled;

  if (GetVisible()) {
    UpdateBorder();
  }
}

void SidePanel::UpdateBorder() {
  // To make sure |horizontal_alignemnt_| refreshed before using
  // IsRightAligned().
  UpdateHorizontalAlignment();

  // Re-apply corners to existing content: the pref or header state changed.
  UpdateContentWrapperChildCorners(GetContentParentView(), browser_view_);

  const int header_top_inset =
      header_view_ ? header_view_->GetPreferredSize().height() : 0;
  gfx::Insets insets;
  insets.set_top(header_top_inset);

  // With rounded corners the content view owns its own margins, so only add an
  // outer-side gap for visual separation from the window chrome.
  const bool is_sidebar_leading = (IsRightAligned() == base::i18n::IsRTL());
  if (rounded_border_enabled_) {
    // Include outline thickness on window-facing edges to align with main
    // content.
    constexpr int kWindowEdgeMargin = kRoundedCornersContentsViewMargin +
                                      kRoundedCornersContentsOutlineThickness;

    // Reserve an extra |kRoundedCornersContentsOutlineThickness| on the top and
    // inner-side edges -- where the header/content would otherwise be flush
    // with the panel edge
    // -- so the outline painted below this has room to show on all four
    // sides.
    insets.set_top(insets.top() + kRoundedCornersContentsOutlineThickness);
    insets.set_bottom(kWindowEdgeMargin);
    if (is_sidebar_leading) {
      insets.set_left(kWindowEdgeMargin);
      insets.set_right(kRoundedCornersContentsOutlineThickness);
    } else {
      insets.set_right(kWindowEdgeMargin);
      insets.set_left(kRoundedCornersContentsOutlineThickness);
    }

    // Unlike |insets| (used for content layout), the outline itself should
    // wrap the header and content together as one shape, so its top inset
    // never includes the header's height -- only the slim outline gap. The
    // other three sides are copied unchanged from |insets|: only the top edge
    // has a header "borrowing" part of the reserved space (see the
    // SidePanelOutlineBorder class comment above for why).
    gfx::Insets outline_insets = insets;
    outline_insets.set_top(kRoundedCornersContentsOutlineThickness);

    ui::ColorProvider* color_provider = GetColorProvider();
    CHECK(color_provider);
    SetBorder(std::make_unique<SidePanelOutlineBorder>(
        insets, outline_insets,
        brave::GetPanelContentsRoundedCorners(browser_view_,
                                              /*flatten_top_for_header=*/false),
        color_provider));
    return;
  }

  // Without rounded corners, draw a 1px vertical separator between content and
  // panel.
  constexpr int kBorderThickness = 1;
  if (is_sidebar_leading) {
    insets.set_right(kBorderThickness);
  } else {
    insets.set_left(kBorderThickness);
  }
  SetBorder(
      views::CreateSolidSidedBorder(insets, kColorToolbarContentAreaSeparator));
}

void SidePanel::AddHeaderView(std::unique_ptr<views::View> view) {
  AddHeaderView_ChromiumImpl(std::move(view));
  UpdateBorder();

  // The resize area overlaps other views (contents, header), so it must be
  // top-most to receive drag events.
  ReorderChildView(resize_area_, children().size());
}

void SidePanel::RemoveHeaderView() {
  RemoveHeaderView_ChromiumImpl();
  UpdateBorder();
}
