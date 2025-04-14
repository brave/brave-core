/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "brave/components/update_client/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/embedder_support/switches.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/test/browser_test.h"
#include "services/network/public/cpp/network_switches.h"

using BraveMainDelegateBrowserTest = PlatformBrowserTest;

constexpr char kBraveOriginTrialsPublicKey[] =
    "bYUKPJoPnCxeNvu72j4EmPuK7tr1PAC7SHh8ld9Mw3E=,"
    "fMS4mpO6buLQ/QMd+zJmxzty/VQ6B1EUZqoCU04zoRU=";

struct SyncUrlTestCase {
  // Input parameters for SetUpCommandLine
  std::optional<std::string> input_sync_url;
  std::optional<std::string> input_unsafe_origin;

  // Expected results after command line processing
  bool expect_sync_url_switch;
  std::optional<std::string> expected_sync_url_value;

  // Test case name suffix
  const char* test_name_suffix;
};

class BraveMainDelegateSyncUrlBrowserTest
    : public PlatformBrowserTest,
      public testing::WithParamInterface<SyncUrlTestCase> {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    const SyncUrlTestCase& test_case = GetParam();
    if (test_case.input_sync_url) {
      command_line->AppendSwitchASCII(syncer::kSyncServiceURL,
                                      *test_case.input_sync_url);
    }
    if (test_case.input_unsafe_origin) {
      command_line->AppendSwitchASCII(
          network::switches::kUnsafelyTreatInsecureOriginAsSecure,
          *test_case.input_unsafe_origin);
    }
  }
};

const SyncUrlTestCase kSyncUrlTestCases[] = {
    // Test Case 1: HTTPS URL (secure)
    // A secure HTTPS URL should be accepted and retained unchanged
    {"https://some-sync-server.com/v2", std::nullopt, true,
     "https://some-sync-server.com/v2", "Secure"},

    // Test Case 2: No URL provided
    // When no sync URL is provided, the switch should be removed entirely
    {std::nullopt, std::nullopt, false, std::nullopt, "None"},

    // Test Case 3: HTTP URL (insecure)
    // An insecure HTTP URL should be rejected and the switch removed
    // unless the origin is explicitly marked as safe
    {"http://insecure-sync-server.com/v2", std::nullopt,
     false,  // Should be removed
     std::nullopt, "Insecure"},

    // Test Case 4: Localhost HTTP URL
    // Localhost is considered a potentially trustworthy origin,
    // so HTTP is allowed for localhost URLs
    {"http://localhost:8295/v2", std::nullopt,
     true,  // Localhost is always allowed
     "http://localhost:8295/v2", "Localhost"},

    // Test Case 5: Insecure HTTP URL with origin explicitly marked as safe
    // When an origin is explicitly marked as safe via the
    // |--unsafely-treat-insecure-origin-as-secure| switch, HTTP is allowed
    {"http://insecure-sync-server.com/v2", "http://insecure-sync-server.com",
     true,  // Allowed via unsafe origin switch
     "http://insecure-sync-server.com/v2", "InsecureButAllowed"}};

IN_PROC_BROWSER_TEST_P(BraveMainDelegateSyncUrlBrowserTest, SyncUrlHandling) {
  const SyncUrlTestCase& test_case = GetParam();
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  EXPECT_EQ(test_case.expect_sync_url_switch,
            command_line->HasSwitch(syncer::kSyncServiceURL));

  if (test_case.expect_sync_url_switch) {
    ASSERT_TRUE(test_case.expected_sync_url_value.has_value());
    EXPECT_EQ(*test_case.expected_sync_url_value,
              command_line->GetSwitchValueASCII(syncer::kSyncServiceURL));
  } else {
    // If the switch is not expected, the value should also not be expected.
    ASSERT_FALSE(test_case.expected_sync_url_value.has_value());
  }
}

INSTANTIATE_TEST_SUITE_P(
    SyncUrlTests,
    BraveMainDelegateSyncUrlBrowserTest,
    testing::ValuesIn(kSyncUrlTestCases),
    [](const testing::TestParamInfo<SyncUrlTestCase>& info) {
      return info.param.test_name_suffix;
    });

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest,
                       DomainReliabilityServiceDisabled) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableDomainReliability));
  EXPECT_FALSE(domain_reliability::ShouldCreateService());
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest,
                       ComponentUpdaterReplacement) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kComponentUpdater));
  EXPECT_EQ(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                switches::kComponentUpdater),
            std::string("url-source=") + BUILDFLAG(UPDATER_PROD_ENDPOINT));
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, OriginTrialsTest) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      embedder_support::kOriginTrialPublicKey));
  EXPECT_EQ(kBraveOriginTrialsPublicKey,
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                embedder_support::kOriginTrialPublicKey));
}
