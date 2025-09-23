/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_policy_manager.h"

#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_origin/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_origin {

// Test preference name constants
constexpr char kTestGlobalPref[] = "test.global.pref";
constexpr char kTestProfilePref[] = "test.profile.pref";
constexpr char kUnknownPref[] = "unknown.pref";
constexpr char kBrowserPolicyKey[] = "Testtest.global.prefPolicy";
constexpr char kProfilePolicyKey[] = "Testtest.profile.prefPolicy";

class BraveOriginPolicyManagerTest : public testing::Test {
 public:
  BraveOriginPolicyManagerTest() = default;
  ~BraveOriginPolicyManagerTest() override = default;

  void SetUp() override {
    pref_service_.registry()->RegisterDictionaryPref(kBraveOriginPolicies);
  }

  void TearDown() override {
    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Shutdown();
  }

 protected:
  void CreateTestPolicy(BraveOriginPolicyMap& policies,
                        const std::string& pref_name,
                        bool default_value) {
    policies.emplace(
        pref_name, BraveOriginPolicyInfo(
                       pref_name, default_value, true,
                       base::StrCat({"Test", pref_name, "Policy"}), pref_name));
  }

  BraveOriginPolicyMap CreateBrowserTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestGlobalPref, false);
    return test_policies;
  }

  BraveOriginPolicyMap CreateProfileTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestProfilePref, true);
    return test_policies;
  }

  void InitializeManager() {
    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Init(CreateBrowserTestPolicies(), CreateProfileTestPolicies(),
                  &pref_service_);
  }

  void SetPolicyInLocalState(const std::string& key, bool value) {
    base::Value::Dict policies_dict =
        pref_service_.GetDict(kBraveOriginPolicies).Clone();
    policies_dict.Set(key, value);
    pref_service_.SetDict(kBraveOriginPolicies, std::move(policies_dict));
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
};

class TestObserver : public BraveOriginPolicyManager::Observer {
 public:
  void OnBraveOriginPoliciesReady() override { ready_called_ = true; }

  bool ready_called() const { return ready_called_; }

 private:
  bool ready_called_ = false;
};

TEST_F(BraveOriginPolicyManagerTest, GetInstance_ReturnsSingleton) {
  auto* instance1 = BraveOriginPolicyManager::GetInstance();
  auto* instance2 = BraveOriginPolicyManager::GetInstance();

  EXPECT_EQ(instance1, instance2);
  EXPECT_NE(instance1, nullptr);
}

TEST_F(BraveOriginPolicyManagerTest, IsInitialized_InitiallyFalse) {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  EXPECT_FALSE(manager->IsInitialized());
}

TEST_F(BraveOriginPolicyManagerTest, Init_SetsInitializedState) {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  manager->Init(CreateBrowserTestPolicies(), CreateProfileTestPolicies(),
                &pref_service_);

  EXPECT_TRUE(manager->IsInitialized());
}

TEST_F(BraveOriginPolicyManagerTest, Init_NotifiesObservers) {
  TestObserver observer;
  auto* manager = BraveOriginPolicyManager::GetInstance();

  manager->AddObserver(&observer);
  EXPECT_FALSE(observer.ready_called());

  manager->Init(CreateBrowserTestPolicies(), CreateProfileTestPolicies(),
                &pref_service_);
  EXPECT_TRUE(observer.ready_called());

  manager->RemoveObserver(&observer);
}

TEST_F(BraveOriginPolicyManagerTest,
       AddObserver_NotifiesImmediatelyIfInitialized) {
  InitializeManager();

  TestObserver observer;
  auto* manager = BraveOriginPolicyManager::GetInstance();
  manager->AddObserver(&observer);

  // Should be called immediately since manager is already initialized
  EXPECT_TRUE(observer.ready_called());

  manager->RemoveObserver(&observer);
}

TEST_F(BraveOriginPolicyManagerTest,
       GetPolicyValue_ReturnsDefaultForUnknownPref) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  auto result = manager->GetPolicyValue(kUnknownPref);
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveOriginPolicyManagerTest, GetPolicyValue_ReturnsDefaultValue) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  // Test global pref default
  auto result = manager->GetPolicyValue(kTestGlobalPref);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value());

  // Test profile pref default
  result = manager->GetPolicyValue(kTestProfilePref, "profile123");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value());
}

TEST_F(BraveOriginPolicyManagerTest, GetPolicyValue_ReturnsLocalStateValue) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  // Set policy value in local state
  const auto* policy_info = manager->GetPrefInfo(kTestGlobalPref);
  ASSERT_NE(policy_info, nullptr);
  std::string policy_key = GetBraveOriginBrowserPrefKey(*policy_info);
  SetPolicyInLocalState(policy_key, true);

  auto result = manager->GetPolicyValue(kTestGlobalPref);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value());  // Should return local state value, not default
}

TEST_F(BraveOriginPolicyManagerTest, GetPolicyValue_ProfileScopedPolicy) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  const std::string profile_id = "profile123";
  const auto* policy_info = manager->GetPrefInfo(kTestProfilePref);
  ASSERT_NE(policy_info, nullptr);
  std::string policy_key =
      GetBraveOriginProfilePrefKey(*policy_info, profile_id);
  SetPolicyInLocalState(policy_key, false);

  auto result = manager->GetPolicyValue(kTestProfilePref, profile_id);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value());  // Should return local state value, not default
}

TEST_F(BraveOriginPolicyManagerTest, GetPolicyValue_RequiresInitialization) {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  // Should crash/check fail when not initialized
  EXPECT_DEATH_IF_SUPPORTED(manager->GetPolicyValue(kTestGlobalPref), "");
}

TEST_F(BraveOriginPolicyManagerTest,
       GetAllBrowserPolicies_ReturnsOnlyBrowserPolicies) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  const auto browser_policies = manager->GetAllBrowserPolicies();
  EXPECT_EQ(browser_policies.size(), 1u);
  EXPECT_TRUE(browser_policies.contains(kBrowserPolicyKey));
  EXPECT_FALSE(browser_policies.contains(kProfilePolicyKey));
  // Test that we get the actual policy value (default false)
  EXPECT_FALSE(browser_policies.at(kBrowserPolicyKey));
}

TEST_F(BraveOriginPolicyManagerTest,
       GetAllProfilePolicies_ReturnsOnlyProfilePolicies) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  const auto profile_policies = manager->GetAllProfilePolicies("test_profile");
  EXPECT_EQ(profile_policies.size(), 1u);
  EXPECT_TRUE(profile_policies.contains(kProfilePolicyKey));
  EXPECT_FALSE(profile_policies.contains(kBrowserPolicyKey));
  // Test that we get the actual policy value (default true)
  EXPECT_TRUE(profile_policies.at(kProfilePolicyKey));
}

TEST_F(BraveOriginPolicyManagerTest,
       GetAllBrowserPolicies_RequiresInitialization) {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  // Should crash/check fail when not initialized
  EXPECT_DEATH_IF_SUPPORTED(manager->GetAllBrowserPolicies(), "");
}

TEST_F(BraveOriginPolicyManagerTest,
       GetAllProfilePolicies_RequiresInitialization) {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  // Should crash/check fail when not initialized
  EXPECT_DEATH_IF_SUPPORTED(manager->GetAllProfilePolicies("test"), "");
}

TEST_F(BraveOriginPolicyManagerTest, GetPrefInfo_ReturnsValidInfo) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  const auto* info = manager->GetPrefInfo(kTestGlobalPref);
  ASSERT_NE(info, nullptr);
  EXPECT_FALSE(info->default_value);
  EXPECT_EQ(info->pref_name, kTestGlobalPref);
}

TEST_F(BraveOriginPolicyManagerTest, GetPrefInfo_ReturnsNullForUnknown) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  const auto* info = manager->GetPrefInfo(kUnknownPref);
  EXPECT_EQ(info, nullptr);
}

TEST_F(BraveOriginPolicyManagerTest, MultipleObservers_AllNotified) {
  TestObserver observer1, observer2, observer3;
  auto* manager = BraveOriginPolicyManager::GetInstance();

  manager->AddObserver(&observer1);
  manager->AddObserver(&observer2);
  manager->AddObserver(&observer3);

  manager->Init(CreateBrowserTestPolicies(), CreateProfileTestPolicies(),
                &pref_service_);

  EXPECT_TRUE(observer1.ready_called());
  EXPECT_TRUE(observer2.ready_called());
  EXPECT_TRUE(observer3.ready_called());

  manager->RemoveObserver(&observer1);
  manager->RemoveObserver(&observer2);
  manager->RemoveObserver(&observer3);
}

TEST_F(BraveOriginPolicyManagerTest, RemoveObserver_StopsNotifications) {
  TestObserver observer;
  auto* manager = BraveOriginPolicyManager::GetInstance();

  manager->AddObserver(&observer);
  manager->RemoveObserver(&observer);

  manager->Init(CreateBrowserTestPolicies(), CreateProfileTestPolicies(),
                &pref_service_);

  // Should not be notified since observer was removed
  EXPECT_FALSE(observer.ready_called());
}

TEST_F(BraveOriginPolicyManagerTest, SetBrowserPolicyValue_UpdatesValue) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();

  // Initially should return default value (false)
  auto result = manager->GetPolicyValue(kTestGlobalPref);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value());

  // Set the value to true
  manager->SetBrowserPolicyValue(kTestGlobalPref, true);

  // Should now return the updated value
  result = manager->GetPolicyValue(kTestGlobalPref);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value());

  // Set the value back to false
  manager->SetBrowserPolicyValue(kTestGlobalPref, false);

  // Should return the updated value
  result = manager->GetPolicyValue(kTestGlobalPref);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value());
}

TEST_F(BraveOriginPolicyManagerTest, SetProfilePolicyValue_UpdatesValue) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  const std::string profile_id = "test-profile";

  // Initially should return default value (true)
  auto result = manager->GetPolicyValue(kTestProfilePref, profile_id);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value());

  // Set the value to false
  manager->SetProfilePolicyValue(kTestProfilePref, false, profile_id);

  // Should now return the updated value
  result = manager->GetPolicyValue(kTestProfilePref, profile_id);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value());

  // Set the value back to true
  manager->SetProfilePolicyValue(kTestProfilePref, true, profile_id);

  // Should return the updated value
  result = manager->GetPolicyValue(kTestProfilePref, profile_id);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value());
}

TEST_F(BraveOriginPolicyManagerTest,
       SetBrowserPolicyValue_RequiresInitialization) {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  // Should crash/check fail when not initialized
  EXPECT_DEATH_IF_SUPPORTED(
      manager->SetBrowserPolicyValue(kTestGlobalPref, true), "");
}

TEST_F(BraveOriginPolicyManagerTest,
       SetProfilePolicyValue_RequiresInitialization) {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  // Should crash/check fail when not initialized
  EXPECT_DEATH_IF_SUPPORTED(
      manager->SetProfilePolicyValue(kTestProfilePref, true, "profile"), "");
}

TEST_F(BraveOriginPolicyManagerTest, SetBrowserPolicyValue_RejectsUnknownPref) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();

  // Setting unknown pref should not crash but should be ignored
  manager->SetBrowserPolicyValue(kUnknownPref, true);

  // Verify it's still unknown
  auto result = manager->GetPolicyValue(kUnknownPref);
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveOriginPolicyManagerTest, SetProfilePolicyValue_RejectsUnknownPref) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();

  // Setting unknown pref should not crash but should be ignored
  manager->SetProfilePolicyValue(kUnknownPref, true, "profile");

  // Verify it's still unknown
  auto result = manager->GetPolicyValue(kUnknownPref, "profile");
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveOriginPolicyManagerTest,
       SetProfilePolicyValue_RequiresNonEmptyProfileId) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();

  // Should crash/check fail with empty profile ID
  EXPECT_DEATH_IF_SUPPORTED(
      manager->SetProfilePolicyValue(kTestProfilePref, true, ""), "");
}

TEST_F(BraveOriginPolicyManagerTest, SetValues_UpdatesGetAllPoliciesMethods) {
  InitializeManager();

  auto* manager = BraveOriginPolicyManager::GetInstance();
  const std::string profile_id = "test-profile";

  // Set browser policy value
  manager->SetBrowserPolicyValue(kTestGlobalPref, true);

  // Set profile policy value
  manager->SetProfilePolicyValue(kTestProfilePref, false, profile_id);

  // Verify GetAllBrowserPolicies reflects the change
  const auto browser_policies = manager->GetAllBrowserPolicies();
  EXPECT_EQ(browser_policies.size(), 1u);
  EXPECT_TRUE(browser_policies.contains(kBrowserPolicyKey));
  EXPECT_TRUE(browser_policies.at(kBrowserPolicyKey));

  // Verify GetAllProfilePolicies reflects the change
  const auto profile_policies = manager->GetAllProfilePolicies(profile_id);
  EXPECT_EQ(profile_policies.size(), 1u);
  EXPECT_TRUE(profile_policies.contains(kProfilePolicyKey));
  EXPECT_FALSE(profile_policies.at(kProfilePolicyKey));
}

}  // namespace brave_origin
