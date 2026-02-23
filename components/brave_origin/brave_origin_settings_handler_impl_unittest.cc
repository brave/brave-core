/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_settings_handler_impl.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/skus/browser/test/fake_skus_service.h"
#include "build/build_config.h"
#include "components/policy/core/common/mock_policy_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_origin {

using skus::FakeSkusService;

// Test constants
constexpr char kTestProfileId[] = "test-profile-id";
constexpr char kTestBrowserPrefName[] = "test.browser.pref";
constexpr char kTestProfilePrefName[] = "test.profile.pref";
// Policy keys (generated from pref names)
constexpr char kTestBrowserPolicyKey[] = "test.browser.pref.policy";
constexpr char kTestProfilePolicyKey[] = "test.profile.pref.policy";
constexpr char kUnknownPolicyKey[] = "unknown.pref.policy";

class BraveOriginHandlerTest : public testing::Test {
 public:
  BraveOriginHandlerTest() = default;
  ~BraveOriginHandlerTest() override = default;

  void SetUp() override {
    // Enable the BraveOrigin feature for tests
    feature_list_.InitAndEnableFeature(features::kBraveOrigin);

    // Register the BraveOrigin policies dictionary pref in local_state
    local_state_.registry()->RegisterDictionaryPref(kBraveOriginPolicies);

    // Register test browser preferences in local_state
    local_state_.registry()->RegisterBooleanPref(kTestBrowserPrefName, false);

    // Register test profile preferences in profile_prefs
    profile_prefs_.registry()->RegisterBooleanPref(kTestProfilePrefName, true);

    // Create test policies
    auto browser_policies = CreateBrowserTestPolicies();
    auto profile_policies = CreateProfileTestPolicies();

    // Initialize the policy manager
    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Init(std::move(browser_policies), std::move(profile_policies),
                  &local_state_);
    // Set purchased state so IsBraveOriginEnabled() returns true when the
    // feature flag is enabled.
    manager->SetPurchased(true);

    // Create the service with both policy services
    service_ = std::make_unique<BraveOriginService>(
        &local_state_, &profile_prefs_, kTestProfileId, &mock_policy_service_,
        &mock_policy_service_, BraveOriginService::SkusServiceGetter());

    // Create the handler
    handler_ = std::make_unique<BraveOriginSettingsHandlerImpl>(service_.get());
  }

  void TearDown() override {
    handler_.reset();
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
    CreateTestPolicy(test_policies, kTestBrowserPrefName, false, true);
    return test_policies;
  }

  BraveOriginPolicyMap CreateProfileTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestProfilePrefName, true, true);
    return test_policies;
  }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  policy::MockPolicyService mock_policy_service_;
  std::unique_ptr<BraveOriginService> service_;
  std::unique_ptr<BraveOriginSettingsHandlerImpl> handler_;
};

// In branded builds, IsBraveOriginUser unconditionally returns true.
#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
TEST_F(BraveOriginHandlerTest,
       IsBraveOriginUser_FeatureEnabled_NoSkus_ReturnsFalse) {
  // Reset purchase state - without a SKU service and no prior purchase,
  // the user should not be considered a BraveOrigin user.
  BraveOriginPolicyManager::GetInstance()->SetPurchased(false);

  base::test::TestFuture<bool> result;
  handler_->IsBraveOriginUser(result.GetCallback());
  EXPECT_FALSE(result.Get());
}
#endif

TEST_F(BraveOriginHandlerTest,
       IsPolicyControlledByBraveOrigin_KnownPref_ReturnsTrue) {
  base::RunLoop run_loop;
  bool result = false;

  // Set up mock to return a PolicyMap with the test policy
  policy::PolicyMap policy_map;
  policy_map.Set(kTestBrowserPolicyKey, policy::POLICY_LEVEL_MANDATORY,
                 policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                 base::Value(true), nullptr);

  EXPECT_CALL(mock_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(policy_map));

  handler_->IsPolicyControlledByBraveOrigin(
      kTestBrowserPolicyKey,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool* result, bool is_controlled) {
            *result = is_controlled;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  EXPECT_TRUE(result);
}

TEST_F(BraveOriginHandlerTest,
       IsPolicyControlledByBraveOrigin_UnknownPref_ReturnsFalse) {
  base::RunLoop run_loop;
  bool result = true;

  handler_->IsPolicyControlledByBraveOrigin(
      kUnknownPolicyKey,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool* result, bool is_controlled) {
            *result = is_controlled;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  EXPECT_FALSE(result);
}

TEST_F(BraveOriginHandlerTest, GetPolicyValue_KnownPref_ReturnsValue) {
  // First set a value
  service_->SetPolicyValue(kTestBrowserPolicyKey, true);

  base::RunLoop run_loop;
  std::optional<bool> result;

  handler_->GetPolicyValue(
      kTestBrowserPolicyKey,
      base::BindOnce(
          [](base::RunLoop* run_loop, std::optional<bool>* result,
             std::optional<bool> value) {
            *result = value;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value());
}

TEST_F(BraveOriginHandlerTest, GetPolicyValue_UnknownPref_ReturnsNullopt) {
  base::RunLoop run_loop;
  std::optional<bool> result = true;  // Initialize with a value

  handler_->GetPolicyValue(
      kUnknownPolicyKey,
      base::BindOnce(
          [](base::RunLoop* run_loop, std::optional<bool>* result,
             std::optional<bool> value) {
            *result = value;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveOriginHandlerTest, GetPolicyValue_ProfilePref_ReturnsValue) {
  // First set a value
  service_->SetPolicyValue(kTestProfilePolicyKey, false);

  base::RunLoop run_loop;
  std::optional<bool> result;

  handler_->GetPolicyValue(
      kTestProfilePolicyKey,
      base::BindOnce(
          [](base::RunLoop* run_loop, std::optional<bool>* result,
             std::optional<bool> value) {
            *result = value;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value());
}

TEST_F(BraveOriginHandlerTest, SetPolicyValue_KnownPref_ReturnsTrue) {
  base::RunLoop run_loop;
  bool result = false;

  handler_->SetPolicyValue(
      kTestBrowserPolicyKey, true,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool* result, bool success) {
            *result = success;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  EXPECT_TRUE(result);

  // Verify the value was actually set
  auto policy_value = service_->GetPolicyValue(kTestBrowserPolicyKey);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_TRUE(policy_value.value());
}

TEST_F(BraveOriginHandlerTest, SetPolicyValue_UnknownPref_ReturnsFalse) {
  base::RunLoop run_loop;
  bool result = true;

  handler_->SetPolicyValue(
      kUnknownPolicyKey, true,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool* result, bool success) {
            *result = success;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  EXPECT_FALSE(result);
}

TEST_F(BraveOriginHandlerTest, SetPolicyValue_ProfilePref_ReturnsTrue) {
  base::RunLoop run_loop;
  bool result = false;

  handler_->SetPolicyValue(
      kTestProfilePolicyKey, false,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool* result, bool success) {
            *result = success;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  EXPECT_TRUE(result);

  // Verify the value was actually set
  auto policy_value = service_->GetPolicyValue(kTestProfilePolicyKey);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());
}

// In branded builds, RefreshPurchaseState unconditionally returns true.
#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
TEST_F(BraveOriginHandlerTest, RefreshPurchaseState_NoSkus_ReturnsFalse) {
  // Reset purchase state - without a SKU service and no prior purchase,
  // refresh should return false.
  BraveOriginPolicyManager::GetInstance()->SetPurchased(false);

  base::test::TestFuture<bool> result;
  handler_->RefreshPurchaseState(result.GetCallback());
  EXPECT_FALSE(result.Get());
}
#endif

// Test fixture for handler with a FakeSkusService wired up.
class BraveOriginHandlerWithSkusTest : public testing::Test {
 public:
  BraveOriginHandlerWithSkusTest() = default;
  ~BraveOriginHandlerWithSkusTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kBraveOrigin);

    local_state_.registry()->RegisterDictionaryPref(kBraveOriginPolicies);
    local_state_.registry()->RegisterBooleanPref(kTestBrowserPrefName, false);
    profile_prefs_.registry()->RegisterBooleanPref(kTestProfilePrefName, true);

    auto browser_policies = CreateBrowserTestPolicies();
    auto profile_policies = CreateProfileTestPolicies();
    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Init(std::move(browser_policies), std::move(profile_policies),
                  &local_state_);

    fake_skus_service_ = std::make_unique<FakeSkusService>();
    // Seed with a valid response so the constructor's eager
    // CheckPurchaseState completes with a deterministic result.
    fake_skus_service_->SetCredentialSummaryResponse(
        R"({"remaining_credential_count": 1})");
    auto getter = base::BindRepeating(
        &BraveOriginHandlerWithSkusTest::GetSkusServiceRemote,
        base::Unretained(this));
    service_ = std::make_unique<BraveOriginService>(
        &local_state_, &profile_prefs_, kTestProfileId, &mock_policy_service_,
        &mock_policy_service_, std::move(getter));
    // Wait for the constructor's eager CheckPurchaseState to complete.
    ASSERT_TRUE(base::test::RunUntil([&] { return service_->IsPurchased(); }));

    handler_ = std::make_unique<BraveOriginSettingsHandlerImpl>(service_.get());
  }

  void TearDown() override {
    handler_.reset();
    service_.reset();
    fake_skus_service_.reset();
    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Shutdown();
  }

 protected:
  mojo::PendingRemote<skus::mojom::SkusService> GetSkusServiceRemote() {
    return fake_skus_service_->MakeRemote();
  }

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
    CreateTestPolicy(test_policies, kTestBrowserPrefName, false, true);
    return test_policies;
  }

  BraveOriginPolicyMap CreateProfileTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestProfilePrefName, true, true);
    return test_policies;
  }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  policy::MockPolicyService mock_policy_service_;
  std::unique_ptr<FakeSkusService> fake_skus_service_;
  std::unique_ptr<BraveOriginService> service_;
  std::unique_ptr<BraveOriginSettingsHandlerImpl> handler_;
};

TEST_F(BraveOriginHandlerWithSkusTest,
       RefreshPurchaseState_WithCredentials_ReturnsTrue) {
  fake_skus_service_->SetCredentialSummaryResponse(
      R"({"remaining_credential_count": 5})");

  base::test::TestFuture<bool> result;
  handler_->RefreshPurchaseState(result.GetCallback());
  EXPECT_TRUE(result.Get());
}

// In branded builds, RefreshPurchaseState unconditionally returns true.
// On Linux, CheckPurchaseState always returns true (no SKU store).
#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED) && !BUILDFLAG(IS_LINUX)
TEST_F(BraveOriginHandlerWithSkusTest,
       RefreshPurchaseState_NoCredentials_ReturnsFalse) {
  fake_skus_service_->SetCredentialSummaryResponse(
      R"({"remaining_credential_count": 0, "expires_at": ""})");

  base::test::TestFuture<bool> result;
  handler_->RefreshPurchaseState(result.GetCallback());
  EXPECT_FALSE(result.Get());
}
#endif

TEST_F(BraveOriginHandlerWithSkusTest,
       IsBraveOriginUser_WithCredentials_ReturnsTrue) {
  fake_skus_service_->SetCredentialSummaryResponse(
      R"({"remaining_credential_count": 1})");

  base::test::TestFuture<bool> result;
  handler_->IsBraveOriginUser(result.GetCallback());
  EXPECT_TRUE(result.Get());
}

// Test class for when BraveOrigin feature is disabled
class BraveOriginHandlerDisabledTest : public testing::Test {
 public:
  BraveOriginHandlerDisabledTest() = default;
  ~BraveOriginHandlerDisabledTest() override = default;

  void SetUp() override {
    // Explicitly disable the BraveOrigin feature for these tests
    feature_list_.InitAndDisableFeature(features::kBraveOrigin);

    // Register the BraveOrigin policies dictionary pref in local_state
    local_state_.registry()->RegisterDictionaryPref(kBraveOriginPolicies);

    // Register test preferences (needed for pref service not to crash)
    local_state_.registry()->RegisterBooleanPref(kTestBrowserPrefName, false);
    profile_prefs_.registry()->RegisterBooleanPref(kTestProfilePrefName, true);

    // Create test policies and initialize policy manager
    auto browser_policies = CreateBrowserTestPolicies();
    auto profile_policies = CreateProfileTestPolicies();

    auto* manager = BraveOriginPolicyManager::GetInstance();
    manager->Init(std::move(browser_policies), std::move(profile_policies),
                  &local_state_);

    // Create the service with both policy services
    service_ = std::make_unique<BraveOriginService>(
        &local_state_, &profile_prefs_, kTestProfileId, &mock_policy_service_,
        &mock_policy_service_, BraveOriginService::SkusServiceGetter());

    // Create the handler
    handler_ = std::make_unique<BraveOriginSettingsHandlerImpl>(service_.get());
  }

  void TearDown() override {
    handler_.reset();
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
    CreateTestPolicy(test_policies, kTestBrowserPrefName, false, true);
    return test_policies;
  }

  BraveOriginPolicyMap CreateProfileTestPolicies() {
    BraveOriginPolicyMap test_policies;
    CreateTestPolicy(test_policies, kTestProfilePrefName, true, true);
    return test_policies;
  }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  policy::MockPolicyService mock_policy_service_;
  std::unique_ptr<BraveOriginService> service_;
  std::unique_ptr<BraveOriginSettingsHandlerImpl> handler_;
};

// In branded builds, IsBraveOriginUser unconditionally returns true.
#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
TEST_F(BraveOriginHandlerDisabledTest,
       IsBraveOriginUser_FeatureDisabled_ReturnsFalse) {
  base::RunLoop run_loop;
  bool result = true;

  handler_->IsBraveOriginUser(base::BindOnce(
      [](base::RunLoop* run_loop, bool* result, bool is_brave_origin_user) {
        *result = is_brave_origin_user;
        run_loop->Quit();
      },
      &run_loop, &result));

  run_loop.Run();
  EXPECT_FALSE(result);
}
#endif

TEST_F(BraveOriginHandlerDisabledTest,
       GetPolicyValue_FeatureDisabled_ReturnsNullopt) {
  base::RunLoop run_loop;
  std::optional<bool> result = true;  // Initialize with a value

  handler_->GetPolicyValue(
      kTestBrowserPolicyKey,
      base::BindOnce(
          [](base::RunLoop* run_loop, std::optional<bool>* result,
             std::optional<bool> value) {
            *result = value;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveOriginHandlerDisabledTest,
       SetPolicyValue_FeatureDisabled_ReturnsFalse) {
  base::RunLoop run_loop;
  bool result = true;

  handler_->SetPolicyValue(
      kTestBrowserPolicyKey, true,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool* result, bool success) {
            *result = success;
            run_loop->Quit();
          },
          &run_loop, &result));

  run_loop.Run();
  EXPECT_FALSE(result);
}

// In branded builds, RefreshPurchaseState unconditionally returns true.
#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
TEST_F(BraveOriginHandlerDisabledTest,
       RefreshPurchaseState_FeatureDisabled_ReturnsFalse) {
  base::test::TestFuture<bool> result;
  handler_->RefreshPurchaseState(result.GetCallback());
  EXPECT_FALSE(result.Get());
}
#endif

}  // namespace brave_origin
