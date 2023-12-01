/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXTENSIONS_ICON_WITH_BADGE_IMAGE_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXTENSIONS_ICON_WITH_BADGE_IMAGE_SOURCE_H_

#include <optional>

namespace brave {
class BraveIconWithBadgeImageSource;
}

#define BRAVE_ICON_WITH_BADGE_IMAGE_SOURCE_H_           \
 private:                                               \
  friend class brave::BraveIconWithBadgeImageSource;    \
  virtual std::optional<int> GetCustomGraphicSize();    \
  virtual std::optional<int> GetCustomGraphicXOffset(); \
  virtual std::optional<int> GetCustomGraphicYOffset(); \
                                                        \
 public:                                                \
  // #define BRAVE_ICON_WITH_BADGE_IMAGE_SOURCE_H_

#define PaintBadge virtual PaintBadge
#define GetIconAreaRect virtual GetIconAreaRect
#include "src/chrome/browser/ui/extensions/icon_with_badge_image_source.h"  // IWYU pragma: export
#undef GetIconAreaRect
#undef PaintBadge
#undef BRAVE_ICON_WITH_BADGE_IMAGE_SOURCE_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXTENSIONS_ICON_WITH_BADGE_IMAGE_SOURCE_H_
