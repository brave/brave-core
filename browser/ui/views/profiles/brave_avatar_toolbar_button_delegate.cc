/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button_delegate.h"

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_provider.h"

BraveAvatarToolbarButtonDelegate::BraveAvatarToolbarButtonDelegate(
    AvatarToolbarButton* button,
    Browser* browser)
    : AvatarToolbarButtonDelegate(button, browser), browser_(browser) {}

gfx::Image BraveAvatarToolbarButtonDelegate::GetGaiaAccountImage() const {
  return gfx::Image();
}

ui::ImageModel BraveAvatarToolbarButtonDelegate::GetAvatarIcon(
    int icon_size,
    SkColor icon_color) const {
  const auto location_bar_icon_size = GetLayoutConstant(LOCATION_BAR_ICON_SIZE);
  if (browser_->profile()->IsTor()) {
    return ui::ImageModel::FromVectorIcon(kLeoProductTorIcon,
                                          SkColorSetRGB(0x3C, 0x82, 0x3C),
                                          location_bar_icon_size);
  }

  if (browser_->profile()->IsIncognitoProfile()) {
    return ui::ImageModel::FromVectorIcon(
        kIncognitoIcon, SkColorSetRGB(0xFF, 0xFF, 0xFF), icon_size);
  }

  if (browser_->profile()->IsGuestSession()) {
    return ui::ImageModel::FromVectorIcon(kUserMenuGuestIcon, icon_color,
                                          location_bar_icon_size);
  }

  return AvatarToolbarButtonDelegate::GetAvatarIcon(location_bar_icon_size,
                                                    icon_color);
}

std::u16string BraveAvatarToolbarButtonDelegate::GetAvatarTooltipText() const {
  if (browser_->profile()->IsTor()) {
    return brave_l10n::GetLocalizedResourceUTF16String(
        IDS_TOR_AVATAR_BUTTON_TOOLTIP_TEXT);
  }

  return AvatarToolbarButtonDelegate::GetAvatarTooltipText();
}
