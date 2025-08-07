/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/brave_account_navigation_throttle.h"

#include "brave/browser/brave_account/allow_brave_account_tag.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

struct TestCase {
  bool should_tag_web_contents;
  std::string url;
  bool expected_create_throttle;
  std::optional<content::NavigationThrottle::ThrottleAction> expected_action;
  std::optional<int> expected_error_code;
};

}  // namespace

class BraveAccountNavigationThrottleUnitTest
    : public testing::TestWithParam<TestCase> {
  content::BrowserTaskEnvironment task_environment;
};

TEST_P(BraveAccountNavigationThrottleUnitTest, NavigationThrottleBehavior) {
  const TestCase& test_case = GetParam();

  TestingProfile profile;
  auto web_contents = content::WebContents::Create(
      content::WebContents::CreateParams(&profile));
  if (test_case.should_tag_web_contents) {
    AllowBraveAccountTag::Mark(web_contents.get());
  }

  content::MockNavigationHandle handle(web_contents.get());
  handle.set_url(GURL(test_case.url));
  content::MockNavigationThrottleRegistry registry(
      &handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  BraveAccountNavigationThrottle::MaybeCreateAndAdd(registry);

  EXPECT_EQ(registry.throttles().empty(), !test_case.expected_create_throttle);
  if (!test_case.expected_create_throttle) {
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
        // Non-chrome://account/ URLs - no throttle created
        TestCase{false, "https://example.com", false, std::nullopt,
                 std::nullopt},
        TestCase{false, "file:///tmp/test.html", false, std::nullopt,
                 std::nullopt},
        TestCase{false, "chrome://settings", false, std::nullopt, std::nullopt},
        TestCase{false, "chrome://account/path", false, std::nullopt,
                 std::nullopt},
        // Exact chrome://account/ URL - throttle created, navigation canceled
        TestCase{false, "chrome://account/", true,
                 content::NavigationThrottle::CANCEL, net::ERR_INVALID_URL},
        // Exact chrome://account/ URL with tag - throttle created, navigation
        // allowed
        TestCase{true, "chrome://account/", true,
                 content::NavigationThrottle::PROCEED, net::OK}),
    [](const auto& info) {
      std::string name;
      base::ReplaceChars(info.param.url, "/:.-", "_", &name);
      return name +
             (info.param.should_tag_web_contents ? "_tagged" : "_untagged");
    });
