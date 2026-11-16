/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"

#include <memory>

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "components/infobars/core/infobar.h"
#include "third_party/skia/include/core/SkBlendMode.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/painter.h"
#include "ui/views/view.h"
#include "ui/views/view_utils.h"

namespace {

BraveConfirmInfoBar* AsBraveConfirmInfoBar(const infobars::InfoBar* infobar,
                                           const views::View::Views& children) {
  for (views::View* child : children) {
    auto* brave_confirm_infobar =
        views::AsViewClass<BraveConfirmInfoBar>(child);
    if (brave_confirm_infobar && brave_confirm_infobar == infobar) {
      return brave_confirm_infobar;
    }
  }

  return nullptr;
}

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

}  // namespace

BraveInfoBarContainerView::BraveInfoBarContainerView(
    infobars::InfoBarContainer::Delegate* delegate)
    : InfoBarContainerView(delegate) {
  // Brave draws the infobar/page boundary itself (see UpdateBorder()), so
  // upstream's heavy drop shadow stays hidden for the view's lifetime.
  CHECK(content_shadow_);
  content_shadow_->SetVisible(false);
  UpdateBorder();
}

BraveInfoBarContainerView::~BraveInfoBarContainerView() {
  // Upstream only detaches its infobars from `~InfoBarContainerView()`, where
  // this subclass is already gone and our `PlatformSpecificRemoveInfoBar()` no
  // longer dispatches here - so upstream would cast a BraveConfirmInfoBar to
  // InfoBarView. Detach while this part of `this` is still alive.
  RemoveAllInfoBarsForDestruction();
}

void BraveInfoBarContainerView::PlatformSpecificRemoveInfoBar(
    infobars::InfoBar* infobar) {
  // TODO(https://github.com/brave/brave-browser/issues/61574): Add
  // queued-count pill support for `BraveConfirmInfoBar`.
  BraveConfirmInfoBar* const brave_confirm_infobar =
      AsBraveConfirmInfoBar(infobar, children());
  if (brave_confirm_infobar) {
    RemoveChildView(brave_confirm_infobar);
  } else {
    InfoBarContainerView::PlatformSpecificRemoveInfoBar(infobar);
  }

  // A removal without a closing animation detaches the view without ever
  // recalculating heights, so nothing else tells the browser view to lay the
  // contents out over the room this container no longer needs.
  OnInfoBarStateChanged(/*is_animating=*/false);
}

void BraveInfoBarContainerView::Layout(PassKey) {
  LayoutSuperclass<InfoBarContainerView>(this);

  if (rounded_border_enabled_) {
    // Position infobars within content bounds to accommodate border insets and
    // reserve space for the rounded-rect outline.
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
  if (size.height() == 0) {
    return size;
  }

  if (!rounded_border_enabled_) {
    // Upstream reports just the infobars' height, as it draws the boundary
    // inside its shadow view's own bounds. Reserve the row the bottom line
    // paints in instead.
    size.Enlarge(0, kContentSeparatorHeightDip);
    return size;
  }

  // Reserve extra height for card margins and outline so layout insets don't
  // reduce infobar space.
  size.Enlarge(0, 2 * (kRoundedCornersContentsViewMargin +
                       kContentSeparatorHeightDip + 1));
  return size;
}

void BraveInfoBarContainerView::OnThemeChanged() {
  InfoBarContainerView::OnThemeChanged();

  if (rounded_border_enabled_) {
    // The outline's colors are baked in at creation; rebuild it so it picks up
    // the new theme's colors. The classic bottom line is painted from a color
    // id and needs no rebuild.
    UpdateBorder();
  }
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

  // Upstream's background is what shows through as the separator between
  // stacked infobars; the rounded style keeps the gaps transparent instead.
  SetBackground(enabled
                    ? nullptr
                    : views::CreateSolidBackground(kColorBraveContentsOutline));

  UpdateBorder();
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

void BraveInfoBarContainerView::UpdateBorder() {
  if (!rounded_border_enabled_) {
    // A browser-drawn (unspoofable) line at the infobar/page boundary, in the
    // row CalculatePreferredSize() reserves for it below the last infobar.
    SetBorder(views::CreateSolidSidedBorder(
        gfx::Insets::TLBR(0, 0, kContentSeparatorHeightDip, 0),
        kColorBraveContentsOutline));
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
