/*  Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/json/values_util.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/content_settings_pref.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/core/test/content_settings_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content_settings {

namespace {

constexpr char kUserProfilePluginsPath[] =
    "profile.content_settings.exceptions.plugins";

constexpr char kExpirationPath[] = "expiration";
constexpr char kLastModifiedPath[] = "last_modified";
constexpr char kSessionModelPath[] = "model";
constexpr char kSettingPath[] = "setting";
constexpr char kPerResourcePath[] = "per_resource";

using GURLSourcePair = std::pair<GURL, ContentSettingsType>;

ContentSettingsPattern SecondaryUrlToPattern(const GURL& gurl) {
  CHECK(gurl == GURL() || gurl == GURL("https://firstParty/*"));
  if (gurl == GURL())
    return ContentSettingsPattern::Wildcard();
  else
    return ContentSettingsPattern::FromString("https://firstParty/*");
}

base::Value::Dict* InitializeCommonSettingsAndGetPerResourceDictionary(
    base::Value::Dict* dict,
    const base::Time& last_modified_time) {
  const uint64_t last_modified_time_in_ms =
      last_modified_time.ToDeltaSinceWindowsEpoch().InMicroseconds();

  dict->SetByDottedPath(kExpirationPath, 0);
  dict->SetByDottedPath(kLastModifiedPath,
                        base::NumberToString(last_modified_time_in_ms));
  dict->SetByDottedPath(
      kSessionModelPath,
      static_cast<int>(content_settings::mojom::SessionModel::DURABLE));

  return dict->EnsureDict(kPerResourcePath);
}

void InitializeAllShieldSettingsInDictionary(
    base::Value::Dict* dict,
    const base::Time& last_modified_time,
    int value) {
  base::Value::Dict* per_resource_dict =
      InitializeCommonSettingsAndGetPerResourceDictionary(dict,
                                                          last_modified_time);
  per_resource_dict->Set(brave_shields::kAds, value);
  per_resource_dict->Set(brave_shields::kCookies, value);
  per_resource_dict->Set(brave_shields::kCosmeticFiltering, value);
  per_resource_dict->Set(brave_shields::kFingerprintingV2, value);
  per_resource_dict->Set(brave_shields::kHTTPUpgradableResources, value);
  per_resource_dict->Set(brave_shields::kReferrers, value);
  per_resource_dict->Set(brave_shields::kTrackers, value);
}

void InitializeBraveShieldsSettingInDictionary(
    base::Value::Dict* dict,
    const base::Time& last_modified_time,
    int value) {
  base::Value::Dict* per_resource_dict =
      InitializeCommonSettingsAndGetPerResourceDictionary(dict,
                                                          last_modified_time);
  per_resource_dict->Set(brave_shields::kBraveShields, value);
}

void InitializeUnsupportedShieldSettingInDictionary(
    base::Value::Dict* dict,
    const base::Time& last_modified_time) {
  base::Value::Dict* per_resource_dict =
      InitializeCommonSettingsAndGetPerResourceDictionary(dict,
                                                          last_modified_time);
  per_resource_dict->Set("unknown_setting", 1);
}

void CheckMigrationFromResourceIdentifierForDictionary(
    const base::Value::Dict& dict,
    std::string_view patterns_string,
    const std::optional<base::Time> expected_last_modified,
    std::optional<int> expected_setting_value) {
  const base::Value::Dict* settings_dict = dict.FindDict(patterns_string);
  EXPECT_NE(settings_dict, nullptr);

  auto actual_value = settings_dict->FindInt(kSettingPath);
  EXPECT_EQ(base::ValueToTime(settings_dict->Find(kLastModifiedPath)),
            expected_last_modified);
  EXPECT_EQ(GetSessionModelFromDictionary(*settings_dict, kSessionModelPath),
            content_settings::mojom::SessionModel::DURABLE);
  EXPECT_EQ(actual_value, expected_setting_value);
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
          pattern, SecondaryUrlToPattern(url_source.first), url_source.second,
          ContentSettingToValue(setting), {});
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

  void CheckSettingsWouldAsk(const GURL& url) const {
    CheckSettings(url, CONTENT_SETTING_ASK);
  }

 protected:
  virtual void CheckSettings(const GURL& url, ContentSetting setting) const {
    for (const auto& url_source : urls_) {
      EXPECT_EQ(setting,
                TestUtils::GetContentSetting(provider_, url, url_source.first,
                                             url_source.second, false));
    }
  }

  raw_ptr<BravePrefProvider> provider_ = nullptr;
  const std::vector<GURLSourcePair> urls_;
};

class ShieldsCookieSetting : public ShieldsSetting {
 public:
  ShieldsCookieSetting(BravePrefProvider* provider, PrefService* prefs)
      : ShieldsSetting(
            provider,
            {{GURL(), ContentSettingsType::BRAVE_COOKIES},
             {GURL("https://firstParty/*"), ContentSettingsType::BRAVE_COOKIES},
             {GURL(), ContentSettingsType::BRAVE_REFERRERS}}),
        prefs_(prefs) {}

  void RollbackShieldsCookiesVersion() {
    base::Value::Dict shieldsCookies =
        prefs_->GetDict("profile.content_settings.exceptions.shieldsCookiesV3")
            .Clone();
    prefs_->Set("profile.content_settings.exceptions.shieldsCookies",
                base::Value(std::move(shieldsCookies)));
    prefs_->ClearPref("profile.content_settings.exceptions.shieldsCookiesV3");
  }

 private:
  void CheckSettings(const GURL& url, ContentSetting setting) const override {
    if (prefs_->GetInteger(kBraveShieldsSettingsVersion) < 3) {
      return ShieldsSetting::CheckSettings(url, setting);
    }
    // We need this because if version below than 3 brave cookies patterns
    // are reversed.
    for (const auto& url_source : urls_) {
      if (url_source.second == ContentSettingsType::BRAVE_COOKIES) {
        EXPECT_EQ(setting,
                  TestUtils::GetContentSetting(provider_, url_source.first, url,
                                               url_source.second, false));
      } else {
        EXPECT_EQ(setting,
                  TestUtils::GetContentSetting(provider_, url, url_source.first,
                                               url_source.second, false));
      }
    }
  }

  raw_ptr<PrefService> prefs_ = nullptr;
};

class CookieSettings : public ShieldsSetting {
 public:
  explicit CookieSettings(BravePrefProvider* provider)
      : ShieldsSetting(provider, {}) {}
};

class ShieldsFingerprintingSetting : public ShieldsSetting {
 public:
  explicit ShieldsFingerprintingSetting(BravePrefProvider* provider)
      : ShieldsSetting(provider, {}) {}

  void SetPreMigrationSettings(const ContentSettingsPattern& pattern,
                               ContentSetting setting) override {
    provider_->SetWebsiteSettingForTest(
        pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::BRAVE_FINGERPRINTING_V2,
        ContentSettingToValue(setting), {});
  }

  void SetPreMigrationSettingsWithSecondary(
      const ContentSettingsPattern& pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSetting setting) {
    provider_->SetWebsiteSettingForTest(
        pattern, secondary_pattern,
        ContentSettingsType::BRAVE_FINGERPRINTING_V2,
        ContentSettingToValue(setting), {});
  }
};

class ShieldsHTTPSESetting : public ShieldsSetting {
 public:
  explicit ShieldsHTTPSESetting(BravePrefProvider* provider)
      : ShieldsSetting(
            provider,
            {{GURL(), ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES}}) {}
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
      : ShieldsSetting(provider,
                       {{GURL(), ContentSettingsType::BRAVE_SHIELDS}}) {}
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

  ShieldsCookieSetting cookie_settings(&provider,
                                       testing_profile()->GetPrefs());
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
  testing_profile()->GetPrefs()->SetInteger(kBraveShieldsSettingsVersion, 1);

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
  cookie_settings.RollbackShieldsCookiesVersion();
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

  // Should have migrated when constructed (with profile).
  EXPECT_EQ(4, prefs->GetInteger(kBraveShieldsSettingsVersion));

  // Reset and check that migration runs.
  prefs->SetInteger(kBraveShieldsSettingsVersion, 1);
  provider.MigrateShieldsSettings(/*incognito*/ false);
  EXPECT_EQ(4, prefs->GetInteger(kBraveShieldsSettingsVersion));

  // Test that migration doesn't run for another version.
  prefs->SetInteger(kBraveShieldsSettingsVersion, 5);
  provider.MigrateShieldsSettings(/*incognito*/ false);
  EXPECT_EQ(5, prefs->GetInteger(kBraveShieldsSettingsVersion));

  provider.ShutdownOnUIThread();
}

TEST_F(BravePrefProviderTest, MigrateFPShieldsSettings) {
  BravePrefProvider provider(
      testing_profile()->GetPrefs(), false /* incognito */,
      true /* store_last_modified */, false /* restore_session */);

  ShieldsFingerprintingSetting fp_settings(&provider);

  GURL url("http://brave.com:8080/");
  ContentSettingsPattern pattern = ContentSettingsPattern::FromURL(url);
  fp_settings.SetPreMigrationSettings(pattern, CONTENT_SETTING_BLOCK);

  GURL url2("http://brave.com:3030");
  ContentSettingsPattern pattern2 = ContentSettingsPattern::FromURL(url2);
  fp_settings.SetPreMigrationSettingsWithSecondary(
      pattern2, ContentSettingsPattern::FromString("https://balanced/*"),
      CONTENT_SETTING_BLOCK);

  GURL url3("http://brave.com:8181/");
  ContentSettingsPattern pattern3 = ContentSettingsPattern::FromURL(url3);
  fp_settings.SetPreMigrationSettings(pattern3, CONTENT_SETTING_ALLOW);

  GURL url4("http://brave.com:8282/");
  ContentSettingsPattern pattern4 = ContentSettingsPattern::FromURL(url4);
  fp_settings.SetPreMigrationSettings(pattern4, CONTENT_SETTING_ASK);

  provider.MigrateFingerprintingSettings();
  provider.MigrateFingerprintingSetingsToOriginScoped();
#if BUILDFLAG(IS_ANDROID)
  fp_settings.CheckSettingsWouldAsk(url);
#else
  fp_settings.CheckSettingsWouldBlock(url);
#endif
  fp_settings.CheckSettingsWouldAsk(url2);

  // ignore attempts to set balanced settings
  provider.SetWebsiteSetting(
      pattern2, ContentSettingsPattern::FromString("https://balanced/*"),
      ContentSettingsType::BRAVE_FINGERPRINTING_V2,
      ContentSettingToValue(CONTENT_SETTING_BLOCK), {});
  std::vector<Rule> rules;
  auto rule_iterator = provider.GetRuleIterator(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, false,
      content_settings::PartitionKey::WipGetDefault());
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    EXPECT_NE(
        rule->secondary_pattern.ToString(),
        ContentSettingsPattern::FromString("https://balanced/*").ToString());
  }
  rule_iterator.reset();

  fp_settings.SetPreMigrationSettingsWithSecondary(
      pattern2, ContentSettingsPattern::FromString("https://balanced/*"),
      CONTENT_SETTING_BLOCK);
  // should ignore any balanced setting set after the migration
  fp_settings.CheckSettingsWouldAsk(url2);

  fp_settings.CheckSettingsWouldAllow(url3);
  fp_settings.CheckSettingsWouldAsk(url4);

  provider.ShutdownOnUIThread();
}

TEST_F(BravePrefProviderTest, TestShieldsSettingsMigrationFromResourceIDs) {
  PrefService* pref_service = testing_profile()->GetPrefs();
  BravePrefProvider provider(pref_service, false /* incognito */,
                             true /* store_last_modified */,
                             false /* restore_session */);

  // Manually write settings under the PLUGINS type using the no longer existing
  // ResourceIdentifier names, and then perform the migration.
  std::optional<ScopedDictPrefUpdate> plugins(std::in_place, pref_service,
                                              kUserProfilePluginsPath);

  base::Time expected_last_modified = base::Time::Now();

  // Seed global shield settings with non-default values.
  base::Value::Dict* global_settings = plugins.value()->EnsureDict("*,*");

  const int expected_global_settings_value = 1;
  InitializeAllShieldSettingsInDictionary(
      global_settings, expected_last_modified, expected_global_settings_value);

  // Change all of those global settings for www.example.com.
  base::Value::Dict* example_settings =
      plugins.value()->EnsureDict("www.example.com,*");

  const int expected_example_com_settings_value = 1;
  InitializeAllShieldSettingsInDictionary(example_settings,
                                          expected_last_modified,
                                          expected_example_com_settings_value);

  // Disable Brave Shields for www.brave.com.
  base::Value::Dict* brave_settings =
      plugins.value()->EnsureDict("www.brave.com,*");

  const int expected_brave_com_settings_value = 1;
  InitializeBraveShieldsSettingInDictionary(brave_settings,
                                            expected_last_modified,
                                            expected_brave_com_settings_value);

  // Destroying `plugins` at this point, as otherwise it will be holding a
  // dangling pointer, after `MigrateShieldsSettingsFromResourceIds()`.
  plugins = std::nullopt;

  provider.MigrateShieldsSettingsFromResourceIds();

  // Check migration for all the settings has been properly done.
  for (auto content_type : GetShieldsContentSettingsTypes()) {
    const auto& brave_shields_dict =
        pref_service->GetDict(GetShieldsSettingUserPrefsPath(
            GetShieldsContentTypeName(content_type)));

    if (content_type == ContentSettingsType::BRAVE_SHIELDS) {
      // We only changed the value of BRAVE_SHIELDS in www.brave.com.
      CheckMigrationFromResourceIdentifierForDictionary(
          brave_shields_dict, "www.brave.com,*", expected_last_modified,
          expected_brave_com_settings_value);
    } else {
      // All the other settings we changed them globally and in www.example.com.
      CheckMigrationFromResourceIdentifierForDictionary(
          brave_shields_dict, "*,*", expected_last_modified,
          expected_global_settings_value);
      CheckMigrationFromResourceIdentifierForDictionary(
          brave_shields_dict, "www.example.com,*", expected_last_modified,
          expected_example_com_settings_value);
    }
  }

  provider.ShutdownOnUIThread();
}

TEST_F(BravePrefProviderTest, TestShieldsSettingsMigrationFromUnknownSettings) {
  PrefService* pref_service = testing_profile()->GetPrefs();
  BravePrefProvider provider(pref_service, false /* incognito */,
                             true /* store_last_modified */,
                             false /* restore_session */);

  // Manually write invalid settings under the PLUGINS type using the no longer
  // existing ResourceIdentifier names, to attempt the migration.
  std::optional<ScopedDictPrefUpdate> plugins(std::in_place, pref_service,
                                              kUserProfilePluginsPath);

  // Seed both global and per-site shield settings preferences using unsupported
  // names, so that we can test that Brave doesn't crash while attempting the
  // migration and simply ignore those unsupported names instead.
  //
  // For a list of supported names, see |kBraveContentSettingstypes| inside the
  // components/content_settings/core/browser/content_settings_registry.cc
  // override, in the chromium_src/ directory.
  base::Value::Dict* global_settings = plugins.value()->EnsureDict("*,*");
  InitializeUnsupportedShieldSettingInDictionary(global_settings,
                                                 base::Time::Now());
  base::Value::Dict* example_settings =
      plugins.value()->EnsureDict("www.example.com,*");
  InitializeUnsupportedShieldSettingInDictionary(example_settings,
                                                 base::Time::Now());

  // Destroying `plugins` at this point, as otherwise it will be holding a
  // dangling pointer, after `MigrateShieldsSettingsFromResourceIds()`.
  plugins = std::nullopt;

  // Doing the migration below should NOT get a crash due to invalid settings.
  provider.MigrateShieldsSettingsFromResourceIds();

  // New Shields-specific content settings types should have been created due to
  // the migration, but all should be empty since only invalid data was fed.
  for (auto content_type : GetShieldsContentSettingsTypes()) {
    const auto& brave_shields_dict =
        pref_service->GetDict(GetShieldsSettingUserPrefsPath(
            GetShieldsContentTypeName(content_type)));
    EXPECT_TRUE(brave_shields_dict.empty());
  }

  provider.ShutdownOnUIThread();
}

TEST_F(BravePrefProviderTest, TestShieldsSettingsMigrationV2toV4) {
  BravePrefProvider provider(
      testing_profile()->GetPrefs(), false /* incognito */,
      true /* store_last_modified */, false /* restore_session */);

  ShieldsCookieSetting shields_cookie_settings(&provider,
                                               testing_profile()->GetPrefs());
  CookieSettings cookie_settings(&provider);
  ShieldsEnabledSetting shields_enabled_settings(&provider);

  GURL blocked("http://brave.com:8080/");
  GURL allowed("http://allowed.brave.com:3030");

  ContentSettingsPattern allowed_pattern =
      ContentSettingsPattern::FromURL(blocked);
  ContentSettingsPattern blocked_pattern =
      ContentSettingsPattern::FromURL(allowed);
  // ShieldsCookies.
  shields_cookie_settings.SetPreMigrationSettings(allowed_pattern,
                                                  CONTENT_SETTING_BLOCK);
  shields_cookie_settings.SetPreMigrationSettings(blocked_pattern,
                                                  CONTENT_SETTING_ALLOW);

  // Disable shields -> cookies should be allowed.
  shields_enabled_settings.SetPreMigrationSettings(blocked_pattern,
                                                   CONTENT_SETTING_BLOCK);

  shields_cookie_settings.RollbackShieldsCookiesVersion();
  testing_profile()->GetPrefs()->SetInteger(kBraveShieldsSettingsVersion, 2);
  provider.MigrateShieldsSettings(/*incognito*/ false);

  shields_cookie_settings.CheckSettingsWouldAllow(allowed);

  // BRAVE_COOKIES blocked but COOKIES allowed.
  shields_cookie_settings.CheckSettingsWouldBlock(blocked);
  cookie_settings.CheckSettingsWouldAllow(blocked);

  // Enable shields -> cookies should be blocked according to settings.
  shields_enabled_settings.SetPreMigrationSettings(blocked_pattern,
                                                   CONTENT_SETTING_ALLOW);
  shields_cookie_settings.CheckSettingsWouldBlock(blocked);
  cookie_settings.CheckSettingsWouldBlock(blocked);

  // V3 to V4
  testing_profile()->GetPrefs()->SetInteger(kBraveShieldsSettingsVersion, 3);
  provider.MigrateShieldsSettings(/*incognito*/ false);

  shields_cookie_settings.CheckSettingsWouldBlock(blocked);
  cookie_settings.CheckSettingsWouldBlock(blocked);

  provider.ShutdownOnUIThread();
}

TEST_F(BravePrefProviderTest, EnsureNoWildcardEntries) {
  BravePrefProvider provider(
      testing_profile()->GetPrefs(), false /* incognito */,
      true /* store_last_modified */, false /* restore_session */);
  ShieldsEnabledSetting shields_enabled_settings(&provider);
  GURL example_url("https://example.com");
  shields_enabled_settings.CheckSettingsAreDefault(example_url);
  // Set wildcard entry
  auto pattern = ContentSettingsPattern::Wildcard();
  provider.SetWebsiteSetting(pattern, pattern,
                             ContentSettingsType::BRAVE_SHIELDS,
                             base::Value(CONTENT_SETTING_ALLOW), {});
  // Verify global has changed
  shields_enabled_settings.CheckSettingsWouldAllow(example_url);
  // Remove wildcards
  provider.EnsureNoWildcardEntries(ContentSettingsType::BRAVE_SHIELDS);
  // Verify global has reset
  shields_enabled_settings.CheckSettingsAreDefault(example_url);

  // Simulate sync updates pref directly.
  base::Value::Dict value;
  value.Set("expiration", "0");
  value.Set("last_modified", "13304670271801570");
  value.Set("model", 0);
  value.Set("setting", 2);

  base::Value::Dict update;
  update.Set("*,*", std::move(value));

  testing_profile()->GetPrefs()->SetDict(
      "profile.content_settings.exceptions.braveShields", std::move(update));
  base::RunLoop().RunUntilIdle();
  // Verify global has reset
  shields_enabled_settings.CheckSettingsAreDefault(example_url);
  provider.ShutdownOnUIThread();
}

}  //  namespace content_settings
