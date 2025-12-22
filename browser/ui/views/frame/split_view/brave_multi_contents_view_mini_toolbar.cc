/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_mini_toolbar.h"

#include "base/i18n/rtl.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_split_tab_menu_model.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/frame/top_container_background.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPathBuilder.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/layout/flex_layout.h"

namespace {
constexpr int kMiniToolbarContentPadding = 4;
constexpr int kMiniToolbarOutlineCornerRadius = 8;
}  // namespace

// static
const gfx::VectorIcon& MultiContentsViewMiniToolbar::GetMoreVerticalIcon() {
  return kLeoMoreVerticalIcon;
}

// static
std::unique_ptr<ui::SimpleMenuModel>
MultiContentsViewMiniToolbar::CreateBraveSplitTabMenuModel(
    TabStripModel* tab_strip_model,
    SplitTabMenuModel::MenuSource source,
    int split_tab_index) {
  return std::make_unique<BraveSplitTabMenuModel>(tab_strip_model, source,
                                                  split_tab_index);
}

BraveMultiContentsViewMiniToolbar::~BraveMultiContentsViewMiniToolbar() =
    default;

void BraveMultiContentsViewMiniToolbar::UpdateState(bool is_active,
                                                    bool is_highlighted) {
  MultiContentsViewMiniToolbar::UpdateState(is_active, is_highlighted);

  if (!GetVisible()) {
    return;
  }

  is_active_ = is_active;
  stroke_color_ = is_active_ ? kColorBraveSplitViewActiveWebViewBorder
                             : kColorBraveSplitViewInactiveWebViewBorder;

  const gfx::Insets active_interior_margins = gfx::Insets::TLBR(
      kMiniToolbarOutlineCornerRadius + kMiniToolbarContentPadding,
      kMiniToolbarOutlineCornerRadius + kMiniToolbarContentPadding,
      kMiniToolbarContentPadding, GetOutlineThickness() * 2);

  const gfx::Insets inactive_interior_margins = gfx::Insets::TLBR(
      kMiniToolbarOutlineCornerRadius + kMiniToolbarContentPadding,
      kMiniToolbarOutlineCornerRadius * 2, kMiniToolbarContentPadding,
      GetOutlineThickness());

  static_cast<views::FlexLayout*>(GetLayoutManager())
      ->SetInteriorMargin(is_active ? active_interior_margins
                                    : inactive_interior_margins);
}

void BraveMultiContentsViewMiniToolbar::OnBoundsChanged(
    const gfx::Rect& previous_bounds) {
  // Clip the curved inner side of the mini toolbar.
  SetClipPath(GetPath(/*border_stroke_only=*/false));
}

void BraveMultiContentsViewMiniToolbar::OnPaint(gfx::Canvas* canvas) {
  // Bypassing MultiContentsViewMiniToolbar::OnPaint() and Paint the mini
  // toolbar background to match the toolbar.
  TopContainerBackground::PaintBackground(canvas, this, browser_view_);

  // Draw the bordering stroke.
  cc::PaintFlags flags;
  flags.setStrokeWidth(GetOutlineThickness() * 2);
  flags.setColor(GetColorProvider()->GetColor(stroke_color_));
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);
  SkPath path = GetPath(/*border_stroke_only=*/true);
  canvas->DrawPath(path, flags);
}

SkPath BraveMultiContentsViewMiniToolbar::GetPath(
    bool border_stroke_only) const {
  const gfx::Rect local_bounds = GetLocalBounds();
  SkPathBuilder path;
  path.moveTo(
      0, local_bounds.height() - BraveContentsContainerView::kBorderThickness);
  path.arcTo({kMiniToolbarOutlineCornerRadius, kMiniToolbarOutlineCornerRadius},
             0, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCCW,
             {kMiniToolbarOutlineCornerRadius,
              static_cast<float>(local_bounds.height() -
                                 kMiniToolbarOutlineCornerRadius)});
  path.lineTo(kMiniToolbarOutlineCornerRadius,
              kMiniToolbarOutlineCornerRadius * 2);
  path.arcTo(
      {kMiniToolbarOutlineCornerRadius, kMiniToolbarOutlineCornerRadius},
      270.0f, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
      {kMiniToolbarOutlineCornerRadius * 2, kMiniToolbarOutlineCornerRadius});
  path.lineTo(local_bounds.width() - kMiniToolbarOutlineCornerRadius,
              kMiniToolbarOutlineCornerRadius);
  path.arcTo({kMiniToolbarOutlineCornerRadius, kMiniToolbarOutlineCornerRadius},
             0, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCCW,
             {static_cast<float>(local_bounds.width() -
                                 BraveContentsContainerView::kBorderThickness),
              0});
  if (!border_stroke_only) {
    path.lineTo(local_bounds.width(), 0);
    path.lineTo(local_bounds.width(), local_bounds.height());
    path.lineTo(0, local_bounds.height());
    path.lineTo(0, local_bounds.height() -
                       BraveContentsContainerView::kBorderThickness);
  }
  if (base::i18n::IsRTL()) {
    // Mirror if in RTL.
    gfx::Point center = local_bounds.CenterPoint();
    SkMatrix flip;
    flip.setScale(-1, 1, center.x(), center.y());
    path.transform(flip);
  }
  return path.detach();
}

int BraveMultiContentsViewMiniToolbar::GetOutlineThickness() const {
  return is_active_ ? BraveContentsContainerView::kBorderThickness
                    : BraveContentsContainerView::kBorderThickness / 2;
}

BEGIN_METADATA(BraveMultiContentsViewMiniToolbar)
END_METADATA
