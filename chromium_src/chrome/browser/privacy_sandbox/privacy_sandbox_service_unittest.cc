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
#include "chrome/browser/federated_learning/floc_id_provider.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_service.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/ui/webui/federated_learning/floc_internals.mojom.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/federated_learning/features/features.h"
#include "components/federated_learning/floc_id.h"
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
#include "third_party/blink/public/mojom/federated_learning/floc.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace {

class MockFlocIdProvider : public federated_learning::FlocIdProvider {
 public:
  blink::mojom::InterestCohortPtr GetInterestCohortForJsApi(
      const GURL& url,
      const absl::optional<url::Origin>& top_frame_origin) const override {
    return blink::mojom::InterestCohort::New();
  }
  MOCK_METHOD(federated_learning::mojom::WebUIFlocStatusPtr,
              GetFlocStatusForWebUi,
              (),
              (const, override));
  MOCK_METHOD(void, MaybeRecordFlocToUkm, (ukm::SourceId), (override));
  MOCK_METHOD(base::Time, GetApproximateNextComputeTime, (), (const, override));
};

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
        identity_test_env()->identity_manager(), mock_floc_id_provider(),
        test_interest_group_manager(),
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
  MockFlocIdProvider* mock_floc_id_provider() {
    return &mock_floc_id_provider_;
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
  MockFlocIdProvider mock_floc_id_provider_;
  TestInterestGroupManager test_interest_group_manager_;

  std::unique_ptr<PrivacySandboxService> privacy_sandbox_service_;
};

TEST_F(PrivacySandboxServiceTest, GetFlocIdForDisplay) {
  // Check that the cohort identifier is correctly converted to a string when
  // available.
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);

  // In Brave, we actually don't enable anything here.
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxEnabled());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  federated_learning::FlocId floc_id = federated_learning::FlocId::CreateValid(
      123456, base::Time(), base::Time::Now(),
      /*sorting_lsh_version=*/0);
  floc_id.SaveToPrefs(profile()->GetTestingPrefService());

  // No valid ID is obtained since FLoC is actually disabled.
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_service()->GetFlocIdForDisplay());

  // If the FLoC preference, the Sandbox Preference, or the feature is disabled,
  // or the FLoC ID is invalid, the invalid string should be returned.
  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {}, {blink::features::kInterestCohortAPIOriginTrial});
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_service()->GetFlocIdForDisplay());

  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, false);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_service()->GetFlocIdForDisplay());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_service()->GetFlocIdForDisplay());

  floc_id.UpdateStatusAndSaveToPrefs(
      profile()->GetTestingPrefService(),
      federated_learning::FlocId::Status::kInvalidReset);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_service()->GetFlocIdForDisplay());
}

TEST_F(PrivacySandboxServiceTest, GetFlocIdNextUpdateForDisplay) {
  // Check that date FLoC will be next updated is returned when available.
  MockFlocIdProvider mock_floc_id_provider;
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);

  // In Brave, we actually don't enable anything here.
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxEnabled());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  std::map<base::TimeDelta, std::u16string> offsets_to_expected_string = {
      {base::Hours(23),
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)},
      {base::Hours(25),
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)},
      {base::Days(2),
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)},
      {base::Hours(60),
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)},
      {base::Hours(167),  // 1 hour less than 7 days.
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)}};

  for (const auto& offset_expected : offsets_to_expected_string) {
    EXPECT_EQ(offset_expected.second,
              privacy_sandbox_service()->GetFlocIdNextUpdateForDisplay(
                  base::Time::Now()));
    testing::Mock::VerifyAndClearExpectations(&mock_floc_id_provider);
  }

  // Disabling the FLoC feature should also invalidate the next compute time.
  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {}, {blink::features::kInterestCohortAPIOriginTrial});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  testing::Mock::VerifyAndClearExpectations(&mock_floc_id_provider);
}

TEST_F(PrivacySandboxServiceTest, GetFlocStatusForDisplay) {
  // Check the status of the user's FLoC is correctly returned. This depends
  // on whether the FLoC origin trial feature is enabled, and whether the user
  // has FLoC enabled.
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);

  // In Brave, we actually don't enable anything here.
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxEnabled());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  // Will report not active since nothing is actually enabled.
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE),
      privacy_sandbox_service()->GetFlocStatusForDisplay());

  // The Privacy Sandbox APIs pref & FLoC pref should disable the trial when
  // either is disabled.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, false);
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE),
      privacy_sandbox_service()->GetFlocStatusForDisplay());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE),
      privacy_sandbox_service()->GetFlocStatusForDisplay());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {}, {blink::features::kInterestCohortAPIOriginTrial});

  // Will report not active again since nothing is actually enabled.
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE),
      privacy_sandbox_service()->GetFlocStatusForDisplay());
}

TEST_F(PrivacySandboxServiceTest, IsFlocIdResettable) {
  // Check that if FLoC is functional the FLoC ID is resettable, regardless of
  // whether the FLoC ID is currently valid.
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  federated_learning::FlocId floc_id = federated_learning::FlocId::CreateValid(
      123456, base::Time(), base::Time::Now(),
      /*sorting_lsh_version=*/0);
  floc_id.SaveToPrefs(profile()->GetTestingPrefService());
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);

  // In Brave, we actually don't enable anything here.
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxEnabled());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());
  EXPECT_FALSE(privacy_sandbox_service()->IsFlocIdResettable());

  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {}, {blink::features::kInterestCohortAPIOriginTrial});
  EXPECT_FALSE(privacy_sandbox_service()->IsFlocIdResettable());

  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_FALSE(privacy_sandbox_service()->IsFlocIdResettable());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_FALSE(privacy_sandbox_service()->IsFlocIdResettable());

  floc_id.UpdateStatusAndSaveToPrefs(
      profile()->GetTestingPrefService(),
      federated_learning::FlocId::Status::kInvalidReset);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);

  // In Brave, trying to re-enable FLoC won't make a difference.
  EXPECT_FALSE(privacy_sandbox_service()->IsFlocIdResettable());
}

TEST_F(PrivacySandboxServiceTest, IsFlocPrefEnabled) {
  // IsFlocPrefEnabled should directly reflect the state of the FLoC pref, which
  // will always be false regardless of our attempts to set it to true.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  EXPECT_FALSE(privacy_sandbox_service()->IsFlocPrefEnabled());

  // The Privacy Sandbox APIs pref should not impact the return value.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, false);
  EXPECT_FALSE(privacy_sandbox_service()->IsFlocPrefEnabled());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_FALSE(privacy_sandbox_service()->IsFlocPrefEnabled());
}

TEST_F(PrivacySandboxServiceTest, SetFlocPrefEnabled) {
  // The FLoc pref should NEVER be updated by this function, regardless of
  // other Sandbox State or any calls to SetFlocPrefEnabled().
  base::UserActionTester user_action_tester;
  ASSERT_EQ(0, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocEnabled"));
  ASSERT_EQ(0, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocDisabled"));

  privacy_sandbox_service()->SetFlocPrefEnabled(false);
  EXPECT_FALSE(profile()->GetTestingPrefService()->GetBoolean(
      prefs::kPrivacySandboxFlocEnabled));
  ASSERT_EQ(0, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocEnabled"));
  ASSERT_EQ(1, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocDisabled"));

  // Disabling the sandbox shouldn't make a difference on the FLoC preference,
  // which should remain disabled regardless of calls to SetFlocPrefEnabled().
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, false);
  privacy_sandbox_service()->SetFlocPrefEnabled(true);
  EXPECT_FALSE(profile()->GetTestingPrefService()->GetBoolean(
      prefs::kPrivacySandboxFlocEnabled));
  ASSERT_EQ(1, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocEnabled"));
  ASSERT_EQ(1, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocDisabled"));
}

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
