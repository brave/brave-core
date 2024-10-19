/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/user_education/common/user_education_features.cc"

#include "base/feature_override.h"

namespace user_education::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kWhatsNewVersion2, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace user_education::features
