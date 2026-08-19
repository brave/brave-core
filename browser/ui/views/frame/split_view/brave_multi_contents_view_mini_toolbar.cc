/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_mini_toolbar.h"

#include "base/i18n/rtl.h"
#include "brave/browser/ui/brave_scheme_utils.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_split_tab_menu_model.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/themed_background.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPathBuilder.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
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
std::unique_ptr<SplitTabMenuModel>
MultiContentsViewMiniToolbar::CreateBraveSplitTabMenuModel(
    TabStripModel* tab_strip_model,
    SplitTabMenuModel::MenuSource source,
    int split_tab_index) {
  return std::make_unique<BraveSplitTabMenuModel>(tab_strip_model, source,
                                                  split_tab_index);
}

// static
BraveMultiContentsViewMiniToolbar* BraveMultiContentsViewMiniToolbar::From(
    MultiContentsViewMiniToolbar* toolbar) {
  return static_cast<BraveMultiContentsViewMiniToolbar*>(toolbar);
}

BraveMultiContentsViewMiniToolbar::~BraveMultiContentsViewMiniToolbar() =
    default;

void BraveMultiContentsViewMiniToolbar::SetAlwaysShowDomain(
    bool always_show_domain) {
  always_show_domain_ = always_show_domain;
}

void BraveMultiContentsViewMiniToolbar::SetStyle(Style style) {
  style_ = style;
}

void BraveMultiContentsViewMiniToolbar::UpdateContents() {
  MultiContentsViewMiniToolbar::UpdateContents();

  // Display as brave scheme.
  std::u16string domain{domain_label_->GetText()};
  if (brave_utils::ReplaceChromeToBraveScheme(domain)) {
    domain_label_->SetText(domain);
  }
}

void BraveMultiContentsViewMiniToolbar::UpdateState(bool is_active,
                                                    bool is_highlighted) {
  const bool domain_visible = !is_active || always_show_domain_;
  MultiContentsViewMiniToolbar::UpdateState(!domain_visible, is_highlighted);

  if (!GetVisible()) {
    return;
  }

  is_active_ = is_active;

  // The menu button acts on the split that hosts this toolbar.
  close_button_->SetVisible(style_ == Style::kSplit);

  // The leading margin must clear the curved inner side when the favicon is
  // displayed.
  const int leading_margin =
      domain_visible
          ? kMiniToolbarOutlineCornerRadius * 2
          : kMiniToolbarOutlineCornerRadius + kMiniToolbarContentPadding;

  // Without a split border, the trailing child needs its own padding.
  int trailing_margin = kMiniToolbarOutlineCornerRadius;
  if (UsesSplitBorder()) {
    trailing_margin =
        domain_visible ? GetOutlineThickness() : GetOutlineThickness() * 2;
  }

  static_cast<views::FlexLayout*>(GetLayoutManager())
      ->SetInteriorMargin(gfx::Insets::TLBR(
          kMiniToolbarOutlineCornerRadius + kMiniToolbarContentPadding,
          leading_margin, kMiniToolbarContentPadding, trailing_margin));

  UpdateClipPath();
}

void BraveMultiContentsViewMiniToolbar::OnBoundsChanged(
    const gfx::Rect& previous_bounds) {
  UpdateClipPath();
}

void BraveMultiContentsViewMiniToolbar::OnPaint(gfx::Canvas* canvas) {
  // Bypassing MultiContentsViewMiniToolbar::OnPaint() and Paint the mini
  // toolbar background to match the toolbar.
  ThemedBackground::PaintBackground(canvas, this, browser_view_);

  // Draw the bordering stroke.
  cc::PaintFlags flags;
  flags.setStrokeWidth(GetOutlineThickness() * 2);
  flags.setColor(GetColorProvider()->GetColor(GetStrokeColor()));
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);
  SkPath path = GetPath(/*border_stroke_only=*/true);
  canvas->DrawPath(path, flags);
}

bool BraveMultiContentsViewMiniToolbar::UsesSplitBorder() const {
  return style_ != Style::kStandalone;
}

ui::ColorId BraveMultiContentsViewMiniToolbar::GetStrokeColor() const {
  if (!UsesSplitBorder()) {
    return kColorBraveContentsOutline;
  }

  return is_active_ ? kColorBraveSplitViewActiveWebViewBorder
                    : kColorBraveSplitViewInactiveWebViewBorder;
}

int BraveMultiContentsViewMiniToolbar::GetOutlineThickness() const {
  if (!UsesSplitBorder()) {
    return kRoundedCornersContentsOutlineThickness;
  }

  return is_active_ ? BraveContentsContainerView::kBorderThickness
                    : BraveContentsContainerView::kBorderThickness / 2;
}

int BraveMultiContentsViewMiniToolbar::GetContainerBorderThickness() const {
  return UsesSplitBorder() ? BraveContentsContainerView::kBorderThickness
                           : kRoundedCornersContentsOutlineThickness;
}

SkPath BraveMultiContentsViewMiniToolbar::GetPath(
    bool border_stroke_only) const {
  const gfx::Rect local_bounds = GetLocalBounds();
  const int border_thickness = GetContainerBorderThickness();
  SkPathBuilder path;
  path.moveTo(0, local_bounds.height() - border_thickness);
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
             {static_cast<float>(local_bounds.width() - border_thickness), 0});
  if (!border_stroke_only) {
    path.lineTo(local_bounds.width(), 0);
    path.lineTo(local_bounds.width(), local_bounds.height());
    path.lineTo(0, local_bounds.height());
    path.lineTo(0, local_bounds.height() - border_thickness);
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

void BraveMultiContentsViewMiniToolbar::UpdateClipPath() {
  // Clip the curved inner side of the mini toolbar.
  SetClipPath(GetPath(/*border_stroke_only=*/false));
}

BEGIN_METADATA(BraveMultiContentsViewMiniToolbar)
END_METADATA
