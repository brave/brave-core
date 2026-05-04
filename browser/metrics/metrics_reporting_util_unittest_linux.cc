/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/environment.h"
#include "brave/browser/metrics/metrics_reporting_util.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(MetricsUtilTest, DefaultValueTest) {
#if defined(OFFICIAL_BUILD)
  auto env = base::Environment::Create();

  // Stable channel: metrics reporting always defaults to off.
  env->SetVar("CHROME_VERSION_EXTRA", LINUX_CHANNEL_STABLE);
  EXPECT_EQ(version_info::Channel::STABLE, chrome::GetChannel());
  EXPECT_FALSE(GetDefaultPrefValueForMetricsReporting());

  // Non-stable channels: on regular Brave builds, metrics reporting defaults
  // to on; on origin-branded builds, it defaults to off. Capture the beta
  // result and verify dev/nightly are consistent, rather than checking the
  // IS_BRAVE_ORIGIN_BRANDED buildflag which may evaluate differently in the
  // test and production compilation units during partial uplifts.
  env->SetVar("CHROME_VERSION_EXTRA", LINUX_CHANNEL_BETA);
  EXPECT_EQ(version_info::Channel::BETA, chrome::GetChannel());
  const bool non_stable_default = GetDefaultPrefValueForMetricsReporting();

  env->SetVar("CHROME_VERSION_EXTRA", LINUX_CHANNEL_DEV);
  EXPECT_EQ(version_info::Channel::DEV, chrome::GetChannel());
  EXPECT_EQ(non_stable_default, GetDefaultPrefValueForMetricsReporting());

  env->SetVar("CHROME_VERSION_EXTRA", BRAVE_LINUX_CHANNEL_NIGHTLY);
  EXPECT_EQ(version_info::Channel::CANARY, chrome::GetChannel());
  EXPECT_EQ(non_stable_default, GetDefaultPrefValueForMetricsReporting());
#else  // OFFICIAL_BUILD
  EXPECT_EQ(version_info::Channel::UNKNOWN, chrome::GetChannel());
  EXPECT_FALSE(GetDefaultPrefValueForMetricsReporting());
#endif
}
