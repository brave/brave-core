/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/window_util.h"

#include "brave/browser/themes/brave_dark_mode_utils.h"

namespace brave_ads {

bool ShouldUseDarkModeTheme() {
  return dark_mode::GetActiveBraveDarkModeType() ==
         dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
}

}  // namespace brave_ads
