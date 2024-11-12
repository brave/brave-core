/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/companion/core/features.cc"

#include "base/feature_override.h"

namespace companion::features::internal {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kCompanionEnabledByObservingExpsNavigations,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kSidePanelCompanion, base::FEATURE_DISABLED_BY_DEFAULT},
    {kSidePanelCompanion2, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace companion::features::internal
