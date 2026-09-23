/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/sanitizer_buildflags.h"
#include "build/build_config.h"

#if !BUILDFLAG(IS_WIN) && !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(USING_SANITIZER)
#include <array>
#include <string_view>

#include "base/feature_list.h"

namespace {
constexpr std::array<std::string_view, 1> kFieldTrialExceptions = {
    "FledgeEnforceKAnonymity",
};
}

// `ContentTestSuite` has a CHECK to ensure that the field trial flags it
// disables are currently enabled. However, there are certain field trial flags
// that we always disable, so we must skip this CHECK for those flags.
#define GetEnabledFieldTrialByFeatureName(FEATURE) \
  GetEnabledFieldTrialByFeatureName(FEATURE) ||    \
      std::ranges::contains(kFieldTrialExceptions, FEATURE)
#endif

#include <content/test/content_test_suite.cc>

#if !BUILDFLAG(IS_WIN) && !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(USING_SANITIZER)
#undef GetEnabledFieldTrialByFeatureName
#endif
