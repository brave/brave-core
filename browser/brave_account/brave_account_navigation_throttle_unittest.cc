/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/brave_account_navigation_throttle.h"

#include <optional>
#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_account/features.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace {

struct TestCase {
  bool enable_feature;
  std::string url;
  bool page_transition_auto_toplevel;
  bool expected_throttle_created;
  std::optional<content::NavigationThrottle::ThrottleAction> expected_action;
  std::optional<int> expected_error_code;
};

}  // namespace

class BraveAccountNavigationThrottleUnitTest
    : public testing::TestWithParam<TestCase> {
 protected:
  void SetUp() override {
    scoped_feature_list_.InitWithFeatureState(
        brave_account::features::kBraveAccount, GetParam().enable_feature);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_P(BraveAccountNavigationThrottleUnitTest,
       BlockNavigationUnlessAutoToplevel) {
  const TestCase& test_case = GetParam();

  TestingProfile profile;
  auto web_contents = content::WebContents::Create(
      content::WebContents::CreateParams(&profile));
  content::MockNavigationHandle handle(web_contents.get());
  handle.set_url(GURL(test_case.url));
  if (test_case.page_transition_auto_toplevel) {
    handle.set_page_transition(
        ui::PageTransition::PAGE_TRANSITION_AUTO_TOPLEVEL);
  }
  content::MockNavigationThrottleRegistry registry(
      &handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  BraveAccountNavigationThrottle::MaybeCreateAndAdd(registry);

  EXPECT_EQ(registry.throttles().empty(), !test_case.expected_throttle_created);
  if (!test_case.expected_throttle_created) {
    EXPECT_FALSE(test_case.expected_action);
    EXPECT_FALSE(test_case.expected_error_code);
    return;
  }

  ASSERT_TRUE(test_case.expected_action);
  ASSERT_TRUE(test_case.expected_error_code);

  auto result = registry.throttles().back()->WillStartRequest();
  EXPECT_EQ(*test_case.expected_action, result.action());
  EXPECT_EQ(*test_case.expected_error_code, result.net_error_code());
}

INSTANTIATE_TEST_SUITE_P(
    All,
    BraveAccountNavigationThrottleUnitTest,
    testing::Values(
        // Feature disabled => no throttle created
        TestCase{false, "", false, false, std::nullopt, std::nullopt},
        // Non-chrome://account URLs => no throttle created
        TestCase{true, "https://example.com", false, false, std::nullopt,
                 std::nullopt},
        TestCase{true, "file:///tmp/test.html", false, false, std::nullopt,
                 std::nullopt},
        TestCase{true, "chrome://settings", false, false, std::nullopt,
                 std::nullopt},
        TestCase{true, "chrome://account/path", false, false, std::nullopt,
                 std::nullopt},
        // Exact chrome://account URL => throttle created, navigation canceled
        TestCase{true, "chrome://account", false, true,
                 content::NavigationThrottle::CANCEL, net::ERR_INVALID_URL},
        // Exact chrome://account URL with PAGE_TRANSITION_AUTO_TOPLEVEL =>
        // throttle created, navigation allowed
        TestCase{true, "chrome://account", true, true,
                 content::NavigationThrottle::PROCEED, net::OK}),
    [](const auto& info) -> std::string {
      if (!info.param.enable_feature) {
        return "feature_off";
      }

      std::string name;
      base::ReplaceChars(info.param.url, "/:.-", "_", &name);
      return name + (info.param.page_transition_auto_toplevel
                         ? "_auto_toplevel_page_transition"
                         : "");
    });
