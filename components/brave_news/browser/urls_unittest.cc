// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/urls.h"

#include <cstring>

#include "base/command_line.h"
#include "base/debug/debugging_buildflags.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/brave_domains/buildflags.h"
#include "brave/brave_domains/service_domains.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {

const char kBraveNewsHostnamePrefix[] = "brave-today-cdn";
const char kPCDNHostnamePrefix[] = "pcdn";

// Setup expected answers based on the brave_services buildflag values
const char kProductionValue[] = BUILDFLAG(BRAVE_SERVICES_PRODUCTION_DOMAIN);
const char kStagingValue[] = BUILDFLAG(BRAVE_SERVICES_STAGING_DOMAIN);

}  // namespace

TEST(BraveNewsURLs, GetMatchingPCDNHostname_Regular) {
  // Sanity check, we don't need to test this as it's covered in
  // brave_domains::GetServicesDomain tests.
  EXPECT_EQ(GetHostname(),
            base::StrCat({kBraveNewsHostnamePrefix, ".", kProductionValue}));
  // Test the string manipulation is working ok
  EXPECT_EQ(GetMatchingPCDNHostname(),
            base::StrCat({kPCDNHostnamePrefix, ".", kProductionValue}));
}

TEST(BraveNewsURLs, GetMatchingPCDNHostname_Staging) {
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();
  cl->AppendSwitchASCII(base::StrCat({"env-", kBraveNewsHostnamePrefix}),
                        "staging");
  // Both should be staging even though we only override the main brave news
  // hostname.
  EXPECT_EQ(GetHostname(),
            base::StrCat({kBraveNewsHostnamePrefix, ".", kStagingValue}));
  EXPECT_EQ(GetMatchingPCDNHostname(),
            base::StrCat({kPCDNHostnamePrefix, ".", kStagingValue}));
}

TEST(BraveNewsURLs, GetMatchingPCDNHostname_AvoidPCDNOverride) {
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();
  cl->AppendSwitchASCII(base::StrCat({"env-", kPCDNHostnamePrefix}), "staging");
  // We never want the actual PCDN override (which may be specified by a user
  // for another component?) to affect the output of this function.
  EXPECT_EQ(GetHostname(),
            base::StrCat({kBraveNewsHostnamePrefix, ".", kProductionValue}));
  EXPECT_EQ(GetMatchingPCDNHostname(),
            base::StrCat({kPCDNHostnamePrefix, ".", kProductionValue}));
  // Sanity check
  EXPECT_EQ(brave_domains::GetServicesDomain(kPCDNHostnamePrefix),
            base::StrCat({kPCDNHostnamePrefix, ".", kStagingValue}));
}

}  // namespace brave_news
