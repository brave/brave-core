// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_
#define BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_

#include "base/optional.h"
#include "brave/browser/themes/brave_theme_service.h"
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
  COLOR_WAYBACK_INFOBAR_SEPARATOR = BRAVE_THEME_PROPERTIES_START,
  COLOR_WAYBACK_INFOBAR_SAD_FOLDER,
};

}  // namespace BraveThemeProperties

base::Optional<SkColor> MaybeGetDefaultColorForBraveUi(
    int id, bool incognito, BraveThemeType theme);

#endif  // BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_
