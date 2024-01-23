// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/brave_domains/service_domains.h"
#include <cstring>

#include "base/command_line.h"
#include "base/debug/debugging_buildflags.h"
#include "base/files/file_path.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/brave_domains/buildflags.h"
#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace brave_domains {

namespace {

// Setup expected answers based on the buildflag values
const char kProductionValue[] = BUILDFLAG(BRAVE_SERVICES_PRODUCTION_DOMAIN);
const char kStagingValue[] = BUILDFLAG(BRAVE_SERVICES_STAGING_DOMAIN);
const char kDevValue[] = BUILDFLAG(BRAVE_SERVICES_DEV_DOMAIN);

}  // namespace

TEST(BraveServiceDomains, TestValuesPresent) {
  // These tests don't work if the values are the same, or empty
  EXPECT_GT(strlen(kProductionValue), 0u);
  EXPECT_GT(strlen(kStagingValue), 0u);
  EXPECT_GT(strlen(kDevValue), 0u);
  EXPECT_NE(kProductionValue, kStagingValue);
  EXPECT_NE(kProductionValue, kDevValue);
  EXPECT_NE(kStagingValue, kDevValue);
}

TEST(BraveServiceDomains, ProductionWhenEmpty) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);

  EXPECT_EQ(GetServicesDomain("", "", &cl), kProductionValue);
}

TEST(BraveServiceDomains, GlobalStaging) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("brave-services-env", "staging");

  EXPECT_EQ(GetServicesDomain("", "", &cl), kStagingValue);
}

TEST(BraveServiceDomains, GlobalDev) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("brave-services-env", "dev");

  EXPECT_EQ(GetServicesDomain("", "", &cl), kDevValue);
}

TEST(BraveServiceDomains, PrefixOverride) {
  std::string prefix = "my.sub.domain";

  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("brave-services-env", "dev");
  cl.AppendSwitchASCII("env-my.sub.domain", "prod");

  auto prefixed_domain = GetServicesDomain(prefix, "", &cl);

  // Prefixed domain should be production override
  EXPECT_TRUE(base::EndsWith(prefixed_domain, kProductionValue));
  EXPECT_TRUE(base::StartsWith(prefixed_domain, prefix));

  // All other domain retrievals should be dev
  EXPECT_EQ(GetServicesDomain("", "", &cl), kDevValue);

  std::string other_prefix = "another_prefix";
  auto other_prefixed_domain = GetServicesDomain(other_prefix, "", &cl);

  EXPECT_TRUE(base::EndsWith(other_prefixed_domain, kDevValue));
  EXPECT_TRUE(base::StartsWith(other_prefixed_domain, other_prefix));
}

TEST(BraveServiceDomains, DefaultEnvValue) {
  std::string prefix = "test_prefix";

  base::CommandLine cl(base::CommandLine::NO_PROGRAM);

  // When no default is given and no switch is supplied,
  // prod is used.
  auto result = GetServicesDomain(prefix, "", &cl);
  EXPECT_EQ(result, base::StrCat({prefix, ".", kProductionValue}));

  // When no env is present from the command line switch, the default is used.
  result = GetServicesDomain(prefix, "dev", &cl);
  EXPECT_EQ(result, base::StrCat({prefix, ".", kDevValue}));

  // When an invalid env is passed as a default present the production value is
  // used.
  result = GetServicesDomain(prefix, "invalid", &cl);
  EXPECT_EQ(result, base::StrCat({prefix, ".", kProductionValue}));

  // When an env is present from the command line switch, the default is
  // ignored.
  cl.AppendSwitchASCII("env-test_prefix", "dev");
  result = GetServicesDomain(prefix, "staging", &cl);
  EXPECT_EQ(result, base::StrCat({prefix, ".", kDevValue}));

  // When an global env is present from the command line switch, the default is
  // ignored.
  base::CommandLine cl2(base::CommandLine::NO_PROGRAM);
  cl2.AppendSwitchASCII("brave-services-env", "dev");
  result = GetServicesDomain(prefix, "staging", &cl2);
  EXPECT_EQ(result, base::StrCat({prefix, ".", kDevValue}));
}

}  // namespace brave_domains
