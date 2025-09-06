/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define TODO_BASE_FEATURE_MACROS_NEED_MIGRATION

#include "base/feature_override.h"

#include <base/features.cc>

namespace base::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_ANDROID)
    {kCollectAndroidFrameTimelineMetrics, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
}});

}  // namespace base::features

#undef TODO_BASE_FEATURE_MACROS_NEED_MIGRATION
