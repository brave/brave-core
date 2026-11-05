// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_
#define BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_

namespace brave {

// Icon size used by #toolbars-small-icons.
inline constexpr int kToolbarsSmallIconSize = 18;

bool IsToolbarsSmallIconsEnabled();

// Returns kToolbarsSmallIconSize when #toolbars-small-icons is enabled,
// otherwise `default_size`.
int GetToolbarsIconSize(int default_size);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_
