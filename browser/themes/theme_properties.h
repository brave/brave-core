/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_
#define BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_

#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"

namespace BraveThemeProperties {

const SkColor kPrivateColorForTest = SkColorSetRGB(0xFF, 0x00, 0x00);
const SkColor kLightColorForTest = SkColorSetRGB(0xFF, 0xFF, 0xFF);
const SkColor kDarkColorForTest = SkColorSetRGB(0x00, 0x00, 0x00);

enum TestProperty {
  COLOR_FOR_TEST = 9000
};

enum ThemeProperties {
  BRAVE_THEME_PROPERTIES_START = 10000,
  COLOR_BOOKMARK_BAR_INSTRUCTIONS_TEXT = BRAVE_THEME_PROPERTIES_START,
#if BUILDFLAG(ENABLE_SIDEBAR)
  COLOR_SIDEBAR_ADD_BUTTON_DISABLED,
  COLOR_SIDEBAR_BACKGROUND,
  COLOR_SIDEBAR_BUTTON_BASE,
  COLOR_SIDEBAR_BORDER,
  COLOR_SIDEBAR_PANEL_BORDER,
  COLOR_SIDEBAR_ITEM_BACKGROUND,
  COLOR_SIDEBAR_ITEM_DRAG_INDICATOR_COLOR,
  COLOR_SIDEBAR_ADD_BUBBLE_BACKGROUND,
  COLOR_SIDEBAR_ADD_BUBBLE_HEADER_TEXT,
  COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_NORMAL,
  COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_HOVERED,
  COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_BACKGROUND_HOVERED,
  COLOR_SIDEBAR_ARROW_BACKGROUND_HOVERED,
  COLOR_SIDEBAR_ARROW_NORMAL,
  COLOR_SIDEBAR_ARROW_DISABLED,
  COLOR_SIDEBAR_SEPARATOR,
#endif
  BRAVE_THEME_PROPERTIES_LAST = COLOR_SIDEBAR_SEPARATOR,
};

bool IsBraveThemeProperties(int id);

}  // namespace BraveThemeProperties

absl::optional<SkColor> MaybeGetDefaultColorForBraveUi(
    int id,
    bool incognito,
    bool is_tor,
    dark_mode::BraveDarkModeType dark_mode);

#endif  // BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_
