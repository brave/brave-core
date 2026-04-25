// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/variations/command_line_utils.h"

#include <string>

#include "base/command_line.h"
#include "brave/components/variations/buildflags.h"
#include "brave/components/variations/switches.h"
#include "components/variations/variations_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace variations {

TEST(VariationsCommandLineUtils, DefaultVariationsServerUrl) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  AppendBraveCommandLineOptions(command_line);

  EXPECT_EQ(command_line.GetSwitchValueASCII(
                variations::switches::kVariationsServerURL),
            BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL));
  EXPECT_EQ(command_line.GetSwitchValueASCII(
                variations::switches::kVariationsInsecureServerURL),
            BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL));
  EXPECT_FALSE(command_line.HasSwitch(
      variations::switches::kAcceptEmptySeedSignatureForTesting));
  EXPECT_FALSE(command_line.HasSwitch(
      variations::switches::kDisableVariationsSeedFetchThrottling));
}

TEST(VariationsCommandLineUtils, OverrideVariationsServerUrl) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  const std::string override_variations_url = "https://variations.com";
  const std::string override_insecure_variations_url = "http://insecure.com";
  command_line.AppendSwitchASCII(variations::switches::kVariationsServerURL,
                                 override_variations_url);
  command_line.AppendSwitchASCII(
      variations::switches::kVariationsInsecureServerURL,
      override_insecure_variations_url);
  AppendBraveCommandLineOptions(command_line);

  EXPECT_EQ(override_variations_url,
            command_line.GetSwitchValueASCII(
                variations::switches::kVariationsServerURL));
  EXPECT_EQ(override_insecure_variations_url,
            command_line.GetSwitchValueASCII(
                variations::switches::kVariationsInsecureServerURL));
}

TEST(VariationsCommandLineUtils, SetVariationsPrParameter) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(variations::switches::kVariationsPR, "1234");
  AppendBraveCommandLineOptions(command_line);

  EXPECT_EQ(command_line.GetSwitchValueASCII(
                variations::switches::kVariationsServerURL),
            "https://griffin.brave.com/pull/1234/seed");
  EXPECT_EQ(command_line.GetSwitchValueASCII(
                variations::switches::kVariationsInsecureServerURL),
            "https://griffin.brave.com/pull/1234/seed");
  EXPECT_TRUE(command_line.HasSwitch(
      variations::switches::kAcceptEmptySeedSignatureForTesting));
  EXPECT_TRUE(command_line.HasSwitch(
      variations::switches::kDisableVariationsSeedFetchThrottling));
}

}  // namespace variations
