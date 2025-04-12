/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_main_delegate.h"

#include <string>

#include "base/command_line.h"
#include "brave/components/brave_sync/buildflags.h"
#include "brave/components/variations/buildflags.h"
#include "components/embedder_support/switches.h"
#include "components/sync/base/command_line_switches.h"
#include "components/variations/variations_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

constexpr char kBraveOriginTrialsPublicKey[] =
    "bYUKPJoPnCxeNvu72j4EmPuK7tr1PAC7SHh8ld9Mw3E=,"
    "fMS4mpO6buLQ/QMd+zJmxzty/VQ6B1EUZqoCU04zoRU=";

TEST(BraveMainDelegateUnitTest, DefaultCommandLineOverrides) {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  BraveMainDelegate::AppendCommandLineOptions();

  ASSERT_STREQ(
      kBraveOriginTrialsPublicKey,
      command_line.GetSwitchValueASCII(embedder_support::kOriginTrialPublicKey)
          .c_str());
  ASSERT_STREQ(
      BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL),
      command_line
          .GetSwitchValueASCII(variations::switches::kVariationsServerURL)
          .c_str());
  ASSERT_STREQ(BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL),
               command_line
                   .GetSwitchValueASCII(
                       variations::switches::kVariationsInsecureServerURL)
                   .c_str());
}

TEST(BraveMainDelegateUnitTest, OverrideSwitchFromCommandLine) {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  const std::string override_sync_url = "https://sync.com";
  const std::string override_origin_trials_public_key = "public_key";
  command_line.AppendSwitchASCII(syncer::kSyncServiceURL, override_sync_url);
  command_line.AppendSwitchASCII(embedder_support::kOriginTrialPublicKey,
                                 override_origin_trials_public_key);

  BraveMainDelegate::AppendCommandLineOptions();

  ASSERT_STREQ(
      override_sync_url.c_str(),
      command_line.GetSwitchValueASCII(syncer::kSyncServiceURL).c_str());
  ASSERT_STREQ(
      override_origin_trials_public_key.c_str(),
      command_line.GetSwitchValueASCII(embedder_support::kOriginTrialPublicKey)
          .c_str());
}
