/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"

gfx::ImageSkia BraveAvatarToolbarButton::GetAvatarIcon(
    const gfx::Image& gaia_image) const {
  if (brave::IsTorProfile(profile_)) {
    const int icon_size =
      ui::MaterialDesignController::touch_ui() ? 24 : 20;
    const SkColor icon_color =
      GetThemeProvider()->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
    return gfx::CreateVectorIcon(kTorProfileIcon, icon_size, icon_color);
  }
  return AvatarToolbarButton::GetAvatarIcon(gaia_image);
}
