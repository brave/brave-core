/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/location_bar/location_bar_util.h"

#define ConfigureInkDropForRefresh2023 \
  ConfigureInkDropForRefresh2023_ChromiumImpl

#include "src/chrome/browser/ui/views/location_bar/location_bar_util.cc"

#undef ConfigureInkDropForRefresh2023

void ConfigureInkDropForRefresh2023(views::View* const view,
                                    const ChromeColorIds hover_color_id,
                                    const ChromeColorIds ripple_color_id) {
  if (features::IsChromeRefresh2023()) {
    ConfigureInkDropForRefresh2023_ChromiumImpl(view, hover_color_id,
                                                ripple_color_id);
  }
}
