/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"

#include <memory>

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "third_party/skia/include/core/SkBlendMode.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/painter.h"
#include "ui/views/view.h"

namespace {

constexpr int kContentSeparatorHeightDip = 1;

std::unique_ptr<views::Border> CreateOutlineBorder(
    const ui::ColorProvider* color_provider,
    const gfx::RoundedCornersF& corner_radii) {
  return views::CreateBorderPainter(
      views::Painter::CreateRoundRectWith1PxBorderPainter(
          color_provider->GetColor(kColorToolbar),
          color_provider->GetColor(kColorBraveContentsOutline), corner_radii,
          SkBlendMode::kSrc,
          /*antialias=*/true,
          /*should_border_scale=*/true),
      gfx::Insets(kRoundedCornersContentsOutlineThickness));
}

// A thin 1px line between infobars and page content.
class ContentSeparator : public views::View {
  METADATA_HEADER(ContentSeparator, views::View)

 public:
  ContentSeparator() {
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
  }

  // views::View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    return gfx::Size(0, kContentSeparatorHeightDip);
  }

  void OnPaint(gfx::Canvas* canvas) override {
    canvas->FillRect(GetLocalBounds(), GetColorProvider()->GetColor(
                                           kColorInfoBarContentAreaSeparator));
  }

  void OnThemeChanged() override {
    views::View::OnThemeChanged();
    // ContentSeparator is a layer, it needs to be manually marked for
    // redraw when the theme changes.
    SchedulePaint();
  }
};

BEGIN_METADATA(ContentSeparator)
END_METADATA

}  // namespace

BraveInfoBarContainerView::BraveInfoBarContainerView(
    infobars::InfoBarContainer::Delegate* delegate)
    : InfoBarContainerView(delegate) {
  // Replace upstream's heavy drop shadow with a thin separator so there's
  // still a browser-drawn (unspoofable) line at the infobar/page boundary.
  DCHECK(content_shadow_);
  RemoveChildViewT(content_shadow_.ExtractAsDangling());
  content_shadow_ = AddChildView(std::make_unique<ContentSeparator>());
}

BraveInfoBarContainerView::~BraveInfoBarContainerView() = default;

void BraveInfoBarContainerView::Layout(PassKey) {
  LayoutSuperclass<InfoBarContainerView>(this);

  if (rounded_border_enabled_) {
    // InfoBarContainerView::Layout() stacks the infobars across this
    // container's raw bounds, with no awareness of SetBorder()'s insets. Move
    // them into the content bounds so the rounded-rect outline painted in
    // OnPaintBorder() has a reserved ring to show in - each infobar paints to
    // its own opaque layer, which composites over whatever the container
    // painted underneath it. Their own heights are kept as-is;
    // CalculatePreferredSize() reserves the extra space this needs.
    const gfx::Rect contents_bounds = GetContentsBounds();
    for (views::View* child : children()) {
      if (child == content_shadow_) {
        continue;
      }
      gfx::Rect bounds = child->bounds();
      bounds.Offset(contents_bounds.OffsetFromOrigin());
      bounds.set_width(contents_bounds.width());
      child->SetBoundsRect(bounds);
    }
  }

  UpdateInfoBarCornerRadii();
}

gfx::Size BraveInfoBarContainerView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = InfoBarContainerView::CalculatePreferredSize(available_size);

  // A zero height is how IsEmpty() reports "no infobars"; never pad that.
  if (!rounded_border_enabled_ || size.height() == 0) {
    return size;
  }

  // The browser view gives this container exactly its preferred height and
  // starts the web contents directly below it, then insets those bounds to
  // float the infobars as a card (see AdjustInfobarLayout()). Ask for the
  // card's margins and outline on top of the infobars' own heights, so that
  // inset doesn't eat into them.
  size.Enlarge(0, 2 * (kRoundedCornersContentsViewMargin +
                       kRoundedCornersContentsOutlineThickness));
  return size;
}

void BraveInfoBarContainerView::OnThemeChanged() {
  InfoBarContainerView::OnThemeChanged();

  if (rounded_border_enabled_) {
    // The outline's colors are baked in at creation; rebuild it so it picks up
    // the new theme's colors.
    UpdateOutlineBorder();
  }
}

void BraveInfoBarContainerView::PlatformSpecificRemoveInfoBar(
    infobars::InfoBar* infobar) {
  InfoBarContainerView::PlatformSpecificRemoveInfoBar(infobar);
  OnInfoBarStateChanged(/*is_animating=*/false);
}

void BraveInfoBarContainerView::SetRoundedBorderEnabled(
    bool enabled,
    const gfx::RoundedCornersF& contents_corner_radii) {
  if (rounded_border_enabled_ == enabled &&
      contents_corner_radii_ == contents_corner_radii) {
    return;
  }

  rounded_border_enabled_ = enabled;
  contents_corner_radii_ = contents_corner_radii;

  // The flat hairline divider is only used in the classic (non-rounded)
  // style; the outline border below takes over marking the boundary when
  // rounded corners are in use.
  DCHECK(content_shadow_);
  content_shadow_->SetVisible(!enabled);

  // Upstream's background is a square-cornered solid fill, which would show
  // through outside the outline's rounded corners, since the outline's own
  // fill is clipped to its path. In the rounded style that fill covers the
  // whole card, so the background isn't needed at all.
  SetBackground(enabled ? nullptr
                        : views::CreateSolidBackground(
                              kColorInfoBarContentAreaSeparator));

  UpdateOutlineBorder();
  UpdateInfoBarCornerRadii();
}

gfx::RoundedCornersF BraveInfoBarContainerView::GetOutlineCornerRadii() const {
  if (contents_corner_radii_.IsEmpty()) {
    return contents_corner_radii_;
  }

  constexpr int kThickness = kRoundedCornersContentsOutlineThickness;
  return {contents_corner_radii_.upper_left() + kThickness,
          contents_corner_radii_.upper_right() + kThickness,
          contents_corner_radii_.lower_right() + kThickness,
          contents_corner_radii_.lower_left() + kThickness};
}

void BraveInfoBarContainerView::UpdateOutlineBorder() {
  if (!rounded_border_enabled_) {
    SetBorder(nullptr);
    return;
  }

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }
  SetBorder(CreateOutlineBorder(color_provider, GetOutlineCornerRadii()));
}

void BraveInfoBarContainerView::UpdateInfoBarCornerRadii() {
  const gfx::RoundedCornersF radii =
      rounded_border_enabled_ ? contents_corner_radii_ : gfx::RoundedCornersF();

  // content_shadow_ is always the last child, so the infobars are the children
  // before it, top to bottom. Only the outer corners of the stack follow the
  // outline; stacked infobars still meet each other squarely.
  DCHECK_EQ(content_shadow_, children().back());
  const size_t infobar_count = children().size() - 1;
  for (size_t i = 0; i < infobar_count; ++i) {
    ui::Layer* layer = children()[i]->layer();
    if (!layer) {
      continue;
    }
    const bool is_first = i == 0;
    const bool is_last = i == infobar_count - 1;
    layer->SetRoundedCornerRadius({is_first ? radii.upper_left() : 0.f,
                                   is_first ? radii.upper_right() : 0.f,
                                   is_last ? radii.lower_right() : 0.f,
                                   is_last ? radii.lower_left() : 0.f});
    layer->SetIsFastRoundedCorner(!layer->rounded_corner_radii().IsEmpty());
  }
}

BEGIN_METADATA(BraveInfoBarContainerView)
END_METADATA
