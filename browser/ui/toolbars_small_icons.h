// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_
#define BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_

#include <optional>

namespace brave {

// Icon size used by #toolbars-small-icons.
inline constexpr int kToolbarsSmallIconSize = 18;

bool IsToolbarsSmallIconsEnabled();

// Returns kToolbarsSmallIconSize when #toolbars-small-icons is enabled,
// otherwise nullopt so callers can `.value_or()` their own default.
std::optional<int> GetToolbarsIconSize();

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_
