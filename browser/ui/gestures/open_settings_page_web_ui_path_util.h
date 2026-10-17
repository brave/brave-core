// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_OPEN_SETTINGS_PAGE_WEB_UI_PATH_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_OPEN_SETTINGS_PAGE_WEB_UI_PATH_UTIL_H_

#include <string_view>

#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"

namespace gestures {

// `settings_page`'s settings WebUI relative path, in the same bare,
// no-leading-slash form as the `chrome::k*SubPage` constants
// `ShowSettingsSubPage` expects. Kept in sync with `PAGE_PATHS` in
// `open_settings_page.ts`. Falls back to `""` (the basic, unparameterized
// settings page) for `kBasic` itself and for any value that doesn't name a
// real page (the reserved gap between the upstream and Brave-specific
// blocks, a boundary marker like `kMaxUpstreamValue`/`kMaxValue`, or an
// out-of-range int cast from a corrupted pref).
std::string_view PageToWebUIPath(OpenSettingsPage settings_page);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_OPEN_SETTINGS_PAGE_WEB_UI_PATH_UTIL_H_
