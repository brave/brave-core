/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_COLOR_UTIL_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_COLOR_UTIL_H_

#include <string_view>

#include "third_party/skia/include/core/SkColor.h"

namespace brave_ads {

bool RgbStringToSkColor(std::string_view rgb, SkColor* color);

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_COLOR_UTIL_H_
