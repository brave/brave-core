// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_COMMON_FEATURES_H_

#include "base/component_export.h"
#include "base/feature.h"
#include "brave/components/traffic_control/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_TRAFFIC_CONTROL));

namespace traffic_control::features {

COMPONENT_EXPORT(TRAFFIC_CONTROL_FEATURES)
BASE_DECLARE_FEATURE(kTrafficControl);

}  // namespace traffic_control::features

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_COMMON_FEATURES_H_
