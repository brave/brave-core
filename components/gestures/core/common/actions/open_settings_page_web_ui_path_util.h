// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_WEB_UI_PATH_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_WEB_UI_PATH_UTIL_H_

#include <string_view>

#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"

namespace gestures {

// `settings_page`'s path, in the bare form `chrome::ShowSettingsSubPage`
// takes; `""` for `kBasic` or an out-of-range value.
std::string_view ToSettingsSubPage(OpenSettingsPageType type);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_WEB_UI_PATH_UTIL_H_
