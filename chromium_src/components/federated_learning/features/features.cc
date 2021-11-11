/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "../../../../../components/federated_learning/features/features.cc"

#include "base/feature_override.h"

namespace federated_learning {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kFederatedLearningOfCohorts, base::FEATURE_DISABLED_BY_DEFAULT},
    {kFlocIdComputedEventLogging, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace federated_learning
