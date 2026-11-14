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
#include "ui/gfx/canvas.h"
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

void BraveInfoBarContainerView::PlatformSpecificRemoveInfoBar(
    infobars::InfoBar* infobar) {
  OnInfoBarStateChanged(/*is_animating=*/false);
  // TODO(https://github.com/brave/brave-browser/issues/61574): Add
  // queued-count pill support for `BraveConfirmInfoBar`.
  BraveConfirmInfoBar* const brave_confirm_infobar =
      AsBraveConfirmInfoBar(infobar, children());
  if (brave_confirm_infobar) {
    return RemoveChildView(brave_confirm_infobar);
  }

  InfoBarContainerView::PlatformSpecificRemoveInfoBar(infobar);
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
    // Upstream lays `content_shadow_` out below the height it reports here, so
    // it paints outside these bounds, over whatever the browser view starts
    // below the container. Reserve the divider's row so it stays inside.
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
    // the new theme's colors.
    UpdateOutlineBorder();
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

  // Use the flat hairline divider only for classic style, and the outline
  // border for rounded corners.
  DCHECK(content_shadow_);
  content_shadow_->SetVisible(!enabled);

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
