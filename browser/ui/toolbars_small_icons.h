// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_
#define BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_

#include "base/feature_list.h"
#include "brave/browser/ui/brave_ui_features.h"

namespace brave {

// Returns the icon size to use in toolbars and similar surfaces: 18 when
// #toolbars-small-icons is enabled, otherwise `default_size`.
inline int GetToolbarsIconSize(int default_size) {
  return base::FeatureList::IsEnabled(features::kBraveToolbarsSmallIcons)
             ? 18
             : default_size;
}

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_TOOLBARS_SMALL_ICONS_H_
