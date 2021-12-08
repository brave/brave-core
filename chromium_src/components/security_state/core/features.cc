/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/security_state/core/features.cc"

#include "base/feature_override.h"

namespace security_state {
namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kSafetyTipUI, base::FEATURE_ENABLED_BY_DEFAULT},
}});

}  // namespace features
}  // namespace security_state
