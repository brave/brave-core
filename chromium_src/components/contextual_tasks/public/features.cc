/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/contextual_tasks/public/features.h"

#include "base/feature_override.h"

#include <components/contextual_tasks/public/features.cc>

namespace contextual_tasks {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kContextualTasks, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace contextual_tasks
