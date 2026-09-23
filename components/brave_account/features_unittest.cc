/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/features.h"

#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/components/email_aliases/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::features {

namespace {

struct BraveAccountFeaturesTestCase {
  std::string test_name;
  bool brave_account_enabled;
  bool email_aliases_enabled;
  bool expected_is_brave_account_enabled;
};

class BraveAccountFeaturesTest
    : public testing::TestWithParam<BraveAccountFeaturesTestCase> {
 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

}  // namespace

TEST_P(BraveAccountFeaturesTest, IsBraveAccountEnabled) {
  const auto& test_case = GetParam();

  scoped_feature_list_.InitWithFeatureStates({
      {BraveAccountFeatureForTesting(), test_case.brave_account_enabled},
      {email_aliases::features::kEmailAliases, test_case.email_aliases_enabled},
  });

  EXPECT_EQ(IsBraveAccountEnabled(),
            test_case.expected_is_brave_account_enabled);
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountFeaturesTests,
    BraveAccountFeaturesTest,
    testing::Values(
        BraveAccountFeaturesTestCase{
            .test_name = "brave_account_disabled_email_aliases_disabled",
            .brave_account_enabled = false,
            .email_aliases_enabled = false,
            .expected_is_brave_account_enabled = false},
        BraveAccountFeaturesTestCase{
            .test_name = "brave_account_disabled_email_aliases_enabled",
            .brave_account_enabled = false,
            .email_aliases_enabled = true,
            .expected_is_brave_account_enabled = true},
        BraveAccountFeaturesTestCase{
            .test_name = "brave_account_enabled_email_aliases_disabled",
            .brave_account_enabled = true,
            .email_aliases_enabled = false,
            .expected_is_brave_account_enabled = true},
        BraveAccountFeaturesTestCase{
            .test_name = "brave_account_enabled_email_aliases_enabled",
            .brave_account_enabled = true,
            .email_aliases_enabled = true,
            .expected_is_brave_account_enabled = true}),
    [](const auto& info) { return info.param.test_name; });

}  // namespace brave_account::features
