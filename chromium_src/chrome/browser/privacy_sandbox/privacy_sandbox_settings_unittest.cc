// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings.h"

#include "base/json/values_util.h"
#include "base/test/gtest_util.h"
#include "base/test/icu_test_util.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/metrics/user_action_tester.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/federated_learning/floc_id_provider.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/ui/webui/federated_learning/floc_internals.mojom.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/content_settings/core/test/content_settings_mock_provider.h"
#include "components/content_settings/core/test/content_settings_test_utils.h"
#include "components/federated_learning/features/features.h"
#include "components/federated_learning/floc_id.h"
#include "components/policy/core/common/mock_policy_service.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/signin/public/identity_manager/identity_test_environment.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync/base/user_selectable_type.h"
#include "components/sync/driver/test_sync_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
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

class MockPrivacySandboxObserver : public PrivacySandboxSettings::Observer {
 public:
  MOCK_METHOD(void, OnFlocDataAccessibleSinceUpdated, (bool), (override));
};

// Define an additional content setting value to simulate an unmanaged default
// content setting.
const ContentSetting kNoSetting = static_cast<ContentSetting>(-1);

struct CookieContentSettingException {
  std::string primary_pattern;
  std::string secondary_pattern;
  ContentSetting content_setting;
};

}  // namespace

class PrivacySandboxSettingsTest : public testing::Test {
 public:
  PrivacySandboxSettingsTest()
      : browser_task_environment_(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    InitializePrefsBeforeStart();

    privacy_sandbox_settings_ = std::make_unique<PrivacySandboxSettings>(
        HostContentSettingsMapFactory::GetForProfile(profile()),
        CookieSettingsFactory::GetForProfile(profile()).get(),
        profile()->GetPrefs(), policy_service(), sync_service(),
        identity_test_env()->identity_manager());
  }

  virtual void InitializePrefsBeforeStart() {}

  // Sets up preferences and content settings based on provided parameters.
  void SetupTestState(
      bool privacy_sandbox_enabled,
      bool block_third_party_cookies,
      ContentSetting default_cookie_setting,
      std::vector<CookieContentSettingException> user_cookie_exceptions,
      ContentSetting managed_cookie_setting,
      std::vector<CookieContentSettingException> managed_cookie_exceptions) {
    // Setup block-third-party-cookies settings.
    profile()->GetTestingPrefService()->SetUserPref(
        prefs::kCookieControlsMode,
        std::make_unique<base::Value>(static_cast<int>(
            block_third_party_cookies
                ? content_settings::CookieControlsMode::kBlockThirdParty
                : content_settings::CookieControlsMode::kOff)));

    // Setup cookie content settings.
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
    auto user_provider = std::make_unique<content_settings::MockProvider>();
    auto managed_provider = std::make_unique<content_settings::MockProvider>();

    if (default_cookie_setting != kNoSetting) {
      user_provider->SetWebsiteSetting(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::Wildcard(), ContentSettingsType::COOKIES,
          std::make_unique<base::Value>(default_cookie_setting));
    }

    for (const auto& exception : user_cookie_exceptions) {
      user_provider->SetWebsiteSetting(
          ContentSettingsPattern::FromString(exception.primary_pattern),
          ContentSettingsPattern::FromString(exception.secondary_pattern),
          ContentSettingsType::COOKIES,
          std::make_unique<base::Value>(exception.content_setting));
    }

    if (managed_cookie_setting != kNoSetting) {
      managed_provider->SetWebsiteSetting(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::Wildcard(), ContentSettingsType::COOKIES,
          std::make_unique<base::Value>(managed_cookie_setting));
    }

    for (const auto& exception : managed_cookie_exceptions) {
      managed_provider->SetWebsiteSetting(
          ContentSettingsPattern::FromString(exception.primary_pattern),
          ContentSettingsPattern::FromString(exception.secondary_pattern),
          ContentSettingsType::COOKIES,
          std::make_unique<base::Value>(exception.content_setting));
    }

    content_settings::TestUtils::OverrideProvider(
        map, std::move(user_provider),
        HostContentSettingsMap::DEFAULT_PROVIDER);
    content_settings::TestUtils::OverrideProvider(
        map, std::move(managed_provider),
        HostContentSettingsMap::POLICY_PROVIDER);

    privacy_sandbox_settings()->SetPrivacySandboxEnabled(
        privacy_sandbox_enabled);
  }

  TestingProfile* profile() { return &profile_; }
  PrivacySandboxSettings* privacy_sandbox_settings() {
    return privacy_sandbox_settings_.get();
  }
  base::test::ScopedFeatureList* feature_list() { return &feature_list_; }
  syncer::TestSyncService* sync_service() { return &sync_service_; }
  policy::MockPolicyService* policy_service() { return &mock_policy_service_; }
  signin::IdentityTestEnvironment* identity_test_env() {
    return &identity_test_env_;
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  signin::IdentityTestEnvironment identity_test_env_;
  testing::NiceMock<policy::MockPolicyService> mock_policy_service_;

  TestingProfile profile_;
  base::test::ScopedFeatureList feature_list_;
  syncer::TestSyncService sync_service_;

  std::unique_ptr<PrivacySandboxSettings> privacy_sandbox_settings_;
};

TEST_F(PrivacySandboxSettingsTest, PreferenceOverridesDefaultContentSetting) {
  // Even if we try to enable the Privacy Sandbox, it should remain disabled, so
  // the sandbox preference should never override the default cookie content.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  // All should be DISABLED: FLoC, Conversion measurement & reporting, fledge...
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"),
      url::Origin::Create(GURL("https://test.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsConversionMeasurementAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->ShouldSendConversionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com"),
                 GURL("https://another-embedded.com")}));

  // An allow exception should not override the preference value.
  SetupTestState(
      /*privacy_sandbox_enabled=*/false,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW},
       {"https://another-embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW},
       {"https://embedded.com", "https://another-test.com",
        ContentSetting::CONTENT_SETTING_ALLOW}},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"),
      url::Origin::Create(GURL("https://test.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsConversionMeasurementAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->ShouldSendConversionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com"),
                 GURL("https://another-embedded.com")}));
}

TEST_F(PrivacySandboxSettingsTest, CookieBlockExceptionsNeverApply) {
  // Even if we try to enable the Privacy Sandbox, it should remain disabled, so
  // targeted cookie block exceptions should never apply.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK},
       {"https://another-embedded.com", "*",
        ContentSetting::CONTENT_SETTING_BLOCK}},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"),
      url::Origin::Create(GURL("https://test.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsConversionMeasurementAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->ShouldSendConversionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com"),
                 GURL("https://another-embedded.com")}));

  // User created exceptions should not apply if a managed default coookie
  // setting exists. What the managed default setting actually is should *not*
  // affect whether APIs are enabled. The cookie managed state is reflected in
  // the privacy sandbox preferences directly.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK},
       {"https://another-embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK},
       {"https://embedded.com", "https://another-test.com",
        ContentSetting::CONTENT_SETTING_BLOCK}},
      /*managed_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*managed_cookie_exceptions=*/{});

  // All should be DISABLED: FLoC, Conversion measurement & reporting, fledge...
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"),
      url::Origin::Create(GURL("https://test.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsConversionMeasurementAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->ShouldSendConversionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com"),
                 GURL("https://another-embedded.com")}));

  // Managed content setting exceptions.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW},
       {"https://another-embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW},
       {"https://embedded.com", "https://another-test.com",
        ContentSetting::CONTENT_SETTING_ALLOW}},
      /*managed_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK}});

  // It doesn't matter, everything should be DISABLED again.
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"),
      url::Origin::Create(GURL("https://test.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://unrelated.com"),
      url::Origin::Create(GURL("https://unrelated.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsConversionMeasurementAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->ShouldSendConversionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsConversionMeasurementAllowed(
      url::Origin::Create(GURL("https://unrelated-a.com")),
      url::Origin::Create(GURL("https://unrelated-b.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->ShouldSendConversionReport(
      url::Origin::Create(GURL("https://unrelated-c.com")),
      url::Origin::Create(GURL("https://unrelated-d.com")),
      url::Origin::Create(GURL("https://unrelated-e.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com"),
                 GURL("https://another-embedded.com")}));

  // A less specific block exception.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW},
       {"https://embedded.com", "https://another-test.com",
        ContentSetting::CONTENT_SETTING_ALLOW},
       {"https://[*.]embedded.com", "https://[*.]test.com",
        ContentSetting::CONTENT_SETTING_BLOCK},
       {"https://[*.]embedded.com", "https://[*.]another-test.com",
        ContentSetting::CONTENT_SETTING_BLOCK}},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  // It doesn't matter, everything should be DISABLED again.
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"),
      url::Origin::Create(GURL("https://test.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));

  // Exceptions which specify a top frame origin.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK}},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK}});

  // It doesn't matter, everything should be DISABLED again.
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"), absl::nullopt));

  EXPECT_FALSE(privacy_sandbox_settings()->IsConversionMeasurementAllowed(
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->ShouldSendConversionReport(
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://yet-another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://another-test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com"),
                 GURL("https://another-embedded.com")}));

  // Exceptions which specify a wildcard top frame origin.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "*", ContentSetting::CONTENT_SETTING_BLOCK}},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  // It doesn't matter, everything should be DISABLED again.
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"), absl::nullopt));
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowedForContext(
      GURL("https://embedded.com"),
      url::Origin::Create(GURL("https://test.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsConversionMeasurementAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->ShouldSendConversionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com"),
                 GURL("https://another-embedded.com")}));
}

TEST_F(PrivacySandboxSettingsTest, IsFledgeAllowed) {
  // FLEDGE should be disabled if 3P cookies are blocked.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/true,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com")}));

  // FLEDGE should be disabled if all cookies are blocked.
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com")}));

  // FLEDGE should be disabled if the privacy sandbox is disabled, regardless
  // of other cookie settings.
  SetupTestState(
      /*privacy_sandbox_enabled=*/false,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW}},
      /*managed_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*managed_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW}});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com")}));

  // The managed cookie content setting should not override a disabled privacy
  // sandbox setting.
  SetupTestState(
      /*privacy_sandbox_enabled=*/false,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_EQ(std::vector<GURL>{},
            privacy_sandbox_settings()->FilterFledgeAllowedParties(
                url::Origin::Create(GURL("https://test.com")),
                {GURL("https://embedded.com")}));
}

TEST_F(PrivacySandboxSettingsTest, IsPrivacySandboxAllowed) {
  SetupTestState(
      /*privacy_sandbox_enabled=*/false,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxAllowed());

  SetupTestState(
      /*privacy_sandbox_enabled=*/false,
      /*block_third_party_cookies=*/true,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxAllowed());

  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  // Trying to enable the privacy sandbox doesn't make a difference in Brave.
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxAllowed());

  // Check that even bypassing PrivacySandboxSettings::SetPrivacySandboxEnabled,
  // and manually updating the preference, we still don't get this enabled.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxAllowed());
}

TEST_F(PrivacySandboxSettingsTest, IsFlocAllowed) {
  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/true,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});

  // FLoC should be disabled since the privacy sandbox APIs can't be enabled.
  privacy_sandbox_settings()->SetFlocPrefEnabled(true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  // Check that even bypassing PrivacySandboxSettings::SetFlocPrefEnabled,
  // and manually updating the preferences, we still don't get this enabled.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  SetupTestState(
      /*privacy_sandbox_enabled=*/false,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});
  privacy_sandbox_settings()->SetFlocPrefEnabled(true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  SetupTestState(
      /*privacy_sandbox_enabled=*/true,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});
  privacy_sandbox_settings()->SetFlocPrefEnabled(false);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  SetupTestState(
      /*privacy_sandbox_enabled=*/false,
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/kNoSetting,
      /*managed_cookie_exceptions=*/{});
  privacy_sandbox_settings()->SetFlocPrefEnabled(true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());
}

TEST_F(PrivacySandboxSettingsTest, GetFlocIdForDisplay) {
  // Check that the cohort identifier is correctly converted to a string when
  // available.
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);

  // In Brave, we actually don't enable anything here.
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxAllowed());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  federated_learning::FlocId floc_id = federated_learning::FlocId::CreateValid(
      123456, base::Time(), base::Time::Now(),
      /*sorting_lsh_version=*/0);
  floc_id.SaveToPrefs(profile()->GetTestingPrefService());

  // No valid ID is obtained since FLoC is actually disabled.
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_settings()->GetFlocIdForDisplay());

  // If the FLoC preference, the Sandbox Preference, or the feature is disabled,
  // or the FLoC ID is invalid, the invalid string should be returned.
  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {}, {blink::features::kInterestCohortAPIOriginTrial});
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_settings()->GetFlocIdForDisplay());

  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, false);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_settings()->GetFlocIdForDisplay());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_settings()->GetFlocIdForDisplay());

  floc_id.UpdateStatusAndSaveToPrefs(
      profile()->GetTestingPrefService(),
      federated_learning::FlocId::Status::kInvalidReset);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_INVALID),
            privacy_sandbox_settings()->GetFlocIdForDisplay());
}

TEST_F(PrivacySandboxSettingsTest, GetFlocIdNextUpdateForDisplay) {
  // Check that date FLoC will be next updated is returned when available.
  MockFlocIdProvider mock_floc_id_provider;
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);

  // In Brave, we actually don't enable anything here.
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxAllowed());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  std::map<base::TimeDelta, std::u16string> offsets_to_expected_string = {
      {base::TimeDelta::FromHours(23),
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)},
      {base::TimeDelta::FromHours(25),
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)},
      {base::TimeDelta::FromDays(2),
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)},
      {base::TimeDelta::FromHours(60),
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)},
      {base::TimeDelta::FromHours(167),  // 1 hour less than 7 days.
       l10n_util::GetStringUTF16(
           IDS_PRIVACY_SANDBOX_FLOC_TIME_TO_NEXT_COMPUTE_INVALID)}};

  for (const auto& offset_expected : offsets_to_expected_string) {
    EXPECT_EQ(
        offset_expected.second,
        privacy_sandbox_settings()->GetFlocIdNextUpdateForDisplay(
            &mock_floc_id_provider, profile()->GetPrefs(), base::Time::Now()));
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

TEST_F(PrivacySandboxSettingsTest, GetFlocStatusForDisplay) {
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
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxAllowed());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());

  // Will report not active since nothing is actually enabled.
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE),
      privacy_sandbox_settings()->GetFlocStatusForDisplay());

  // The Privacy Sandbox APIs pref & FLoC pref should disable the trial when
  // either is disabled.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, false);
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE),
      privacy_sandbox_settings()->GetFlocStatusForDisplay());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE),
      privacy_sandbox_settings()->GetFlocStatusForDisplay());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {}, {blink::features::kInterestCohortAPIOriginTrial});

  // Will report not active again since nothing is actually enabled.
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PRIVACY_SANDBOX_FLOC_STATUS_NOT_ACTIVE),
      privacy_sandbox_settings()->GetFlocStatusForDisplay());
}

TEST_F(PrivacySandboxSettingsTest, IsFlocIdResettable) {
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
  EXPECT_FALSE(privacy_sandbox_settings()->IsPrivacySandboxAllowed());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocAllowed());
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocIdResettable());

  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {}, {blink::features::kInterestCohortAPIOriginTrial});
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocIdResettable());

  feature_list()->Reset();
  feature_list()->InitWithFeatures(
      {blink::features::kInterestCohortAPIOriginTrial}, {});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocIdResettable());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocIdResettable());

  floc_id.UpdateStatusAndSaveToPrefs(
      profile()->GetTestingPrefService(),
      federated_learning::FlocId::Status::kInvalidReset);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);

  // In Brave, trying to re-enable FLoC won't make a difference.
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocIdResettable());
}

TEST_F(PrivacySandboxSettingsTest, IsFlocPrefEnabled) {
  // IsFlocPrefEnabled should directly reflect the state of the FLoC pref, which
  // will always be false regardless of our attempts to set it to true.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocPrefEnabled());

  // The Privacy Sandbox APIs pref should not impact the return value.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, false);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocPrefEnabled());

  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxFlocEnabled, false);
  EXPECT_FALSE(privacy_sandbox_settings()->IsFlocPrefEnabled());
}

TEST_F(PrivacySandboxSettingsTest, SetFlocPrefEnabled) {
  // The FLoc pref should NEVER be updated by this function, regardless of
  // other Sandbox State or any calls to SetFlocPrefEnabled().
  base::UserActionTester user_action_tester;
  ASSERT_EQ(0, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocEnabled"));
  ASSERT_EQ(0, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocDisabled"));

  privacy_sandbox_settings()->SetFlocPrefEnabled(false);
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
  privacy_sandbox_settings()->SetFlocPrefEnabled(true);
  EXPECT_FALSE(profile()->GetTestingPrefService()->GetBoolean(
      prefs::kPrivacySandboxFlocEnabled));
  ASSERT_EQ(1, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocEnabled"));
  ASSERT_EQ(1, user_action_tester.GetActionCount(
                   "Settings.PrivacySandbox.FlocDisabled"));
}

TEST_F(PrivacySandboxSettingsTest, OnPrivacySandboxPrefChanged) {
  // When either the main Privacy Sandbox pref, or the FLoC pref, are changed
  // the FLoC ID should be reset.
  MockPrivacySandboxObserver mock_privacy_sandbox_observer;
  privacy_sandbox_settings()->AddObserver(&mock_privacy_sandbox_observer);
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

class PrivacySandboxSettingsTestCookiesClearOnExitTurnedOff
    : public PrivacySandboxSettingsTest {
 public:
  void InitializePrefsBeforeStart() override {
    profile()->GetTestingPrefService()->SetUserPref(
        prefs::kPrivacySandboxFlocDataAccessibleSince,
        std::make_unique<base::Value>(
            ::base::TimeToValue(base::Time::FromTimeT(12345))));
  }
};

TEST_F(PrivacySandboxSettingsTestCookiesClearOnExitTurnedOff,
       UseLastFlocDataAccessibleSince) {
  EXPECT_EQ(base::Time::FromTimeT(12345),
            privacy_sandbox_settings()->FlocDataAccessibleSince());
}

class PrivacySandboxSettingsTestCookiesClearOnExitTurnedOn
    : public PrivacySandboxSettingsTest {
 public:
  void InitializePrefsBeforeStart() override {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
    map->SetDefaultContentSetting(ContentSettingsType::COOKIES,
                                  ContentSetting::CONTENT_SETTING_SESSION_ONLY);

    profile()->GetTestingPrefService()->SetUserPref(
        prefs::kPrivacySandboxFlocDataAccessibleSince,
        std::make_unique<base::Value>(
            ::base::TimeToValue(base::Time::FromTimeT(12345))));
  }
};

TEST_F(PrivacySandboxSettingsTestCookiesClearOnExitTurnedOn,
       UpdateFlocDataAccessibleSince) {
  EXPECT_EQ(base::Time::Now(),
            privacy_sandbox_settings()->FlocDataAccessibleSince());
}
