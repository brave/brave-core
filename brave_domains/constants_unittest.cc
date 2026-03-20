// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/brave_domains/constants.h"

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_domains {

TEST(GetGate3URLTest, AllCases) {
  struct TestCase {
    const char* name;
    std::vector<std::pair<const char*, const char*>> switches;
    const char* expected_url;
  };

  const TestCase cases[] = {
      {"DefaultIsProduction", {}, "https://gate3.wallet.brave.com"},
      {"EnvGate3Dev",
       {{kEnvGate3Switch, kBraveServicesSwitchValueDev}},
       "https://gate3.wallet.brave.software"},
      {"EnvGate3Staging",
       {{kEnvGate3Switch, kBraveServicesSwitchValueStaging}},
       "https://gate3.wallet.brave.software"},
      {"EnvGate3Prod",
       {{kEnvGate3Switch, kBraveServicesSwitchValueProduction}},
       "https://gate3.wallet.brave.com"},
      {"GlobalSwitchDev",
       {{kBraveServicesEnvironmentSwitch, kBraveServicesSwitchValueDev}},
       "https://gate3.wallet.brave.software"},
      {"GlobalSwitchStaging",
       {{kBraveServicesEnvironmentSwitch, kBraveServicesSwitchValueStaging}},
       "https://gate3.wallet.brave.software"},
      {"EnvGate3TakesPrecedenceOverGlobal",
       {{kBraveServicesEnvironmentSwitch, kBraveServicesSwitchValueProduction},
        {kEnvGate3Switch, kBraveServicesSwitchValueDev}},
       "https://gate3.wallet.brave.software"},
      {"InvalidValueFallsBackToProduction",
       {{kEnvGate3Switch, "invalid"}},
       "https://gate3.wallet.brave.com"},
      {"InvalidEnvGate3FallsBackToGlobal",
       {{kEnvGate3Switch, "invalid"},
        {kBraveServicesEnvironmentSwitch, kBraveServicesSwitchValueDev}},
       "https://gate3.wallet.brave.software"},
  };

  for (const auto& tc : cases) {
    SCOPED_TRACE(tc.name);
    base::CommandLine cl(base::CommandLine::NO_PROGRAM);
    for (const auto& [flag, value] : tc.switches) {
      cl.AppendSwitchASCII(flag, value);
    }
    EXPECT_EQ(GetGate3URL(&cl), tc.expected_url);
  }
}

}  // namespace brave_domains
