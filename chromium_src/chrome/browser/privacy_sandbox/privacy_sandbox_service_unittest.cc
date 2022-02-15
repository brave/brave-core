// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/privacy_sandbox/privacy_sandbox_service.h"

#include "base/test/gtest_util.h"
#include "base/test/icu_test_util.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/metrics/user_action_tester.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_service.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/policy/core/common/mock_policy_service.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"
#include "components/privacy_sandbox/privacy_sandbox_settings.h"
#include "components/privacy_sandbox/privacy_sandbox_test_util.h"
#include "components/profile_metrics/browser_profile_type.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/signin/public/identity_manager/identity_test_environment.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync/base/user_selectable_type.h"
#include "components/sync/driver/test_sync_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/interest_group_manager.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace {

class TestInterestGroupManager : public content::InterestGroupManager {
 public:
  void SetInterestGroupJoiningOrigins(const std::vector<url::Origin>& origins) {
    origins_ = origins;
  }

  // content::InterestGroupManager:
  void GetAllInterestGroupJoiningOrigins(
      base::OnceCallback<void(std::vector<url::Origin>)> callback) override {
    std::move(callback).Run(origins_);
  }

 private:
  std::vector<url::Origin> origins_;
};

}  // namespace

class PrivacySandboxServiceTest : public testing::Test {
 public:
  PrivacySandboxServiceTest()
      : browser_task_environment_(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    InitializePrefsBeforeStart();

    privacy_sandbox_service_ = std::make_unique<PrivacySandboxService>(
        PrivacySandboxSettingsFactory::GetForProfile(profile()),
        CookieSettingsFactory::GetForProfile(profile()).get(),
        profile()->GetPrefs(), policy_service(), sync_service(),
        identity_test_env()->identity_manager(), test_interest_group_manager(),
        profile_metrics::BrowserProfileType::kRegular);
  }

  virtual void InitializePrefsBeforeStart() {}

  TestingProfile* profile() { return &profile_; }
  PrivacySandboxService* privacy_sandbox_service() {
    return privacy_sandbox_service_.get();
  }
  PrivacySandboxSettings* privacy_sandbox_settings() {
    return PrivacySandboxSettingsFactory::GetForProfile(profile());
  }

  base::test::ScopedFeatureList* feature_list() { return &feature_list_; }
  sync_preferences::TestingPrefServiceSyncable* prefs() {
    return profile()->GetTestingPrefService();
  }
  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(profile());
  }
  syncer::TestSyncService* sync_service() { return &sync_service_; }
  policy::MockPolicyService* policy_service() { return &mock_policy_service_; }
  signin::IdentityTestEnvironment* identity_test_env() {
    return &identity_test_env_;
  }
  TestInterestGroupManager* test_interest_group_manager() {
    return &test_interest_group_manager_;
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  signin::IdentityTestEnvironment identity_test_env_;
  testing::NiceMock<policy::MockPolicyService> mock_policy_service_;

  TestingProfile profile_;
  base::test::ScopedFeatureList feature_list_;
  syncer::TestSyncService sync_service_;
  TestInterestGroupManager test_interest_group_manager_;

  std::unique_ptr<PrivacySandboxService> privacy_sandbox_service_;
};

TEST_F(PrivacySandboxServiceTest, OnPrivacySandboxPrefChanged) {
  // When either the main Privacy Sandbox pref, or the FLoC pref, are changed
  // the FLoC ID should be reset. This will be propagated to the settings
  // instance, which should then notify observers.
  privacy_sandbox_test_util::MockPrivacySandboxObserver
      mock_privacy_sandbox_observer;
  PrivacySandboxSettingsFactory::GetForProfile(profile())->AddObserver(
      &mock_privacy_sandbox_observer);
  EXPECT_CALL(mock_privacy_sandbox_observer,
              OnFlocDataAccessibleSinceUpdated(/*reset_compute_timer=*/true));

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, false);
  testing::Mock::VerifyAndClearExpectations(&mock_privacy_sandbox_observer);

  EXPECT_CALL(mock_privacy_sandbox_observer,
              OnFlocDataAccessibleSinceUpdated(/*reset_compute_timer=*/true));
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  testing::Mock::VerifyAndClearExpectations(&mock_privacy_sandbox_observer);

  // OnFlocDataAccessibleSinceUpdated() will be called twice because the attempt
  // to enable the pref will be immediately followed by setting it to false.
  EXPECT_CALL(mock_privacy_sandbox_observer,
              OnFlocDataAccessibleSinceUpdated(/*reset_compute_timer=*/true))
      .Times(2);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  testing::Mock::VerifyAndClearExpectations(&mock_privacy_sandbox_observer);

  // OnFlocDataAccessibleSinceUpdated() will be called twice because the attempt
  // to enable the pref will be immediately followed by setting it to false.
  EXPECT_CALL(mock_privacy_sandbox_observer,
              OnFlocDataAccessibleSinceUpdated(/*reset_compute_timer=*/true))
      .Times(2);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  testing::Mock::VerifyAndClearExpectations(&mock_privacy_sandbox_observer);
}
