/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/ad_notification_control_buttons_view.h"

#include <memory>

#include "base/bind.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_ads/ad_notification_view.h"
#include "brave/browser/ui/brave_ads/padded_image_button.h"
#include "brave/browser/ui/brave_ads/padded_image_view.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"

namespace brave_ads {

namespace {

const int kMinimumButtonHeight = 44;

const int kInfoButtonIconDipSize = 40;

constexpr SkColor kLightModeCloseButtonIconColor =
    SkColorSetRGB(0x69, 0x6f, 0x78);
constexpr SkColor kDarkModeCloseButtonIconColor =
    SkColorSetRGB(0xae, 0xb1, 0xc2);
const int kCloseButtonIconDipSize = 16;

}  // namespace

AdNotificationControlButtonsView::AdNotificationControlButtonsView(
    AdNotificationView* ad_notification_view)
    : ad_notification_view_(ad_notification_view) {
  DCHECK(ad_notification_view_);

  CreateView();
}

AdNotificationControlButtonsView::~AdNotificationControlButtonsView() = default;

void AdNotificationControlButtonsView::UpdateContent() {
  UpdateInfoButton();
  UpdateCloseButton();

  Layout();
  SchedulePaint();
}

///////////////////////////////////////////////////////////////////////////////

void AdNotificationControlButtonsView::CreateView() {
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

void AdNotificationControlButtonsView::CreateInfoButton() {
  DCHECK(!info_button_);
  info_button_ = AddChildView(std::make_unique<PaddedImageView>());

  UpdateInfoButton();
}

void AdNotificationControlButtonsView::UpdateInfoButton() {
  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  const gfx::ImageSkia image_skia = gfx::CreateVectorIcon(
      should_use_dark_colors ? kBraveAdsDarkModeInfoButtonIcon
                             : kBraveAdsLightModeInfoButtonIcon,
      kInfoButtonIconDipSize, SK_ColorTRANSPARENT);
  info_button_->SetImage(image_skia);
}

void AdNotificationControlButtonsView::CreateCloseButton() {
  DCHECK(!close_button_);
  close_button_ = AddChildView(std::make_unique<PaddedImageButton>(
      base::BindRepeating(&AdNotificationView::OnCloseButtonPressed,
                          base::Unretained(ad_notification_view_))));

  close_button_->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_BRAVE_ADS_AD_NOTIFICATION_CLOSE_BUTTON));

  UpdateCloseButton();
}

void AdNotificationControlButtonsView::UpdateCloseButton() {
  DCHECK(close_button_);

  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  const gfx::ImageSkia image_skia = gfx::CreateVectorIcon(
      kBraveAdsCloseButtonIcon, kCloseButtonIconDipSize,
      should_use_dark_colors ? kDarkModeCloseButtonIconColor
                             : kLightModeCloseButtonIconColor);
  close_button_->SetImage(views::Button::STATE_NORMAL, image_skia);

  close_button_->AdjustBorderInsetToFitHeight(kMinimumButtonHeight);
}

BEGIN_METADATA(AdNotificationControlButtonsView, views::View)
END_METADATA

}  // namespace brave_ads
