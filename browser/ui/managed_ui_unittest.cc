/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/values.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {
// Forward declare the function from chromium_src
bool HasOnlyBravePolicies(const policy::PolicyMap& policies);
}  // namespace brave_policy

TEST(ManagedUiTest, HasOnlyBravePolicies_EmptyPolicies) {
  policy::PolicyMap empty_policies;
  EXPECT_TRUE(brave_policy::HasOnlyBravePolicies(empty_policies));
}

TEST(ManagedUiTest, HasOnlyBravePolicies_OnlyBravePolicies) {
  policy::PolicyMap policies;
  policies.Set("BravePolicy1", policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
               base::Value("value1"), nullptr);
  policies.Set("BravePolicy2", policy::POLICY_LEVEL_RECOMMENDED,
               policy::POLICY_SCOPE_MACHINE, policy::POLICY_SOURCE_BRAVE,
               base::Value("value2"), nullptr);

  EXPECT_TRUE(brave_policy::HasOnlyBravePolicies(policies));
}

TEST(ManagedUiTest, HasOnlyBravePolicies_MixedPolicies) {
  policy::PolicyMap policies;
  policies.Set("BravePolicy", policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
               base::Value("brave_value"), nullptr);
  policies.Set("EnterprisePolicy", policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_MACHINE,
               policy::POLICY_SOURCE_ENTERPRISE_DEFAULT,
               base::Value("enterprise_value"), nullptr);

  EXPECT_FALSE(brave_policy::HasOnlyBravePolicies(policies));
}

TEST(ManagedUiTest, HasOnlyBravePolicies_OnlyNonBravePolicies) {
  policy::PolicyMap policies;
  policies.Set("CloudPolicy", policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
               base::Value("cloud_value"), nullptr);
  policies.Set("PlatformPolicy", policy::POLICY_LEVEL_RECOMMENDED,
               policy::POLICY_SCOPE_MACHINE, policy::POLICY_SOURCE_PLATFORM,
               base::Value("platform_value"), nullptr);

  EXPECT_FALSE(brave_policy::HasOnlyBravePolicies(policies));
}
