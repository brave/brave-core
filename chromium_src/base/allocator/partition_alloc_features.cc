/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "build/build_config.h"

#include <base/allocator/partition_alloc_features.cc>

namespace base::features {

#if BUILDFLAG(IS_ANDROID)
OVERRIDE_FEATURE_DEFAULT_STATES({{
    // Temporary disable kPartitionAllocFreeWithSize on Android as it causes crashes.
    // This override should be removed in cr148 as it is already disabled there https://chromium-review.googlesource.com/c/chromium/src/+/7707531
    {kPartitionAllocFreeWithSize, base::FEATURE_DISABLED_BY_DEFAULT},
}});
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace base::features
