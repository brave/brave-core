/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <chrome/browser/ui/omnibox/omnibox_next_features.cc>

namespace omnibox::internal {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kWebUIOmniboxPopup, base::FEATURE_DISABLED_BY_DEFAULT},
    {kWebUIOmniboxAimPopup, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace omnibox::internal
