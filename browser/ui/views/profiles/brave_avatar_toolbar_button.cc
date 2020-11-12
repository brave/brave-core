/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"

void BraveAvatarToolbarButton::SetHighlight(
    const base::string16& highlight_text,
    base::Optional<SkColor> highlight_color) {
  // We only want the icon for Tor and Guest profiles.
  AvatarToolbarButton::SetHighlight((browser_->profile()->IsTor() ||
                                     browser_->profile()->IsGuestSession())
                                        ? base::string16()
                                        : highlight_text,
                                    highlight_color);
}

ui::ImageModel BraveAvatarToolbarButton::GetAvatarIcon(
    ButtonState state,
    const gfx::Image& gaia_account_image) const {
  if (browser_->profile()->IsTor() ||
      browser_->profile()->IsGuestSession()) {
    const int icon_size = ui::TouchUiController::Get()->touch_ui() ? 24 : 20;
    const SkColor icon_color = GetForegroundColor(state);
    return ui::ImageModel::FromVectorIcon(
        browser_->profile()->IsTor() ? kTorProfileIcon : kUserMenuGuestIcon,
        icon_color, icon_size);
  }
  return AvatarToolbarButton::GetAvatarIcon(state, gaia_account_image);
}

base::string16 BraveAvatarToolbarButton::GetAvatarTooltipText() const {
  if (browser_->profile()->IsTor())
    return l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME);

  return AvatarToolbarButton::GetAvatarTooltipText();
}
