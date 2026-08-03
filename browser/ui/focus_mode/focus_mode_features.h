/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_FEATURES_H_
#define BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_FEATURES_H_

#include "base/feature_list.h"

namespace features {

BASE_DECLARE_FEATURE(kBraveFocusMode);

// The UI used to display the active tab's URL while Focus Mode is enabled.
enum class FocusModeUrlDisplay {
  // The URL is not displayed.
  kNone,
  // The URL is displayed in a title bar along the top of the browser window.
  kTitleBar,
  // The URL is displayed in a mini-toolbar in the corner of the contents area.
  kMiniToolbar,
};

BASE_DECLARE_FEATURE_PARAM(FocusModeUrlDisplay, kFocusModeUrlDisplay);

}  // namespace features

#endif  // BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_FEATURES_H_
