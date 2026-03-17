// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/brave_domains/constants.h"

#include "base/command_line.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_domains {

TEST(GetGate3URL, DefaultIsProduction) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.com");
}

TEST(GetGate3URL, EnvGate3Dev) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("env-gate3", "dev");
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.software");
}

TEST(GetGate3URL, EnvGate3Staging) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("env-gate3", "staging");
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.software");
}

TEST(GetGate3URL, EnvGate3Prod) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("env-gate3", "prod");
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.com");
}

TEST(GetGate3URL, GlobalSwitchDev) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("brave-services-env", "dev");
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.software");
}

TEST(GetGate3URL, GlobalSwitchStaging) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("brave-services-env", "staging");
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.software");
}

TEST(GetGate3URL, EnvGate3TakesPrecedenceOverGlobal) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("brave-services-env", "prod");
  cl.AppendSwitchASCII("env-gate3", "dev");
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.software");
}

TEST(GetGate3URL, InvalidValueFallsBackToProduction) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("env-gate3", "invalid");
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.com");
}

TEST(GetGate3URL, InvalidEnvGate3FallsBackToGlobal) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("env-gate3", "invalid");
  cl.AppendSwitchASCII("brave-services-env", "dev");
  EXPECT_EQ(GetGate3URL(&cl), "https://gate3.wallet.brave.software");
}

}  // namespace brave_domains
