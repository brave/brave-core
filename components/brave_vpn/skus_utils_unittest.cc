/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace skus {

TEST(SkusUtilsUnittest, GetDefaultEnvironment) {
#if defined(OFFICIAL_BUILD)
    EXPECT_EQ(GetDefaultEnvironment(), kEnvProduction);
#else
    EXPECT_EQ(GetDefaultEnvironment(), kEnvDevelopment);
#endif
}

TEST(SkusUtilsUnittest, GetEnvironmentForDomain) {
  EXPECT_EQ(GetEnvironmentForDomain("account.brave.com"), kEnvProduction);
  EXPECT_EQ(GetEnvironmentForDomain("vpn.brave.com"), kEnvProduction);
  
  EXPECT_EQ(GetEnvironmentForDomain("vpn.bravesoftware.com"), kEnvStaging);
  EXPECT_EQ(GetEnvironmentForDomain("account.bravesoftware.com"), kEnvStaging);

  EXPECT_EQ(GetEnvironmentForDomain("vpn.brave.software"), kEnvDevelopment);
  EXPECT_EQ(GetEnvironmentForDomain("account.brave.software"), kEnvDevelopment);
}

TEST(SkusUtilsUnittest, GetDomain) {
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("vpn", kEnvDevelopment)), kEnvDevelopment);
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("talk", kEnvDevelopment)), kEnvDevelopment);
  
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("vpn", kEnvStaging)), kEnvStaging);
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("talk", kEnvStaging)), kEnvStaging);
  
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("vpn", kEnvProduction)), kEnvProduction);
  EXPECT_EQ(GetEnvironmentForDomain(GetDomain("talk", kEnvProduction)), kEnvProduction);
}

}  // namespcae skus
