/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/notification_ad_control_buttons_view.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_view.h"
#include "brave/browser/ui/views/brave_ads/padded_image_button.h"
#include "brave/browser/ui/views/brave_ads/padded_image_view.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/layout/box_layout.h"

namespace brave_ads {

namespace {

constexpr int kMinimumButtonHeight = 44;

constexpr int kInfoButtonIconDipSize = 40;

constexpr SkColor kLightModeCloseButtonIconColor =
    SkColorSetRGB(0x69, 0x6f, 0x78);
constexpr SkColor kDarkModeCloseButtonIconColor =
    SkColorSetRGB(0xae, 0xb1, 0xc2);
constexpr int kCloseButtonIconDipSize = 16;

}  // namespace

NotificationAdControlButtonsView::NotificationAdControlButtonsView(
    NotificationAdView& notification_ad_view)
    : notification_ad_view_(notification_ad_view) {
  CreateView();
}

NotificationAdControlButtonsView::~NotificationAdControlButtonsView() = default;

void NotificationAdControlButtonsView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateContent();
}

void NotificationAdControlButtonsView::UpdateContent() {
  UpdateInfoButton();
  UpdateCloseButton();

  Layout();
  SchedulePaint();
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdControlButtonsView::CreateView() {
  views::BoxLayout* box_layout =
      SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));

  box_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);

  // Use layer to change the opacity
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  CreateInfoButton();
  CreateCloseButton();
}

void NotificationAdControlButtonsView::CreateInfoButton() {
  CHECK(!info_button_);
  info_button_ = AddChildView(std::make_unique<PaddedImageView>());
}

void NotificationAdControlButtonsView::UpdateInfoButton() {
  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  const gfx::ImageSkia image_skia = gfx::CreateVectorIcon(
      should_use_dark_colors ? kBraveAdsDarkModeInfoButtonIcon
                             : kBraveAdsLightModeInfoButtonIcon,
      kInfoButtonIconDipSize, SK_ColorTRANSPARENT);
  info_button_->SetImage(image_skia);
}

void NotificationAdControlButtonsView::CreateCloseButton() {
  CHECK(!close_button_);
  close_button_ = AddChildView(std::make_unique<PaddedImageButton>(
      base::BindRepeating(&NotificationAdView::OnCloseButtonPressed,
                          base::Unretained(&*notification_ad_view_))));

  close_button_->SetAccessibleName(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_ADS_NOTIFICATION_AD_CLOSE_BUTTON));
}

void NotificationAdControlButtonsView::UpdateCloseButton() {
  CHECK(close_button_);

  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  const gfx::ImageSkia image_skia = gfx::CreateVectorIcon(
      kBraveAdsCloseButtonIcon, kCloseButtonIconDipSize,
      should_use_dark_colors ? kDarkModeCloseButtonIconColor
                             : kLightModeCloseButtonIconColor);
  close_button_->SetImage(views::Button::STATE_NORMAL, image_skia);

  close_button_->AdjustBorderInsetToFitHeight(kMinimumButtonHeight);
}

BEGIN_METADATA(NotificationAdControlButtonsView, views::View)
END_METADATA

}  // namespace brave_ads
