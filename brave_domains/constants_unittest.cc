// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/brave_domains/constants.h"

#include <string>

#include "base/strings/strcat.h"
#include "base/test/scoped_command_line.h"
#include "brave/brave_domains/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_domains {

namespace {
std::string ExpectedGate3Host(const char* domain) {
  return base::StrCat({"gate3.wallet.", domain});
}
}  // namespace

TEST(GetGate3URLTest, DefaultIsProductionURL) {
  GURL gurl(GetGate3URL());
  EXPECT_TRUE(gurl.is_valid());
  EXPECT_EQ(gurl.scheme(), "https");
  EXPECT_EQ(gurl.host(),
            ExpectedGate3Host(BUILDFLAG(BRAVE_SERVICES_PRODUCTION_DOMAIN)));
}

TEST(GetGate3URLTest, DevEnvironment) {
  base::test::ScopedCommandLine scoped_cl;
  scoped_cl.GetProcessCommandLine()->AppendSwitchASCII("env-gate3.wallet",
                                                       "dev");

  GURL gurl(GetGate3URL());
  EXPECT_TRUE(gurl.is_valid());
  EXPECT_EQ(gurl.host(),
            ExpectedGate3Host(BUILDFLAG(BRAVE_SERVICES_DEV_DOMAIN)));
}

TEST(GetGate3URLTest, StagingEnvironment) {
  base::test::ScopedCommandLine scoped_cl;
  scoped_cl.GetProcessCommandLine()->AppendSwitchASCII("env-gate3.wallet",
                                                       "staging");

  GURL gurl(GetGate3URL());
  EXPECT_TRUE(gurl.is_valid());
  EXPECT_EQ(gurl.host(),
            ExpectedGate3Host(BUILDFLAG(BRAVE_SERVICES_STAGING_DOMAIN)));
}

}  // namespace brave_domains
