/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/first_run/first_run.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/browser/metrics/switches.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/first_run/first_run.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "chrome/browser/ui/ui_features.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
TEST(FirstRunTest, OverrideIsMetricsReportingOptInToEnabled) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(metrics::switches::kForceMetricsOptInEnabled);

  EXPECT_TRUE(first_run::IsMetricsReportingOptIn());
}

TEST(FirstRunTest, OverrideIsMetricsReportingOptInToDisabled) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(metrics::switches::kForceMetricsOptInDisabled);

  EXPECT_FALSE(first_run::IsMetricsReportingOptIn());
}
#endif

TEST(FirstRunTest, IsMetricsReportingOptInDefaultValue) {
#if BUILDFLAG(IS_ANDROID)
  EXPECT_FALSE(
      brave::first_run::IsMetricsReportingOptIn(version_info::Channel::STABLE));
#else
  EXPECT_TRUE(
      brave::first_run::IsMetricsReportingOptIn(version_info::Channel::STABLE));
#endif
  EXPECT_FALSE(
      brave::first_run::IsMetricsReportingOptIn(version_info::Channel::BETA));
  EXPECT_FALSE(
      brave::first_run::IsMetricsReportingOptIn(version_info::Channel::DEV));
  EXPECT_FALSE(
      brave::first_run::IsMetricsReportingOptIn(version_info::Channel::CANARY));
  EXPECT_TRUE(brave::first_run::IsMetricsReportingOptIn(
      version_info::Channel::UNKNOWN));
}

#if BUILDFLAG(IS_MAC)
TEST(FirstRunTest, FeatureTestOnMac) {
  // To detect macOS specific FirstRun dialog deprecation.
  // When it's deprecated, we can delete first_run_dialog_controller.mm
  // overrides.
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kViewsFirstRunDialog));
}
#endif
