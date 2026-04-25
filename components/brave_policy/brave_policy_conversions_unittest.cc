// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Tests for the policy conversions array override in
// chromium_src/components/policy/core/browser/policy_conversions.cc

#include <string>

#include "base/values.h"
#include "components/enterprise/browser/reporting/policy_info.h"
#include "components/grit/brave_components_strings.h"
#include "components/policy/core/browser/policy_conversions.h"
#include "components/policy/core/browser/policy_conversions_client.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

class StubPolicyConversionsClient : public PolicyConversionsClient {
 public:
  StubPolicyConversionsClient() = default;
  ~StubPolicyConversionsClient() override = default;

  // PolicyConversionsClient overrides
  bool HasUserPolicies() const override { return false; }
  base::ListValue GetExtensionPolicies(PolicyDomain policy_domain) override {
    return base::ListValue();
  }
  PolicyService* GetPolicyService() const override { return nullptr; }
  SchemaRegistry* GetPolicySchemaRegistry() const override { return nullptr; }
  const ConfigurationPolicyHandlerList* GetHandlerList() const override {
    return nullptr;
  }

  // Expose the protected method for testing
  base::DictValue GetPolicyValuesForTest(
      const PolicyMap& map,
      const std::optional<PolicyConversions::PolicyToSchemaMap>&
          known_policy_schemas) const {
    return GetPolicyValues(map, nullptr, PoliciesSet(), PoliciesSet(),
                           known_policy_schemas);
  }
};

class BravePolicyConversionsTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

// Test that Brave policy source appears in policy conversions
// This tests the override in
// chromium_src/components/policy/core/browser/policy_conversions.cc The test
// ensures that "policySourceBrave" appears in the policy source mapping array
TEST_F(BravePolicyConversionsTest, BraveInPolicyConversions) {
  // Add a policy with POLICY_SOURCE_BRAVE
  PolicyMap policy_map;
  const std::string test_policy_name = "TestBravePolicy";

  PolicyMap::Entry brave_entry(POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                               POLICY_SOURCE_BRAVE, base::Value("test_value"),
                               nullptr);

  policy_map.Set(test_policy_name, brave_entry.DeepCopy());

  // Use the client to get policy values
  StubPolicyConversionsClient client;
  base::DictValue policy_values =
      client.GetPolicyValuesForTest(policy_map, std::nullopt);

  // Find our test policy in the returned policy values
  const base::DictValue* test_policy = policy_values.FindDict(test_policy_name);
  ASSERT_NE(test_policy, nullptr);

  // Check that the source is properly labeled as "policySourceBrave"
  // This verifies our injection into the policy source mapping array worked
  const std::string* source = test_policy->FindString("source");
  ASSERT_NE(source, nullptr);
  EXPECT_EQ(*source, "policySourceBrave");
}

// Test that the string resource is correctly defined
TEST_F(BravePolicyConversionsTest, BraveStringResourceExists) {
  // Verify that IDS_POLICY_SOURCE_BRAVE is defined and has expected
  // content This is a compile-time check that our string resource is available
  EXPECT_NE(IDS_POLICY_SOURCE_BRAVE, 0);
}

// Test that kPolicySources array has the correct entry for Brave
TEST_F(BravePolicyConversionsTest, BraveInPolicySourcesArray) {
  // Verify that the kPolicySources array has "policySourceBrave" at the correct
  // index
  EXPECT_LT(static_cast<int>(POLICY_SOURCE_BRAVE), POLICY_SOURCE_COUNT);
  EXPECT_STREQ(kPolicySources[POLICY_SOURCE_BRAVE].name, "policySourceBrave");
  EXPECT_EQ(kPolicySources[POLICY_SOURCE_BRAVE].id, IDS_POLICY_SOURCE_BRAVE);
}

// Test that GetSource function handles POLICY_SOURCE_BRAVE without
// hitting NOTREACHED()
TEST_F(BravePolicyConversionsTest, GetSourceHandlesBrave) {
  // Create a policy dictionary with POLICY_SOURCE_BRAVE
  base::DictValue test_policy;
  test_policy.Set("level", static_cast<int>(POLICY_LEVEL_MANDATORY));
  test_policy.Set("scope", static_cast<int>(POLICY_SCOPE_USER));
  test_policy.Set("source", static_cast<int>(POLICY_SOURCE_BRAVE));
  test_policy.Set("value", base::Value("test_value"));

  base::DictValue chrome_policies;
  chrome_policies.Set("TestBravePolicy", std::move(test_policy));

  base::DictValue policies;
  policies.Set("chromePolicies", std::move(chrome_policies));

  // Test that AppendChromePolicyInfoIntoProfileReport handles Brave
  // source without hitting NOTREACHED() in the GetSource function
  enterprise_management::ChromeUserProfileInfo profile_info;
  EXPECT_NO_FATAL_FAILURE(
      enterprise_reporting::AppendChromePolicyInfoIntoProfileReport(
          policies, &profile_info));

  // Verify the policy was processed successfully
  ASSERT_EQ(profile_info.chrome_policies_size(), 1);
  const auto& policy_proto = profile_info.chrome_policies(0);
  EXPECT_EQ(policy_proto.name(), "TestBravePolicy");
  EXPECT_EQ(policy_proto.source(),
            enterprise_management::Policy_PolicySource_SOURCE_UNKNOWN);
}

}  // namespace
}  // namespace policy
