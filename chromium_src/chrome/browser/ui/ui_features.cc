/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/ui_features.h"

#define HasTabSearchToolbarButton HasTabSearchToolbarButton_ChromiumImpl
#include <chrome/browser/ui/ui_features.cc>
#undef HasTabSearchToolbarButton

#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {kFewerUpdateConfirmations, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    // TODO(https://github.com/brave/brave-browser/issues/46337): Re-enable
    // scrim views if needed.
    {kSideBySide, base::FEATURE_ENABLED_BY_DEFAULT},
    {kTabHoverCardImages, base::FEATURE_DISABLED_BY_DEFAULT},
    {kTabstripComboButton, base::FEATURE_ENABLED_BY_DEFAULT},
}});

bool HasTabSearchToolbarButton() {
  return true;
}

}  // namespace features
