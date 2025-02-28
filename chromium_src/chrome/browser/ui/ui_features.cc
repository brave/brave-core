// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/ui_features.h"

#define kShowDefaultBrowserAppMenuItem kShowDefaultBrowserAppMenuItem_Unused
#include "src/chrome/browser/ui/ui_features.cc"
#undef kShowDefaultBrowserAppMenuItem

#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {kFewerUpdateConfirmations, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kTabHoverCardImages, base::FEATURE_DISABLED_BY_DEFAULT},
}});

const base::FeatureParam<bool> kShowDefaultBrowserAppMenuItem{
    &kDefaultBrowserPromptRefresh, "show_app_menu_item", false};

}  // namespace features
