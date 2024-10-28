/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/privacy_sandbox/privacy_sandbox_settings.h"

#include "base/json/values_util.h"
#include "base/test/gtest_util.h"
#include "brave/components/privacy_sandbox/brave_privacy_sandbox_settings.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/content_settings/core/test/content_settings_mock_provider.h"
#include "components/content_settings/core/test/content_settings_test_utils.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"
#include "components/privacy_sandbox/privacy_sandbox_test_util.h"
#include "components/privacy_sandbox/tracking_protection_settings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace {

// Sets up preferences and content settings based on provided parameters.
void SetupTestState(
    sync_preferences::TestingPrefServiceSyncable* testing_pref_service,
    HostContentSettingsMap* map,
    bool block_third_party_cookies,
    ContentSetting default_cookie_setting,
    const std::vector<privacy_sandbox_test_util::CookieContentSettingException>&
        user_cookie_exceptions,
    ContentSetting managed_cookie_setting,
    const std::vector<privacy_sandbox_test_util::CookieContentSettingException>&
        managed_cookie_exceptions) {
  // Setup block-third-party-cookies settings.
  testing_pref_service->SetUserPref(
      prefs::kCookieControlsMode,
      base::Value(static_cast<int>(
          block_third_party_cookies
              ? content_settings::CookieControlsMode::kBlockThirdParty
              : content_settings::CookieControlsMode::kOff)));

  // Setup cookie content settings.
  auto user_provider = std::make_unique<content_settings::MockProvider>();
  auto managed_provider = std::make_unique<content_settings::MockProvider>();

  if (default_cookie_setting != privacy_sandbox_test_util::kNoSetting) {
    user_provider->SetWebsiteSetting(
        ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
        ContentSettingsType::COOKIES, base::Value(default_cookie_setting),
        /*constraints=*/{},
        content_settings::PartitionKey::GetDefaultForTesting());
  }

  for (const auto& exception : user_cookie_exceptions) {
    user_provider->SetWebsiteSetting(
        ContentSettingsPattern::FromString(exception.primary_pattern),
        ContentSettingsPattern::FromString(exception.secondary_pattern),
        ContentSettingsType::COOKIES, base::Value(exception.content_setting),
        /*constraints=*/{},
        content_settings::PartitionKey::GetDefaultForTesting());
  }

  if (managed_cookie_setting != privacy_sandbox_test_util::kNoSetting) {
    managed_provider->SetWebsiteSetting(
        ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
        ContentSettingsType::COOKIES, base::Value(managed_cookie_setting),
        /*constraints=*/{},
        content_settings::PartitionKey::GetDefaultForTesting());
  }

  for (const auto& exception : managed_cookie_exceptions) {
    managed_provider->SetWebsiteSetting(
        ContentSettingsPattern::FromString(exception.primary_pattern),
        ContentSettingsPattern::FromString(exception.secondary_pattern),
        ContentSettingsType::COOKIES, base::Value(exception.content_setting),
        /*constraints=*/{},
        content_settings::PartitionKey::GetDefaultForTesting());
  }

  content_settings::TestUtils::OverrideProvider(
      map, std::move(user_provider),
      content_settings::ProviderType::kDefaultProvider);
  content_settings::TestUtils::OverrideProvider(
      map, std::move(managed_provider),
      content_settings::ProviderType::kPolicyProvider);
}

}  // namespace

namespace privacy_sandbox {

class MockPrivacySandboxDelegate : public PrivacySandboxSettings::Delegate {
 public:
  void SetupDefaultResponse() {
    ON_CALL(*this, IsPrivacySandboxRestricted).WillByDefault([]() {
      return false;
    });
  }
  MOCK_METHOD(bool, IsPrivacySandboxRestricted, (), (const, override));
  MOCK_METHOD(bool, IsIncognitoProfile, (), (const, override));
  MOCK_METHOD(bool, HasAppropriateTopicsConsent, (), (const, override));
  MOCK_METHOD(bool, IsSubjectToM1NoticeRestricted, (), (const, override));
  MOCK_METHOD(bool, IsRestrictedNoticeEnabled, (), (const, override));
  MOCK_METHOD(bool,
              IsPrivacySandboxCurrentlyUnrestricted,
              (),
              (const, override));
  MOCK_METHOD(bool,
              IsCookieDeprecationExperimentEligible,
              (),
              (const, override));
  MOCK_METHOD(TpcdExperimentEligibility,
              GetCookieDeprecationExperimentCurrentEligibility,
              (),
              (const, override));
  MOCK_METHOD(bool, IsCookieDeprecationLabelAllowed, (), (const, override));
  MOCK_METHOD(bool,
              AreThirdPartyCookiesBlockedByCookieDeprecationExperiment,
              (),
              (const, override));
};

class PrivacySandboxSettingsTest : public testing::Test {
 public:
  PrivacySandboxSettingsTest()
      : browser_task_environment_(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    content_settings::CookieSettings::RegisterProfilePrefs(prefs()->registry());
    HostContentSettingsMap::RegisterProfilePrefs(prefs()->registry());
    privacy_sandbox::RegisterProfilePrefs(prefs()->registry());

    host_content_settings_map_ = new HostContentSettingsMap(
        &prefs_, false /* is_off_the_record */, false /* store_last_modified */,
        false /* restore_session */, false /* should_record_metrics */);
    cookie_settings_ = new content_settings::CookieSettings(
        host_content_settings_map_.get(), &prefs_,
        /*tracking_protection_settings=*/nullptr, false,
        content_settings::CookieSettings::NoFedCmSharingPermissionsCallback(),
        /*tpcd_metadata_manager=*/nullptr, "chrome-extension");
    tracking_protection_settings_ =
        std::make_unique<privacy_sandbox::TrackingProtectionSettings>(
            &prefs_, host_content_settings_map_.get(),
            /*is_incognito=*/false);
  }
  ~PrivacySandboxSettingsTest() override {
    host_content_settings_map()->ShutdownOnUIThread();
  }

  void SetUp() override {
    auto mock_delegate = std::make_unique<MockPrivacySandboxDelegate>();
    mock_delegate_ = mock_delegate.get();

    InitializePrefsBeforeStart();

    privacy_sandbox_settings_ = std::make_unique<BravePrivacySandboxSettings>(
        std::move(mock_delegate), host_content_settings_map(),
        cookie_settings(), tracking_protection_settings_.get(), prefs());
  }

  virtual void InitializePrefsBeforeStart() {}

  MockPrivacySandboxDelegate* mock_delegate() { return mock_delegate_; }
  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }
  HostContentSettingsMap* host_content_settings_map() {
    return host_content_settings_map_.get();
  }
  content_settings::CookieSettings* cookie_settings() {
    return cookie_settings_.get();
  }

  TestingProfile* profile() { return &profile_; }
  PrivacySandboxSettings* privacy_sandbox_settings() {
    return privacy_sandbox_settings_.get();
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  TestingProfile profile_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  std::unique_ptr<privacy_sandbox::TrackingProtectionSettings>
      tracking_protection_settings_;

  std::unique_ptr<PrivacySandboxSettings> privacy_sandbox_settings_;
  raw_ptr<MockPrivacySandboxDelegate, DanglingUntriaged> mock_delegate_;
};

TEST_F(PrivacySandboxSettingsTest, PreferenceOverridesDefaultContentSetting) {
  // Even if we try to enable the Privacy Sandbox, it should remain disabled, so
  // the sandbox preference should never override the default cookie content.
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});

  // All should be DISABLED: FLoC, Conversion measurement & reporting, fledge...
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_FALSE(privacy_sandbox_settings()->IsAttributionReportingAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->MaySendAttributionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // An allow exception should not override the preference value.
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW},
       {"https://another-embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_ALLOW},
       {"https://embedded.com", "https://another-test.com",
        ContentSetting::CONTENT_SETTING_ALLOW}},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));

  EXPECT_FALSE(privacy_sandbox_settings()->IsAttributionReportingAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->MaySendAttributionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));
}

TEST_F(PrivacySandboxSettingsTest, CookieBlockExceptionsNeverApply) {
  // Even if we try to enable the Privacy Sandbox, it should remain disabled, so
  // targeted cookie block exceptions should never apply.
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK},
       {"https://another-embedded.com", "*",
        ContentSetting::CONTENT_SETTING_BLOCK}},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));

  EXPECT_FALSE(privacy_sandbox_settings()->IsAttributionReportingAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->MaySendAttributionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // User created exceptions should not apply if a managed default coookie
  // setting exists. What the managed default setting actually is should *not*
  // affect whether APIs are enabled. The cookie managed state is reflected in
  // the privacy sandbox preferences directly.
  SetupTestState(
      prefs(), host_content_settings_map(),
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
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_FALSE(privacy_sandbox_settings()->IsAttributionReportingAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->MaySendAttributionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // Managed content setting exceptions.
  SetupTestState(
      prefs(), host_content_settings_map(),
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
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin::Create(GURL("https://unrelated.com")),
      GURL("https://unrelated.com")));

  EXPECT_FALSE(privacy_sandbox_settings()->IsAttributionReportingAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->MaySendAttributionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->IsAttributionReportingAllowed(
      url::Origin::Create(GURL("https://unrelated-a.com")),
      url::Origin::Create(GURL("https://unrelated-b.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->MaySendAttributionReport(
      url::Origin::Create(GURL("https://unrelated-c.com")),
      url::Origin::Create(GURL("https://unrelated-d.com")),
      url::Origin::Create(GURL("https://unrelated-e.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // A less specific block exception.
  SetupTestState(
      prefs(), host_content_settings_map(),
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
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});

  // It doesn't matter, everything should be DISABLED again.
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // Exceptions which specify a top frame origin.
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK}},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/
      {{"https://embedded.com", "https://test.com",
        ContentSetting::CONTENT_SETTING_BLOCK}});

  // It doesn't matter, everything should be DISABLED again.
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin(), GURL("https://embedded.com")));

  EXPECT_FALSE(privacy_sandbox_settings()->IsAttributionReportingAllowed(
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->MaySendAttributionReport(
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://yet-another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // Exceptions which specify a wildcard top frame origin.
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/
      {{"https://embedded.com", "*", ContentSetting::CONTENT_SETTING_BLOCK}},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});

  // It doesn't matter, everything should be DISABLED again.
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin(), GURL("https://embedded.com")));
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowedForContext(
      url::Origin::Create(GURL("https://test.com")),
      GURL("https://embedded.com")));

  EXPECT_FALSE(privacy_sandbox_settings()->IsAttributionReportingAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));
  EXPECT_FALSE(privacy_sandbox_settings()->MaySendAttributionReport(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://another-test.com")),
      url::Origin::Create(GURL("https://embedded.com"))));

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));
}

TEST_F(PrivacySandboxSettingsTest, IsFledgeAllowed) {
  // FLEDGE should be disabled if 3P cookies are blocked.
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/true,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // FLEDGE should be disabled if all cookies are blocked.
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // FLEDGE should be disabled if the privacy sandbox is disabled, regardless
  // of other cookie settings.
  SetupTestState(
      prefs(), host_content_settings_map(),
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
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));

  // The managed cookie content setting should not override a disabled privacy
  // sandbox setting.
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsFledgeAllowed(
      url::Origin::Create(GURL("https://test.com")),
      url::Origin::Create(GURL("https://embedded.com")),
      content::InterestGroupApiOperation::kJoin));
}

TEST_F(PrivacySandboxSettingsTest, IsTopicsAllowed) {
  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/true,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});

  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowed());

  // Check that even manually updating the preferences, we still don't get this
  // enabled.
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabledV2, true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowed());

  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_BLOCK,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabledV2, true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowed());

  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabledV2, true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowed());

  SetupTestState(
      prefs(), host_content_settings_map(),
      /*block_third_party_cookies=*/false,
      /*default_cookie_setting=*/ContentSetting::CONTENT_SETTING_ALLOW,
      /*user_cookie_exceptions=*/{},
      /*managed_cookie_setting=*/privacy_sandbox_test_util::kNoSetting,
      /*managed_cookie_exceptions=*/{});
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabled, true);
  profile()->GetTestingPrefService()->SetBoolean(
      prefs::kPrivacySandboxApisEnabledV2, true);
  EXPECT_FALSE(privacy_sandbox_settings()->IsTopicsAllowed());
}

class PrivacySandboxSettingsTestCookiesClearOnExitTurnedOff
    : public PrivacySandboxSettingsTest {
 public:
  void InitializePrefsBeforeStart() override {
    prefs()->SetUserPref(prefs::kPrivacySandboxTopicsDataAccessibleSince,
                         base::TimeToValue(base::Time::FromTimeT(12345)));
  }
};

TEST_F(PrivacySandboxSettingsTestCookiesClearOnExitTurnedOff,
       UseLastTopicsDataAccessibleSince) {
  // The preference value is ignored
  EXPECT_EQ(base::Time::Max(),
            privacy_sandbox_settings()->TopicsDataAccessibleSince());
}

class PrivacySandboxSettingsTestCookiesClearOnExitTurnedOn
    : public PrivacySandboxSettingsTest {
 public:
  void InitializePrefsBeforeStart() override {
    cookie_settings()->SetDefaultCookieSetting(
        ContentSetting::CONTENT_SETTING_SESSION_ONLY);

    prefs()->SetUserPref(prefs::kPrivacySandboxTopicsDataAccessibleSince,
                         base::TimeToValue(base::Time::FromTimeT(12345)));
  }
};

TEST_F(PrivacySandboxSettingsTestCookiesClearOnExitTurnedOn,
       UpdateTopicsDataAccessibleSince) {
  // Clear cookies on exit doesn't affect TopicsDataAccessibleSince() return
  // value. The preference value is not updated and ignored.
  EXPECT_EQ(base::Time::FromTimeT(12345),
            prefs()->GetTime(prefs::kPrivacySandboxTopicsDataAccessibleSince));
  EXPECT_EQ(base::Time::Max(),
            privacy_sandbox_settings()->TopicsDataAccessibleSince());
}

}  // namespace privacy_sandbox
