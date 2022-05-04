/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_UTILS_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_UTILS_H_

#include "third_party/skia/include/core/SkColor.h"

SkColor GetLocationBarBackground(bool dark, bool priv, bool hover);
SkColor GetOmniboxResultBackground(int id, bool dark, bool priv);

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_HELPER_UTILS_H_
