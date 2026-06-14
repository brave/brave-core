/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <components/personal_context/core/personal_context_features.cc>

namespace personal_context::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kPersonalContext, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace personal_context::features
