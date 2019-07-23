/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_utils.h"

#import <Cocoa/Cocoa.h>

#include "base/mac/sdk_forward_declarations.h"
#include "ui/native_theme/native_theme.h"

bool SystemThemeSupportDarkMode() {
  // Dark mode is supported since Mojave.
  if (@available(macOS 10.14, *))
    return true;
  return false;
}

void SetSystemTheme(BraveThemeType type) {
  if (type == BRAVE_THEME_TYPE_DEFAULT) {
    DCHECK(SystemThemeSupportDarkMode());
    [NSApp setAppearance:nil];
    return;
  }

  if (@available(macOS 10.14, *)) {
    NSAppearanceName new_appearance_name =
        type == BRAVE_THEME_TYPE_DARK ? NSAppearanceNameDarkAqua
                                      : NSAppearanceNameAqua;
    [NSApp setAppearance:[NSAppearance appearanceNamed:new_appearance_name]];
  } else {
    ui::SetDarkMode(type == BraveThemeType::BRAVE_THEME_TYPE_DARK);
    ui::NativeTheme::GetInstanceForNativeUi()->NotifyObservers();
  }
}
