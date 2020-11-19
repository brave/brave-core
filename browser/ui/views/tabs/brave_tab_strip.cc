/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "third_party/skia/include/core/SkColor.h"

BraveTabStrip::~BraveTabStrip() = default;

bool BraveTabStrip::ShouldHideCloseButtonForTab(Tab* tab) const {
  bool should_hide = TabStrip::ShouldHideCloseButtonForTab(tab);

  // If upstream logic want to hide, follow it.
  if (should_hide)
    return should_hide;

  if (tab->IsActive())
    return false;

  // Only shows close button on tab when mouse is hovered on tab.
  return !tab->mouse_hovered();
}

SkColor BraveTabStrip::GetTabSeparatorColor() const {
  Profile* profile = controller()->GetProfile();
  if (!brave::IsRegularProfile(profile)) {
    if (profile->IsTor())
      return SkColorSetRGB(0x5A, 0x53, 0x66);

    // For incognito/guest window.
    return SkColorSetRGB(0x3F, 0x32, 0x56);
  }

  // If custom theme is used, follow upstream separator color.
  auto* theme_service = ThemeServiceFactory::GetForProfile(profile);
  if (theme_service->GetThemeSupplier())
    return TabStrip::GetTabSeparatorColor();

  bool dark_mode = dark_mode::GetActiveBraveDarkModeType() ==
                   dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  return dark_mode ? SkColorSetRGB(0x39, 0x38, 0x38)
                   : SkColorSetRGB(0xBE, 0xBF, 0xBF);
}
