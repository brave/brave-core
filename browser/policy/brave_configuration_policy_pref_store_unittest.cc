// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/policy/brave_simple_policy_map.h"
#include "brave/components/constants/pref_names.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/core/browser/configuration_policy_pref_store.h"
#include "components/policy/core/browser/configuration_policy_pref_store_test.h"
#include "components/policy/core/common/policy_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

class BraveConfigurationPolicyPrefStoreTest
    : public ConfigurationPolicyPrefStoreTest {
  void SetUp() override {
    for (const auto& entry : kBraveSimplePolicyMap) {
      handler_list_.AddHandler(std::make_unique<SimplePolicyHandler>(
          entry.policy_name, entry.preference_path, entry.value_type));
    }
  }
};

TEST_F(BraveConfigurationPolicyPrefStoreTest, GetDefaultFingerprintingV2) {
  EXPECT_FALSE(store_->GetValue(kManagedDefaultBraveFingerprintingV2, nullptr));

  const base::Value* value = nullptr;
  PolicyMap policy;
  policy.Set(key::kDefaultBraveFingerprintingV2Setting, POLICY_LEVEL_MANDATORY,
             POLICY_SCOPE_USER, POLICY_SOURCE_BRAVE,
             base::Value(CONTENT_SETTING_ALLOW), nullptr);
  UpdateProviderPolicy(policy);
  EXPECT_TRUE(store_->GetValue(kManagedDefaultBraveFingerprintingV2, &value));
  EXPECT_EQ(base::Value(CONTENT_SETTING_ALLOW), *value);

  policy.Set(key::kDefaultBraveFingerprintingV2Setting, POLICY_LEVEL_MANDATORY,
             POLICY_SCOPE_USER, POLICY_SOURCE_BRAVE,
             base::Value(CONTENT_SETTING_BLOCK), nullptr);
  UpdateProviderPolicy(policy);
  EXPECT_TRUE(store_->GetValue(kManagedDefaultBraveFingerprintingV2, &value));
  EXPECT_EQ(base::Value(CONTENT_SETTING_BLOCK), *value);

  policy.Set(key::kDefaultBraveFingerprintingV2Setting, POLICY_LEVEL_MANDATORY,
             POLICY_SCOPE_USER, POLICY_SOURCE_BRAVE,
             base::Value(CONTENT_SETTING_ASK), nullptr);
  UpdateProviderPolicy(policy);
  EXPECT_TRUE(store_->GetValue(kManagedDefaultBraveFingerprintingV2, &value));
  EXPECT_EQ(base::Value(CONTENT_SETTING_ASK), *value);
}

}  // namespace policy
