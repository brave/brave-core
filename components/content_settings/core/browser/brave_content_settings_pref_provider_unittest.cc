/*  Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/optional.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/test/content_settings_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content_settings {

namespace {

using GURLSourcePair = std::pair<GURL, ContentSettingsType>;

ContentSettingsPattern SecondaryUrlToPattern(const GURL& gurl) {
  CHECK(gurl == GURL() || gurl == GURL("https://firstParty/*"));
  if (gurl == GURL())
    return ContentSettingsPattern::Wildcard();
  else
    return ContentSettingsPattern::FromString("https://firstParty/*");
}

class ShieldsSetting {
 public:
  ShieldsSetting(BravePrefProvider* provider,
                 const std::vector<GURLSourcePair> urls)
      : provider_(provider), urls_(urls) {}
  virtual ~ShieldsSetting() = default;

  virtual void SetPreMigrationSettings(const ContentSettingsPattern& pattern,
                                       ContentSetting setting) {
    for (const auto& url_source : urls_) {
      provider_->SetWebsiteSetting(
          pattern, SecondaryUrlToPattern(url_source.first),
          url_source.second, ContentSettingToValue(setting), {});
    }
  }

  void CheckSettingsAreDefault(const GURL& url) const {
    CheckSettings(url, CONTENT_SETTING_DEFAULT);
  }

  void CheckSettingsWouldBlock(const GURL& url) const {
    CheckSettings(url, CONTENT_SETTING_BLOCK);
  }

  void CheckSettingsWouldAllow(const GURL& url) const {
    CheckSettings(url, CONTENT_SETTING_ALLOW);
  }

 protected:
  virtual void CheckSettings(const GURL& url, ContentSetting setting) const {
    for (const auto& url_source : urls_) {
      EXPECT_EQ(setting,
                TestUtils::GetContentSetting(provider_, url, url_source.first,
                                             url_source.second, false));
    }
  }

  BravePrefProvider* provider_;
  const std::vector<GURLSourcePair> urls_;
};

class ShieldsCookieSetting : public ShieldsSetting {
 public:
  explicit ShieldsCookieSetting(BravePrefProvider* provider)
      : ShieldsSetting(provider,
                       {{GURL(), ContentSettingsType::BRAVE_COOKIES},
                        {GURL("https://firstParty/*"),
                         ContentSettingsType::BRAVE_COOKIES},
                        {GURL(), ContentSettingsType::BRAVE_REFERRERS}}) {}
};

class ShieldsFingerprintingSetting : public ShieldsSetting {
 public:
  explicit ShieldsFingerprintingSetting(BravePrefProvider* provider)
      : ShieldsSetting(provider,
                       {{GURL(), ContentSettingsType::BRAVE_FINGERPRINTING_V2},
                       {GURL("https://firstParty/*"),
                        ContentSettingsType::BRAVE_FINGERPRINTING_V2}}) {}
};

class ShieldsHTTPSESetting : public ShieldsSetting {
 public:
  explicit ShieldsHTTPSESetting(BravePrefProvider* provider)
      : ShieldsSetting(provider,
                       {{GURL(),
                         ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES}})
        {}
};

class ShieldsAdsSetting : public ShieldsSetting {
 public:
  explicit ShieldsAdsSetting(BravePrefProvider* provider)
      : ShieldsSetting(provider,
                       {{GURL(), ContentSettingsType::BRAVE_ADS},
                        {GURL(), ContentSettingsType::BRAVE_TRACKERS}}) {}
};

class ShieldsEnabledSetting : public ShieldsSetting {
 public:
  explicit ShieldsEnabledSetting(BravePrefProvider* provider)
      : ShieldsSetting(provider, {{GURL(), ContentSettingsType::BRAVE_SHIELDS}})
        {}
};

class ShieldsScriptSetting : public ShieldsSetting {
 public:
  explicit ShieldsScriptSetting(BravePrefProvider* provider)
      : ShieldsSetting(provider, {}) {}

  void SetPreMigrationSettings(const ContentSettingsPattern& pattern,
                               ContentSetting setting) override {
    provider_->SetWebsiteSetting(pattern, ContentSettingsPattern::Wildcard(),
                                 ContentSettingsType::JAVASCRIPT,
                                 ContentSettingToValue(setting), {});
  }

 private:
  void CheckSettings(const GURL& url, ContentSetting setting) const override {
    EXPECT_EQ(setting, TestUtils::GetContentSetting(
                           provider_, url, GURL(),
                           ContentSettingsType::JAVASCRIPT, false));
  }
};

}  // namespace

class BravePrefProviderTest : public testing::Test {
 public:
  BravePrefProviderTest() {
    // Ensure all content settings are initialized.
    ContentSettingsRegistry::GetInstance();
  }

  void SetUp() override {
    testing::Test::SetUp();
    testing_profile_ = TestingProfile::Builder().Build();
  }

  void TearDown() override { testing_profile_.reset(); }

  TestingProfile* testing_profile() { return testing_profile_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> testing_profile_;
};

TEST_F(BravePrefProviderTest, TestShieldsSettingsMigration) {
  BravePrefProvider provider(
      testing_profile()->GetPrefs(), false /* incognito */,
      true /* store_last_modified */, false /* restore_session */);

  ShieldsCookieSetting cookie_settings(&provider);
  ShieldsFingerprintingSetting fp_settings(&provider);
  ShieldsHTTPSESetting httpse_settings(&provider);
  ShieldsAdsSetting ads_settings(&provider);
  ShieldsEnabledSetting enabled_settings(&provider);
  ShieldsScriptSetting script_settings(&provider);

  GURL url("http://brave.com:8080/");
  GURL url2("http://allowed.brave.com:3030");
  // Check that the settings for the url are default values.
  cookie_settings.CheckSettingsAreDefault(url);
  cookie_settings.CheckSettingsAreDefault(url2);
  fp_settings.CheckSettingsAreDefault(url);
  httpse_settings.CheckSettingsAreDefault(url);
  ads_settings.CheckSettingsAreDefault(url);
  enabled_settings.CheckSettingsAreDefault(url);
  script_settings.CheckSettingsAreDefault(url);

  // Set pre-migrtion patterns different from defaults.
  // ------------------------------------------------------
  ContentSettingsPattern pattern = ContentSettingsPattern::FromURL(url);
  ContentSettingsPattern pattern2 = ContentSettingsPattern::FromURL(url2);
  // Cookies.
  cookie_settings.SetPreMigrationSettings(pattern, CONTENT_SETTING_BLOCK);
  cookie_settings.SetPreMigrationSettings(pattern2, CONTENT_SETTING_ALLOW);
  // Pattern that doesn't need to be migrated.
  cookie_settings.SetPreMigrationSettings(
      ContentSettingsPattern::FromString("*://help.brave.com/*"),
      CONTENT_SETTING_BLOCK);
  // Check that settings would block brave.com:8080, but not brave.com:5555.
  cookie_settings.CheckSettingsWouldBlock(url);
  cookie_settings.CheckSettingsWouldAllow(url2);
  cookie_settings.CheckSettingsAreDefault(GURL("http://brave.com:5555"));

  // Finterprinting.
  fp_settings.SetPreMigrationSettings(pattern, CONTENT_SETTING_ALLOW);
  // Check that settings would allow brave.com:8080, but not brave.com:5555.
  fp_settings.CheckSettingsWouldAllow(url);
  fp_settings.CheckSettingsAreDefault(GURL("http://brave.com:5555"));

  // HTTPSE.
  httpse_settings.SetPreMigrationSettings(pattern, CONTENT_SETTING_BLOCK);
  // Check that settings would block brave.com:8080, but not brave.com:5555.
  httpse_settings.CheckSettingsWouldBlock(url);
  httpse_settings.CheckSettingsAreDefault(GURL("http://brave.com:5555"));

  // Ads.
  ads_settings.SetPreMigrationSettings(pattern, CONTENT_SETTING_ALLOW);
  // Check that settings would allow brave.com:8080, but not brave.com:5555.
  ads_settings.CheckSettingsWouldAllow(url);
  ads_settings.CheckSettingsAreDefault(GURL("http://brave.com:5555"));

  // Enabled.
  enabled_settings.SetPreMigrationSettings(pattern, CONTENT_SETTING_BLOCK);
  // Check that settings would block brave.com:8080, but not brave.com:5555.
  httpse_settings.CheckSettingsWouldBlock(url);
  httpse_settings.CheckSettingsAreDefault(GURL("http://brave.com:5555"));

  // Scripts.
  script_settings.SetPreMigrationSettings(pattern, CONTENT_SETTING_BLOCK);
  // Check that settings would block brave.com:8080, but not brave.com:5555.
  script_settings.CheckSettingsWouldBlock(url);
  script_settings.CheckSettingsAreDefault(GURL("http://brave.com:5555"));

  // Migrate settings.
  // ------------------------------------------------------
  testing_profile()->GetPrefs()->SetInteger(kBraveShieldsSettingsVersion, 1);
  provider.MigrateShieldsSettings(/*incognito*/ false);

  // Check post-migration settings.
  // ------------------------------------------------------
  // Cookies.
  // Check that settings would block brave.com with any protocol and port.
  cookie_settings.CheckSettingsWouldBlock(url);
  cookie_settings.CheckSettingsWouldBlock(GURL("http://brave.com:5555"));
  cookie_settings.CheckSettingsWouldBlock(GURL("https://brave.com"));
  // Check that settings would allow allow.brave.com with any protocol and port.
  cookie_settings.CheckSettingsWouldAllow(url2);
  cookie_settings.CheckSettingsWouldAllow(GURL("https://allowed.brave.com"));
  // Check the pattern that didn't need to be migrated.
  cookie_settings.CheckSettingsWouldBlock(
      GURL("https://help.brave.com/article1.html"));
  // Would not block a different domain.
  cookie_settings.CheckSettingsAreDefault(GURL("http://brave2.com"));

  // Fingerprinting.
  // Check that settings would allow brave.com with any protocol and port.
  fp_settings.CheckSettingsWouldAllow(url);
  fp_settings.CheckSettingsWouldAllow(GURL("http://brave.com:5555"));
  fp_settings.CheckSettingsWouldAllow(GURL("https://brave.com"));
  // Would not allow a different domain.
  fp_settings.CheckSettingsAreDefault(GURL("http://brave2.com"));

  // HTTPSE.
  // Check that settings would block brave.com with any protocol and port.
  httpse_settings.CheckSettingsWouldBlock(url);
  httpse_settings.CheckSettingsWouldBlock(GURL("http://brave.com:5555"));
  // Would not block a different domain.
  httpse_settings.CheckSettingsAreDefault(GURL("http://brave2.com"));

  // Ads.
  // Check that settings would allow brave.com with any protocol and port.
  ads_settings.CheckSettingsWouldAllow(url);
  ads_settings.CheckSettingsWouldAllow(GURL("http://brave.com:5555"));
  ads_settings.CheckSettingsWouldAllow(GURL("https://brave.com"));
  // Would not allow a different domain.
  ads_settings.CheckSettingsAreDefault(GURL("http://brave2.com"));

  // Enabled.
  // Check that settings would block brave.com with any protocol and port.
  httpse_settings.CheckSettingsWouldBlock(url);
  httpse_settings.CheckSettingsWouldBlock(GURL("http://brave.com:5555"));
  httpse_settings.CheckSettingsWouldBlock(GURL("https://brave.com"));
  // Would not block a different domain.
  httpse_settings.CheckSettingsAreDefault(GURL("http://brave2.com"));

  // Scripts.
  // Check that settings would block brave.com with any protocol and port.
  script_settings.CheckSettingsWouldBlock(url);
  script_settings.CheckSettingsWouldBlock(GURL("http://brave.com:5555"));
  script_settings.CheckSettingsWouldBlock(GURL("https://brave.com"));
  // Would not block a different domain.
  script_settings.CheckSettingsAreDefault(GURL("http://brave2.com"));

  provider.ShutdownOnUIThread();
}

TEST_F(BravePrefProviderTest, TestShieldsSettingsMigrationVersion) {
  PrefService* prefs = testing_profile()->GetPrefs();
  BravePrefProvider provider(prefs, false /* incognito */,
                             true /* store_last_modified */,
                             false /* restore_session */);

  // Should have migrated when constrcuted (with profile).
  EXPECT_EQ(2, prefs->GetInteger(kBraveShieldsSettingsVersion));

  // Reset and check that migration runs.
  prefs->SetInteger(kBraveShieldsSettingsVersion, 1);
  provider.MigrateShieldsSettings(/*incognito*/ false);
  EXPECT_EQ(2, prefs->GetInteger(kBraveShieldsSettingsVersion));

  // Test that migration doesn't run for another version.
  prefs->SetInteger(kBraveShieldsSettingsVersion, 5);
  provider.MigrateShieldsSettings(/*incognito*/ false);
  EXPECT_EQ(5, prefs->GetInteger(kBraveShieldsSettingsVersion));

  provider.ShutdownOnUIThread();
}

}  //  namespace content_settings
