/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/handlers/brave_referrers_policy_handler.h"

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

class BraveReferrersPolicyHandlerTest : public testing::Test {
 protected:
  void SetAndApplyPolicy(base::Value value) {
    policies_.Set(key::kDefaultBraveReferrersSetting, POLICY_LEVEL_MANDATORY,
                  POLICY_SCOPE_USER, POLICY_SOURCE_PLATFORM, std::move(value),
                  nullptr);
    PolicyHandlerParameters params;
    handler_.ApplyPolicySettingsWithParameters(policies_, params, &prefs_);
  }

  PrefValueMap& prefs() { return prefs_; }

 private:
  PolicyMap policies_;
  PrefValueMap prefs_;
  BraveReferrersPolicyHandler handler_;
};

TEST_F(BraveReferrersPolicyHandlerTest, ApplyPolicyWithWrongTypeValue) {
  SetAndApplyPolicy(base::Value(false));

  base::Value* result;
  EXPECT_FALSE(
      prefs().GetValue(prefs::kManagedDefaultBraveReferrersSetting, &result));
}

TEST_F(BraveReferrersPolicyHandlerTest, ApplyPolicyWithLessThanMinValue) {
  const int out_of_range_value =
      static_cast<int>(BraveReferrersSetting::kAllowReferrer) - 1;
  SetAndApplyPolicy(base::Value(out_of_range_value));

  base::Value* result;
  EXPECT_FALSE(
      prefs().GetValue(prefs::kManagedDefaultBraveReferrersSetting, &result));
}

TEST_F(BraveReferrersPolicyHandlerTest, ApplyPolicyWithGreaterThanMaxValue) {
  const int out_of_range_value =
      static_cast<int>(BraveReferrersSetting::kBlockReferrer) + 1;
  SetAndApplyPolicy(base::Value(out_of_range_value));

  base::Value* result;
  EXPECT_FALSE(
      prefs().GetValue(prefs::kManagedDefaultBraveReferrersSetting, &result));
}

TEST_F(BraveReferrersPolicyHandlerTest, ApplyPolicyWithAllowReferrerValue) {
  const int valid_value =
      static_cast<int>(BraveReferrersSetting::kAllowReferrer);
  SetAndApplyPolicy(base::Value(valid_value));

  int result;
  EXPECT_TRUE(
      prefs().GetInteger(prefs::kManagedDefaultBraveReferrersSetting, &result));
  EXPECT_THAT(result, testing::Eq(CONTENT_SETTING_ALLOW));
}

TEST_F(BraveReferrersPolicyHandlerTest, ApplyPolicyWithBlockReferrerValue) {
  const int valid_value =
      static_cast<int>(BraveReferrersSetting::kBlockReferrer);
  SetAndApplyPolicy(base::Value(valid_value));

  int result;
  EXPECT_TRUE(
      prefs().GetInteger(prefs::kManagedDefaultBraveReferrersSetting, &result));
  EXPECT_THAT(result, testing::Eq(CONTENT_SETTING_BLOCK));
}

}  // namespace policy
