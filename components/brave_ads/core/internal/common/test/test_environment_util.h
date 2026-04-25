/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TEST_ENVIRONMENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TEST_ENVIRONMENT_UTIL_H_

#include "brave/components/brave_ads/core/internal/common/test/build_channel_test_types.h"

// Helpers for configuring global test environment state including device
// identity, build channel, content settings, and platform.

namespace brave_ads::test {

void SetUpDeviceId();

void SetUpBuildChannel(BuildChannelType type);

void SetUpAllowJavaScript(bool allow_javascript);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TEST_ENVIRONMENT_UTIL_H_
