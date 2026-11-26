// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/browser/psst/psst_ui_presenter.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/psst/core/browser/pref_names.h"
#include "brave/components/psst/core/browser/psst_report_uploader.h"
#include "brave/components/psst/core/browser/psst_reporter_service.h"
#include "brave/components/psst/core/browser/psst_settings_service.h"
#include "brave/components/psst/core/common/features.h"
#include "brave/components/skus/browser/pref_names.h"
#include "build/build_config.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/policy/core/common/mock_policy_service.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {

constexpr char kTestProfileId[] = "test-profile-id";

// Minimal delegate: these tests don't exercise SKU connectivity.
class NullBraveOriginDelegate
    : public brave_origin::BraveOriginService::Delegate {
 public:
  void OpenOriginSettings() override {}
  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() override {
    return {};
  }
};

// No-op presenter: these tests drive PsstUiDelegateImpl's private handlers
// directly rather than through the presenter's UI callbacks.
class NoopUiPresenter : public PsstUiPresenter {
 public:
  void SetLocationBarIconStatus(LocationBarIconStatus status,
                                LocationBarMenuCallback,
                                LocationBarMenuCallback) override {}
  void ShowInfoBar(InfoBarCallback) override {}
  void HideInfoBar() override {}
  void ShowConsentDialog() override {}
  void HideConsentDialog() override {}
  bool IsDialogShown() const override { return false; }
  void HideAll() override {}
};

}  // namespace

class PsstUiDelegateImplUnitTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(brave_origin::features::kBraveOrigin);

    local_state_.registry()->RegisterDictionaryPref(
        brave_origin::kBraveOriginPolicies);
    local_state_.registry()->RegisterBooleanPref(
        brave_origin::kOriginPurchaseValidated, false);
    local_state_.registry()->RegisterBooleanPref(
        brave_origin::kOriginPoliciesWereEnforced, false);
    local_state_.registry()->RegisterDictionaryPref(skus::prefs::kSkusState);
#if BUILDFLAG(IS_LINUX)
    local_state_.registry()->RegisterBooleanPref(
        brave_origin::kOriginFreeTierAccepted, false);
#endif

    HostContentSettingsMap::RegisterProfilePrefs(profile_prefs_.registry());
    psst::RegisterProfilePrefs(profile_prefs_.registry());
    map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &profile_prefs_, /*is_off_the_record=*/false,
        /*store_last_modified=*/false, /*restore_session=*/false,
        /*should_record_metrics=*/false);

    brave_origin::BraveOriginPolicyMap browser_policies;
    brave_origin::BraveOriginPolicyMap profile_policies;
    profile_policies.emplace(
        policy::key::kPsstEnabled,
        brave_origin::BraveOriginPolicyInfo(
            prefs::kPsstEnabled, /*default_value=*/false,
            /*user_settable=*/false, "psst_enabled.brave_origin_key"));

    auto* manager = brave_origin::BraveOriginPolicyManager::GetInstance();
    manager->Init(std::move(browser_policies), std::move(profile_policies),
                  &local_state_);
    manager->SetPurchased(true);
    local_state_.SetBoolean(brave_origin::kOriginPoliciesWereEnforced, true);

    brave_origin_service_ = std::make_unique<brave_origin::BraveOriginService>(
        &local_state_, &profile_prefs_, kTestProfileId,
        &mock_profile_policy_service_, &mock_browser_policy_service_,
        std::make_unique<NullBraveOriginDelegate>());

    psst_settings_service_ =
        std::make_unique<PsstSettingsService>(*map_, &profile_prefs_);
    psst_reporter_service_ = std::make_unique<PsstReporterService>(
        base::BindRepeating([] { return std::string(); }),
        base::BindRepeating(
            [] { return std::optional<std::string>(std::nullopt); }),
        std::make_unique<PsstErrorReportUploader>(nullptr));

    delegate_ = std::make_unique<PsstUiDelegateImpl>(
        psst_settings_service_.get(), psst_reporter_service_.get(),
        std::make_unique<NoopUiPresenter>(), brave_origin_service_.get());
  }

  void TearDown() override {
    delegate_.reset();
    psst_reporter_service_.reset();
    psst_settings_service_.reset();
    brave_origin_service_.reset();
    map_->ShutdownOnUIThread();
    brave_origin::BraveOriginPolicyManager::GetInstance()->Shutdown();
  }

 protected:
  PsstUiDelegateImpl* delegate() { return delegate_.get(); }

  void TriggerDisablePrivacySettingsTuning() {
    delegate_->OnDisablePrivacySettingsTuning();
  }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  scoped_refptr<HostContentSettingsMap> map_;
  policy::MockPolicyService mock_profile_policy_service_;
  policy::MockPolicyService mock_browser_policy_service_;
  std::unique_ptr<brave_origin::BraveOriginService> brave_origin_service_;
  std::unique_ptr<PsstSettingsService> psst_settings_service_;
  std::unique_ptr<PsstReporterService> psst_reporter_service_;
  std::unique_ptr<PsstUiDelegateImpl> delegate_;
};

TEST_F(
    PsstUiDelegateImplUnitTest,
    OnDisablePrivacySettingsTuning_ControlledByBraveOrigin_UpdatesPolicyManagerValue) {
  policy::PolicyMap profile_policy_map;
  profile_policy_map.Set(policy::key::kPsstEnabled,
                         policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                         base::Value(true), nullptr);
  policy::PolicyMap empty_browser_policy_map;
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(profile_policy_map));
  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));
  ASSERT_TRUE(brave_origin_service_->IsPolicyControlledByBraveOrigin(
      policy::key::kPsstEnabled));

  TriggerDisablePrivacySettingsTuning();

  auto policy_value =
      brave_origin::BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
          policy::key::kPsstEnabled, kTestProfileId);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());
  EXPECT_FALSE(delegate()->IsPsstEnabled());
}

TEST_F(PsstUiDelegateImplUnitTest,
       IsPsstEnabled_ControlledByBraveOrigin_ReadsPolicyManagerNotRawPref) {
  policy::PolicyMap profile_policy_map;
  profile_policy_map.Set(policy::key::kPsstEnabled,
                         policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                         base::Value(true), nullptr);
  policy::PolicyMap empty_browser_policy_map;
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(profile_policy_map));
  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));

  // Deliberately make the raw pref disagree with the BraveOrigin policy
  // manager value, to prove IsPsstEnabled() consults the policy manager
  // rather than just reading the pref.
  profile_prefs_.SetBoolean(prefs::kPsstEnabled, true);
  brave_origin::BraveOriginPolicyManager::GetInstance()->SetPolicyValue(
      policy::key::kPsstEnabled, false, kTestProfileId);

  EXPECT_FALSE(delegate()->IsPsstEnabled());
}

TEST_F(
    PsstUiDelegateImplUnitTest,
    OnDisablePrivacySettingsTuning_NotControlledByBraveOrigin_WritesPrefDirectly) {
  policy::PolicyMap empty_profile_policy_map;
  policy::PolicyMap empty_browser_policy_map;
  EXPECT_CALL(mock_profile_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_profile_policy_map));
  EXPECT_CALL(mock_browser_policy_service_, GetPolicies(testing::_))
      .WillRepeatedly(testing::ReturnRef(empty_browser_policy_map));
  ASSERT_FALSE(brave_origin_service_->IsPolicyControlledByBraveOrigin(
      policy::key::kPsstEnabled));

  psst_settings_service_->SetPsstEnabled(true);
  ASSERT_TRUE(delegate()->IsPsstEnabled());

  TriggerDisablePrivacySettingsTuning();

  EXPECT_FALSE(delegate()->IsPsstEnabled());
  // BraveOrigin's policy manager must stay untouched -- the write went
  // straight to the pref since the policy isn't BraveOrigin-controlled.
  auto policy_value =
      brave_origin::BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
          policy::key::kPsstEnabled, kTestProfileId);
  ASSERT_TRUE(policy_value.has_value());
  EXPECT_FALSE(policy_value.value());  // still the registered default
}

}  // namespace psst
