/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/handlers/brave_https_upgrade_policy_handler.h"

#include "base/values.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/core/browser/configuration_policy_handler_parameters.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

class BraveHttpsUpgradePolicyHandlerTest : public testing::Test {
 protected:
  void SetAndApplyPolicy(base::Value value) {
    policies_.Set(key::kDefaultBraveHttpsUpgradeSetting, POLICY_LEVEL_MANDATORY,
                  POLICY_SCOPE_USER, POLICY_SOURCE_PLATFORM, std::move(value),
                  nullptr);
    PolicyHandlerParameters params;
    handler_.ApplyPolicySettingsWithParameters(policies_, params, &prefs_);
  }

  PrefValueMap& prefs() { return prefs_; }

 private:
  PolicyMap policies_;
  PrefValueMap prefs_;
  BraveHttpsUpgradePolicyHandler handler_;
};

TEST_F(BraveHttpsUpgradePolicyHandlerTest, ApplyPolicyWithWrongTypeValue) {
  SetAndApplyPolicy(base::Value(false));

  base::Value* result;
  EXPECT_FALSE(
      prefs().GetValue(prefs::kManagedDefaultBraveHttpsUpgrade, &result));
}

TEST_F(BraveHttpsUpgradePolicyHandlerTest, ApplyPolicyWithLessThanMinValue) {
  const int out_of_range_value =
      static_cast<int>(BraveHttpsUpgradeSetting::kDisabled) - 1;
  SetAndApplyPolicy(base::Value(out_of_range_value));

  base::Value* result;
  EXPECT_FALSE(
      prefs().GetValue(prefs::kManagedDefaultBraveHttpsUpgrade, &result));
}

TEST_F(BraveHttpsUpgradePolicyHandlerTest, ApplyPolicyWithGreaterThanMaxValue) {
  const int out_of_range_value =
      static_cast<int>(BraveHttpsUpgradeSetting::kStandard) + 1;
  SetAndApplyPolicy(base::Value(out_of_range_value));

  base::Value* result;
  EXPECT_FALSE(
      prefs().GetValue(prefs::kManagedDefaultBraveHttpsUpgrade, &result));
}

TEST_F(BraveHttpsUpgradePolicyHandlerTest, ApplyPolicyWithDisabledValue) {
  const int valid_value = static_cast<int>(BraveHttpsUpgradeSetting::kDisabled);
  SetAndApplyPolicy(base::Value(valid_value));

  int result;
  EXPECT_TRUE(
      prefs().GetInteger(prefs::kManagedDefaultBraveHttpsUpgrade, &result));
  EXPECT_THAT(result, testing::Eq(CONTENT_SETTING_ALLOW));
}

TEST_F(BraveHttpsUpgradePolicyHandlerTest, ApplyPolicyWithStrictValue) {
  const int valid_value = static_cast<int>(BraveHttpsUpgradeSetting::kStrict);
  SetAndApplyPolicy(base::Value(valid_value));

  int result;
  EXPECT_TRUE(
      prefs().GetInteger(prefs::kManagedDefaultBraveHttpsUpgrade, &result));
  EXPECT_THAT(result, testing::Eq(CONTENT_SETTING_BLOCK));
}

TEST_F(BraveHttpsUpgradePolicyHandlerTest, ApplyPolicyWithStandardValue) {
  const int valid_value = static_cast<int>(BraveHttpsUpgradeSetting::kStandard);
  SetAndApplyPolicy(base::Value(valid_value));

  int result;
  EXPECT_TRUE(
      prefs().GetInteger(prefs::kManagedDefaultBraveHttpsUpgrade, &result));
  EXPECT_THAT(result, testing::Eq(CONTENT_SETTING_ASK));
}

}  // namespace policy
