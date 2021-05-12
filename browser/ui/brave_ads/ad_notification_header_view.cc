/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/ad_notification_header_view.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/brave_ads/insets_util.h"
#include "brave/browser/ui/brave_ads/spacer_view.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/flex_layout.h"

namespace brave_ads {

namespace {

// Spacing around each child view
constexpr gfx::Insets kMargin(/* top */ 0,
                              /* left */ 0,
                              /* bottom */ 0,
                              /* right */ 0);

// Spacing between child views and host views
constexpr gfx::Insets kInteriorMargin(/* top */ 0,
                                      /* left */ 10,
                                      /* bottom */ 0,
                                      /* right */ 2);

const int kHeaderViewHeight = 22;

const int kControlButtonsSpacing = 10;

constexpr char kTitleFontName[] = "Roboto";
const gfx::Font::FontStyle kTitleFontStyle = gfx::Font::NORMAL;
const int kTitleFontSize = 13;
const gfx::Font::Weight kTitleFontWeight = gfx::Font::Weight::MEDIUM;
constexpr SkColor kLightModeTitleColor = SkColorSetRGB(0x00, 0x00, 0x00);
constexpr SkColor kDarkModeTitleColor = SkColorSetRGB(0xe3, 0xe6, 0xec);

const gfx::HorizontalAlignment kTitleHorizontalAlignment = gfx::ALIGN_LEFT;
const gfx::VerticalAlignment kTitleVerticalAlignment = gfx::ALIGN_BOTTOM;

constexpr gfx::Insets kTitleBorderInsets(/* top */ 11,
                                         /* left */ 10,
                                         /* bottom */ 3,
                                         /* right */ 0);

}  // namespace

AdNotificationHeaderView::AdNotificationHeaderView(const int width) {
  CreateView(width);
}

AdNotificationHeaderView::~AdNotificationHeaderView() = default;

void AdNotificationHeaderView::SetTitle(const std::u16string& text) {
  DCHECK(title_label_);
  title_label_->SetText(text);

  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

void AdNotificationHeaderView::SetTitleElideBehavior(
    gfx::ElideBehavior elide_behavior) {
  DCHECK(title_label_);
  title_label_->SetElideBehavior(elide_behavior);
}

void AdNotificationHeaderView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kGenericContainer;

  DCHECK(title_label_);
  node_data->SetName(title_label_->GetText());
}

void AdNotificationHeaderView::UpdateContent() {
  UpdateTitleLabel();

  Layout();
  SchedulePaint();
}

void AdNotificationHeaderView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateContent();
}

///////////////////////////////////////////////////////////////////////////////

void AdNotificationHeaderView::CreateView(const int width) {
  views::FlexLayout* layout_manager =
      SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout_manager->SetDefault(views::kMarginsKey, kMargin);
  layout_manager->SetInteriorMargin(kInteriorMargin);
  layout_manager->SetCollapseMargins(true);

  const gfx::Size size(width, kHeaderViewHeight);
  SetPreferredSize(size);

  DCHECK(!title_label_);
  title_label_ = CreateTitleLabel();
  AddChildView(title_label_);

  views::View* control_button_spacing_view =
      CreateFixedSizeSpacerView(kControlButtonsSpacing);
  AddChildView(control_button_spacing_view);

  // Not focusable by default, only for accessibility
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
}

views::Label* AdNotificationHeaderView::CreateTitleLabel() {
  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  views::Label* label = new views::Label();

  const gfx::FontList font_list({kTitleFontName}, kTitleFontStyle,
                                kTitleFontSize, kTitleFontWeight);
  label->SetFontList(font_list);

  label->SetEnabledColor(should_use_dark_colors ? kDarkModeTitleColor
                                                : kLightModeTitleColor);
  label->SetBackgroundColor(SK_ColorTRANSPARENT);

  label->SetHorizontalAlignment(kTitleHorizontalAlignment);
  label->SetVerticalAlignment(kTitleVerticalAlignment);

  const int line_height = font_list.GetHeight();
  label->SetLineHeight(line_height);

  gfx::Insets border_insets = kTitleBorderInsets;
  AdjustInsetsForFontList(&border_insets, font_list);
  label->SetBorder(views::CreateEmptyBorder(border_insets));

  const views::FlexSpecification flex_specification =
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded);
  label->SetProperty(views::kFlexBehaviorKey, flex_specification);

  label->SetHandlesTooltips(false);

  return label;
}

void AdNotificationHeaderView::UpdateTitleLabel() {
  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  DCHECK(title_label_);
  title_label_->SetEnabledColor(should_use_dark_colors ? kDarkModeTitleColor
                                                       : kLightModeTitleColor);
}

BEGIN_METADATA(AdNotificationHeaderView, views::View)
END_METADATA

}  // namespace brave_ads
