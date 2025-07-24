/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <components/shared_highlighting/core/common/shared_highlighting_features.cc>

namespace shared_highlighting {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kSharedHighlightingManager, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace shared_highlighting
