/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/lens/lens_features.cc"

#include "base/feature_override.h"

namespace lens {
namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kEnableLensPing, base::FEATURE_DISABLED_BY_DEFAULT},
    {kLensStandalone, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
}  // namespace lens
