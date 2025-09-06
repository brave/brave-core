/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define TODO_BASE_FEATURE_MACROS_NEED_MIGRATION

#include "base/feature_override.h"

#include <chrome/common/privacy_budget/privacy_budget_features.cc>

#undef TODO_BASE_FEATURE_MACROS_NEED_MIGRATION

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kIdentifiabilityStudyMetaExperiment, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
