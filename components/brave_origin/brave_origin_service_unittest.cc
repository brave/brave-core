/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_service.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_origin/pref_names.h"
#include "components/policy/core/common/mock_policy_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_origin {

// Test preference name constants
constexpr char kTestProfileId[] = "test-profile-id";
constexpr char kTestBrowserPref[] = "test.browser.pref";
constexpr char kTestProfilePref[] = "test.profile.pref";
constexpr char kTestBrowserNotUserSettable[] = "test.browser.not_user_settable";
constexpr char kTestProfileNotUserSettable[] = "test.profile.not_user_settable";
// Test policy key constants
constexpr char kTestBrowserPolicyKey[] = "test.browser.pref.policy";
constexpr char kTestProfilePolicyKey[] = "test.profile.pref.policy";
constexpr char kUnknownPolicyKey[] = "unknown.policy.key";
constexpr char kTestBrowserNotUserSettablePolicyKey[] =
    "test.browser.not_user_settable.policy";
constexpr char kTestProfileNotUserSettablePolicyKey[] =
    "test.profile.not_user_settable.policy";

class BraveOriginServiceTest : public testing::Test {
 public:
  BraveOriginServiceTest() = default;
  ~BraveOriginServiceTest() override = default;

  void SetUp() override {
    // Enable the BraveOrigin feature for tests
    feature_list_.InitAndEnableFeature(features::kBraveOrigin);

    // Register the BraveOrigin policies dictionary pref in local_state
    local_state_.registry()->RegisterDictionaryPref(kBraveOriginPolicies);

    // Register test browser preferences in local_state
    // These are needed because BraveOriginService::SetBrowserPolicyValue()
    // calls local_state_->SetBoolean() on these prefs after updating the policy
    local_state_.registry()->RegisterBooleanPref(kTestBrowserPref, false);
    local_state_.registry()->RegisterBooleanPref(kTestBrowserNotUserSettable,
                                                 false);

    // Register test profile preferences in profile_prefs
    // These are needed because BraveOriginService::SetProfilePolicyValue()
    // calls profile_prefs_->SetBoolean() on these prefs after updating the
    // policy
    profile_prefs_.registry()->RegisterBooleanPref(kTestProfilePref, true);
    profile_prefs_.registry()->RegisterBooleanPref(kTestProfileNotUserSettable,
                                                   true);

    // Create test policies
    auto browser_policies = CreateBrowserTestPolicies();
    auto profile_policies = CreateProfileTestPolicies();

    // Initialize the policy manager
    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Init(std::move(browser_policies), std::move(profile_policies),
                  &local_state_);

    // Create the service with both policy services
    service_ = std::make_unique<BraveOriginService>(
        &local_state_, &profile_prefs_, kTestProfileId,
        &mock_profile_policy_service_, &mock_browser_policy_service_);
  }

  void TearDown() override {
    service_.reset();
    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Shutdown();
  }

 protected:
  void CreateTestPolicy(BraveOriginPolicyMap& policies,
                        const std::string& pref_name,
                        bool default_value,
                        bool user_settable) {
    std::string policy_key = base::StrCat({pref_name, ".policy"});
    policies.emplace(
        policy_key,
        BraveOriginPolicyInfo(pref_name, default_value, user_settable,
                              base::StrCat({pref_name, ".brave_origin_key"})));
  }

  BraveOriginPolicyMap CreateBrowserTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestBrowserPref, false, true);
    CreateTestPolicy(test_policies, kTestBrowserNotUserSettable, false, false);
    return test_policies;
  }

  BraveOriginPolicyMap CreateProfileTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestProfilePref, true, true);
    CreateTestPolicy(test_policies, kTestProfileNotUserSettable, true, false);
    return test_policies;
  }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  policy::MockPolicyService mock_profile_policy_service_;
  policy::MockPolicyService mock_browser_policy_service_;
  std::unique_ptr<BraveOriginService> service_;
};

TEST_F(BraveOriginServiceTest, SetPolicyValue_UserSettable_SetsPrefs) {
  // Set a user-settable browser policy value to true
  bool result = service_->SetPolicyValue(kTestBrowserPolicyKey, true);
  EXPECT_TRUE(result);

  // Should be set in both the policy manager and local_state
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestBrowserPolicyKey);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_TRUE(policy_value.value());

  bool local_state_value = local_state_.GetBoolean(kTestBrowserPref);
  EXPECT_TRUE(local_state_value);
}

TEST_F(BraveOriginServiceTest,
       SetPolicyValue_NotUserSettable_ClearsDefaultValue) {
  // Set a non-user-settable browser policy to its default value (false)
  bool result =
      service_->SetPolicyValue(kTestBrowserNotUserSettablePolicyKey, false);
  EXPECT_TRUE(result);

  // Should be set in policy manager
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestBrowserNotUserSettablePolicyKey);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());

  // Should clear the pref in local_state since it equals default and is not
  // user-settable
  EXPECT_FALSE(local_state_.HasPrefPath(kTestBrowserNotUserSettable));
}

TEST_F(BraveOriginServiceTest,
       SetPolicyValue_NotUserSettable_SetsNonDefaultValue) {
  // Set a non-user-settable browser policy to a non-default value (true,
  // default is false)
  bool result =
      service_->SetPolicyValue(kTestBrowserNotUserSettablePolicyKey, true);
  EXPECT_TRUE(result);

  // Should be set in both the policy manager and local_state
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestBrowserNotUserSettablePolicyKey);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_TRUE(policy_value.value());

  bool local_state_value = local_state_.GetBoolean(kTestBrowserNotUserSettable);
  EXPECT_TRUE(local_state_value);
}

TEST_F(BraveOriginServiceTest, SetPolicyValue_ProfilePref_SetsPrefs) {
  // Set a user-settable profile policy value to false
  bool result = service_->SetPolicyValue(kTestProfilePolicyKey, false);
  EXPECT_TRUE(result);

  // Should be set in both the policy manager and profile_prefs
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestProfilePolicyKey, kTestProfileId);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());

  bool profile_prefs_value = profile_prefs_.GetBoolean(kTestProfilePref);
  EXPECT_FALSE(profile_prefs_value);
}

TEST_F(BraveOriginServiceTest,
       SetPolicyValue_ProfileNotUserSettable_ClearsDefaultValue) {
  // Set a non-user-settable profile policy to its default value (true)
  bool result =
      service_->SetPolicyValue(kTestProfileNotUserSettablePolicyKey, true);
  EXPECT_TRUE(result);

  // Should be set in policy manager
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestProfileNotUserSettablePolicyKey, kTestProfileId);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_TRUE(policy_value.value());

  // Should clear the pref in profile_prefs since it equals default and is not
  // user-settable
  EXPECT_FALSE(profile_prefs_.HasPrefPath(kTestProfileNotUserSettable));
}

TEST_F(BraveOriginServiceTest,
       SetPolicyValue_ProfileNotUserSettable_SetsNonDefaultValue) {
  // Set a non-user-settable profile policy to a non-default value (false,
  // default is true)
  bool result =
      service_->SetPolicyValue(kTestProfileNotUserSettablePolicyKey, false);
  EXPECT_TRUE(result);

  // Should be set in both the policy manager and profile_prefs
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestProfileNotUserSettablePolicyKey, kTestProfileId);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());

  bool profile_prefs_value =
      profile_prefs_.GetBoolean(kTestProfileNotUserSettable);
  EXPECT_FALSE(profile_prefs_value);
}

TEST_F(BraveOriginServiceTest, SetPolicyValue_UnknownPref_ReturnsFalse) {
  bool result = service_->SetPolicyValue(kUnknownPolicyKey, true);
  EXPECT_FALSE(result);

  // Should not affect any prefs
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kUnknownPolicyKey);
  EXPECT_FALSE(policy_value.has_value());
}

TEST_F(BraveOriginServiceTest, GetPolicyValue_ReturnsValueFromPolicyManager) {
  // Set a value through the policy manager directly
  BraveOriginPolicyManager::GetInstance()->SetPolicyValue(kTestBrowserPolicyKey,
                                                          true);

  // Service should return the same value
  auto result = service_->GetPolicyValue(kTestBrowserPolicyKey);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value());
}

TEST_F(BraveOriginServiceTest,
       GetPolicyValue_ProfilePref_ReturnsValueFromPolicyManager) {
  // Set a value through the policy manager directly
  BraveOriginPolicyManager::GetInstance()->SetPolicyValue(
      kTestProfilePolicyKey, false, kTestProfileId);

  // Service should return the same value
  auto result = service_->GetPolicyValue(kTestProfilePolicyKey);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value());
}

TEST_F(BraveOriginServiceTest, GetPolicyValue_UnknownPref_ReturnsNullopt) {
  auto result = service_->GetPolicyValue(kUnknownPolicyKey);
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveOriginServiceTest, SetThenGet_BrowserPolicy_Consistency) {
  // Set via service
  bool set_result = service_->SetPolicyValue(kTestBrowserPolicyKey, true);
  EXPECT_TRUE(set_result);

  // Get via service
  auto get_result = service_->GetPolicyValue(kTestBrowserPolicyKey);
  ASSERT_TRUE(get_result.has_value());
  EXPECT_TRUE(get_result.value());

  // Change value
  set_result = service_->SetPolicyValue(kTestBrowserPolicyKey, false);
  EXPECT_TRUE(set_result);

  // Verify change
  get_result = service_->GetPolicyValue(kTestBrowserPolicyKey);
  ASSERT_TRUE(get_result.has_value());
  EXPECT_FALSE(get_result.value());
}

TEST_F(BraveOriginServiceTest, SetThenGet_ProfilePolicy_Consistency) {
  // Set via service
  bool set_result = service_->SetPolicyValue(kTestProfilePolicyKey, false);
  EXPECT_TRUE(set_result);

  // Get via service
  auto get_result = service_->GetPolicyValue(kTestProfilePolicyKey);
  ASSERT_TRUE(get_result.has_value());
  EXPECT_FALSE(get_result.value());

  // Change value
  set_result = service_->SetPolicyValue(kTestProfilePolicyKey, true);
  EXPECT_TRUE(set_result);

  // Verify change
  get_result = service_->GetPolicyValue(kTestProfilePolicyKey);
  ASSERT_TRUE(get_result.has_value());
  EXPECT_TRUE(get_result.value());
}

TEST_F(BraveOriginServiceTest, PolicyValueStoredInCorrectBraveOriginLocation) {
  // Set browser policy
  service_->SetPolicyValue(kTestBrowserPolicyKey, true);

  // Verify it's stored in the correct location in kBraveOriginPolicies
  const base::Value::Dict& policies_dict =
      local_state_.GetDict(kBraveOriginPolicies);

  // Get the expected key from the policy manager
  const auto* policy_info =
      BraveOriginPolicyManager::GetInstance()->GetPolicyInfo(
          kTestBrowserPolicyKey);
  ASSERT_NE(policy_info, nullptr);
  std::string expected_browser_key =
      GetBraveOriginPrefKey(kTestBrowserPolicyKey, std::nullopt);

  const base::Value* stored_value = policies_dict.Find(expected_browser_key);
  ASSERT_NE(stored_value, nullptr);
  EXPECT_TRUE(stored_value->GetBool());

  // Set profile policy
  service_->SetPolicyValue(kTestProfilePolicyKey, false);

  // Get the expected profile key
  const auto* profile_policy_info =
      BraveOriginPolicyManager::GetInstance()->GetPolicyInfo(
          kTestProfilePolicyKey);
  ASSERT_NE(profile_policy_info, nullptr);
  std::string expected_profile_key =
      GetBraveOriginPrefKey(kTestProfilePolicyKey, kTestProfileId);

  // Verify it's stored with profile-scoped key
  stored_value = policies_dict.Find(expected_profile_key);
  ASSERT_NE(stored_value, nullptr);
  EXPECT_FALSE(stored_value->GetBool());
}

TEST_F(BraveOriginServiceTest, ClearPrefBehavior_NotUserSettableWithDefault) {
  // Initially, the pref should not exist in local_state
  EXPECT_FALSE(local_state_.HasPrefPath(kTestBrowserNotUserSettable));

  // Set it to default value (false) - should clear the pref
  service_->SetPolicyValue(kTestBrowserNotUserSettablePolicyKey, false);

  // Pref should still not exist in local_state
  EXPECT_FALSE(local_state_.HasPrefPath(kTestBrowserNotUserSettable));

  // But should exist in policy manager
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestBrowserNotUserSettablePolicyKey);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());
}

TEST_F(BraveOriginServiceTest, SetPrefBehavior_NotUserSettableWithNonDefault) {
  // Set it to non-default value (true, default is false) - should set the pref
  service_->SetPolicyValue(kTestBrowserNotUserSettablePolicyKey, true);

  // Pref should exist in local_state
  EXPECT_TRUE(local_state_.HasPrefPath(kTestBrowserNotUserSettable));
  EXPECT_TRUE(local_state_.GetBoolean(kTestBrowserNotUserSettable));

  // And should exist in policy manager
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestBrowserNotUserSettablePolicyKey);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_TRUE(policy_value.value());
}

TEST_F(BraveOriginServiceTest,
       IsPolicyControlledByBraveOrigin_BrowserPolicyService_ReturnsTrue) {
  // Set up mock browser policy service to return a PolicyMap with the test
  // policy
  policy::PolicyMap browser_policy_map;
  browser_policy_map.Set(kTestBrowserPolicyKey, policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                         base::Value(true), nullptr);

  // Set up empty profile policy service
  policy::PolicyMap empty_profile_policy_map;

  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(browser_policy_map));
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_profile_policy_map));

  bool result =
      service_->IsPolicyControlledByBraveOrigin(kTestBrowserPolicyKey);
  EXPECT_TRUE(result);
}

TEST_F(BraveOriginServiceTest,
       IsPolicyControlledByBraveOrigin_ProfilePolicyService_ReturnsTrue) {
  // Set up mock profile policy service to return a PolicyMap with the test
  // policy
  policy::PolicyMap profile_policy_map;
  profile_policy_map.Set(kTestProfilePolicyKey, policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                         base::Value(true), nullptr);

  // Set up empty browser policy service
  policy::PolicyMap empty_browser_policy_map;

  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(profile_policy_map));
  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));

  bool result =
      service_->IsPolicyControlledByBraveOrigin(kTestProfilePolicyKey);
  EXPECT_TRUE(result);
}

TEST_F(BraveOriginServiceTest,
       IsPolicyControlledByBraveOrigin_BothPolicyServices_ReturnsTrue) {
  // Set up both policy services to have policies (browser service takes
  // precedence)
  policy::PolicyMap browser_policy_map;
  browser_policy_map.Set(kTestBrowserPolicyKey, policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                         base::Value(true), nullptr);

  policy::PolicyMap profile_policy_map;
  profile_policy_map.Set(kTestBrowserPolicyKey, policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
                         base::Value(false), nullptr);

  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(browser_policy_map));
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(profile_policy_map));

  bool result =
      service_->IsPolicyControlledByBraveOrigin(kTestBrowserPolicyKey);
  EXPECT_TRUE(result);  // Should return true because browser service has
                        // POLICY_SOURCE_BRAVE
}

TEST_F(BraveOriginServiceTest,
       IsPolicyControlledByBraveOrigin_NeitherService_ReturnsFalse) {
  // Set up both policy services with non-BRAVE source policies
  policy::PolicyMap browser_policy_map;
  browser_policy_map.Set(kTestBrowserPolicyKey, policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
                         base::Value(true), nullptr);

  policy::PolicyMap profile_policy_map;
  profile_policy_map.Set(kTestBrowserPolicyKey, policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER,
                         policy::POLICY_SOURCE_PLATFORM, base::Value(false),
                         nullptr);

  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(browser_policy_map));
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(profile_policy_map));

  bool result =
      service_->IsPolicyControlledByBraveOrigin(kTestBrowserPolicyKey);
  EXPECT_FALSE(
      result);  // Should return false because neither has POLICY_SOURCE_BRAVE
}

TEST_F(BraveOriginServiceTest,
       IsPolicyControlledByBraveOrigin_EmptyPolicyServices_ReturnsFalse) {
  // Set up both policy services to return empty policy maps
  policy::PolicyMap empty_browser_policy_map;
  policy::PolicyMap empty_profile_policy_map;

  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_profile_policy_map));

  bool result =
      service_->IsPolicyControlledByBraveOrigin(kTestBrowserPolicyKey);
  EXPECT_FALSE(result);
}

// Test class for when BraveOrigin feature is disabled
class BraveOriginServiceDisabledTest : public testing::Test {
 public:
  BraveOriginServiceDisabledTest() = default;
  ~BraveOriginServiceDisabledTest() override = default;

  void SetUp() override {
    // Explicitly disable the BraveOrigin feature for these tests
    feature_list_.InitAndDisableFeature(features::kBraveOrigin);

    // Register the BraveOrigin policies dictionary pref in local_state
    local_state_.registry()->RegisterDictionaryPref(kBraveOriginPolicies);

    // Register test preferences (needed for pref service not to crash)
    local_state_.registry()->RegisterBooleanPref(kTestBrowserPref, false);
    profile_prefs_.registry()->RegisterBooleanPref(kTestProfilePref, true);

    // Create test policies and initialize policy manager
    auto browser_policies = CreateBrowserTestPolicies();
    auto profile_policies = CreateProfileTestPolicies();

    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Init(std::move(browser_policies), std::move(profile_policies),
                  &local_state_);

    // Create the service with both policy services
    service_ = std::make_unique<BraveOriginService>(
        &local_state_, &profile_prefs_, kTestProfileId,
        &mock_profile_policy_service_, &mock_browser_policy_service_);
  }

  void TearDown() override {
    service_.reset();
    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Shutdown();
  }

 protected:
  void CreateTestPolicy(BraveOriginPolicyMap& policies,
                        const std::string& pref_name,
                        bool default_value,
                        bool user_settable) {
    std::string policy_key = base::StrCat({pref_name, ".policy"});
    policies.emplace(
        policy_key,
        BraveOriginPolicyInfo(pref_name, default_value, user_settable,
                              base::StrCat({pref_name, ".brave_origin_key"})));
  }

  BraveOriginPolicyMap CreateBrowserTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestBrowserPref, false, true);
    return test_policies;
  }

  BraveOriginPolicyMap CreateProfileTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestProfilePref, true, true);
    return test_policies;
  }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  policy::MockPolicyService mock_profile_policy_service_;
  policy::MockPolicyService mock_browser_policy_service_;
  std::unique_ptr<BraveOriginService> service_;
};

TEST_F(BraveOriginServiceDisabledTest,
       SetBrowserPolicyValue_FeatureDisabled_ReturnsFalse) {
  // When feature is disabled, setting values should return false
  bool result = service_->SetPolicyValue(kTestBrowserPolicyKey, true);
  EXPECT_FALSE(result);

  // Should not affect policy manager values
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestBrowserPolicyKey);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());  // Should remain default value

  // Should not affect user prefs
  bool local_state_value = local_state_.GetBoolean(kTestBrowserPref);
  EXPECT_FALSE(local_state_value);  // Should remain default value
}

TEST_F(BraveOriginServiceDisabledTest,
       SetProfilePolicyValue_FeatureDisabled_ReturnsFalse) {
  // When feature is disabled, setting values should return false
  bool result = service_->SetPolicyValue(kTestProfilePolicyKey, false);
  EXPECT_FALSE(result);

  // Should not affect policy manager values
  auto policy_value = BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
      kTestProfilePolicyKey, kTestProfileId);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_TRUE(policy_value.value());  // Should remain default value

  // Should not affect user prefs
  bool profile_prefs_value = profile_prefs_.GetBoolean(kTestProfilePref);
  EXPECT_TRUE(profile_prefs_value);  // Should remain default value
}

TEST_F(BraveOriginServiceDisabledTest,
       GetBrowserPolicyValue_FeatureDisabled_ReturnsDefault) {
  // Even when feature is disabled, get operations should still work and return
  // defaults
  auto result = service_->GetPolicyValue(kTestBrowserPolicyKey);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value());  // Should return default value
}

TEST_F(BraveOriginServiceDisabledTest,
       GetProfilePolicyValue_FeatureDisabled_ReturnsDefault) {
  // Even when feature is disabled, get operations should still work and return
  // defaults
  auto result = service_->GetPolicyValue(kTestProfilePolicyKey);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value());  // Should return default value
}

TEST_F(BraveOriginServiceDisabledTest, FeatureDisabled_NoSideEffects) {
  // Verify that attempting operations when disabled doesn't leave side effects

  // Initial state - no policies set
  const base::Value::Dict& policies_dict =
      local_state_.GetDict(kBraveOriginPolicies);
  EXPECT_TRUE(policies_dict.empty());

  // Attempt to set values (should fail)
  bool browser_result = service_->SetPolicyValue(kTestBrowserPolicyKey, true);
  bool profile_result = service_->SetPolicyValue(kTestProfilePolicyKey, false);
  EXPECT_FALSE(browser_result);
  EXPECT_FALSE(profile_result);

  // Verify no policies were actually stored
  EXPECT_TRUE(policies_dict.empty());

  // Verify user prefs remain at default values
  EXPECT_FALSE(local_state_.GetBoolean(kTestBrowserPref));
  EXPECT_TRUE(profile_prefs_.GetBoolean(kTestProfilePref));
}

}  // namespace brave_origin
