// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "src/chrome/browser/ui/ui_features.cc"

#include "base/feature_list.h"
#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kTabHoverCardImages, base::FEATURE_DISABLED_BY_DEFAULT},
    // Unified SidePanel actually means that each Side Panel item's WebUI is
    // a separate page, instead of 1 page that has different functions, e.g.
    // (reading list and bookmarks). We want this feature immediately because
    // Brave have its own control for showing Side Panels individually via
    // Brave's Side Bar.
    {kUnifiedSidePanel, base::FEATURE_ENABLED_BY_DEFAULT},
}});

}  // namespace features
