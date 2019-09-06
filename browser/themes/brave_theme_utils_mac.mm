/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_utils.h"

#import <Cocoa/Cocoa.h>

#include "base/mac/sdk_forward_declarations.h"
#include "brave/browser/themes/brave_theme_utils_internal.h"

void SetSystemTheme(BraveThemeType type) {
  if (type == BRAVE_THEME_TYPE_DEFAULT) {
    DCHECK(
        ui::NativeTheme::GetInstanceForNativeUi()->SystemDarkModeSupported());
    [NSApp setAppearance:nil];
    return;
  }

  if (@available(macOS 10.14, *)) {
    NSAppearanceName new_appearance_name =
        type == BRAVE_THEME_TYPE_DARK ? NSAppearanceNameDarkAqua
                                      : NSAppearanceNameAqua;
    [NSApp setAppearance:[NSAppearance appearanceNamed:new_appearance_name]];
  } else {
    internal::SetSystemThemeForNonDarkModePlatform(type);
  }
}
