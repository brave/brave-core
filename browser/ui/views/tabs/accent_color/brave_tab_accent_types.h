// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_ACCENT_COLOR_BRAVE_TAB_ACCENT_TYPES_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_ACCENT_COLOR_BRAVE_TAB_ACCENT_TYPES_H_

#include "third_party/skia/include/core/SkColor.h"

// Set of colors for painting tab accent (border, background, icon, icon
// border).
struct TabAccentColors {
  SkColor border_color = 0;
  SkColor background_color = 0;
  SkColor icon_color = 0;
  SkColor icon_border_color = 0;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_ACCENT_COLOR_BRAVE_TAB_ACCENT_TYPES_H_
