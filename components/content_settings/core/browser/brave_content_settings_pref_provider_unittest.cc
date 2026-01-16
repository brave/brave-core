/*  Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/json/values_util.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/content_settings_pref.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
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

using GURLSourcePair = std::pair<GURL, ContentSettingsType>;

ContentSettingsPattern SecondaryUrlToPattern(const GURL& gurl) {
  CHECK(gurl == GURL() || gurl == GURL("https://firstParty/*"));
  if (gurl == GURL()) {
    return ContentSettingsPattern::Wildcard();
  } else {
    return ContentSettingsPattern::FromString("https://firstParty/*");
  }
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

class ShieldsEnabledSetting : public ShieldsSetting {
 public:
  explicit ShieldsEnabledSetting(BravePrefProvider* provider)
      : ShieldsSetting(provider,
                       {{GURL(), ContentSettingsType::BRAVE_SHIELDS}}) {}
};

class DirectAccessContentSettings {
 public:
  explicit DirectAccessContentSettings(PrefService* prefs,
                                       ContentSettingsType content_type,
                                       const std::string& pref_name = {})
      : prefs_(prefs),
        content_type_(content_type),
        pref_name_(pref_name),
        info_(WebsiteSettingsRegistry::GetInstance()->Get(content_type_)) {
    Refresh();
  }

  void Refresh() { prefs_value_ = prefs_->GetDict(GetPrefName()).Clone(); }

  void AddRule(const ContentSettingsPattern& primary,
               const ContentSettingsPattern& secondary,
               ContentSetting setting) {
    AddRule(primary.ToString(), secondary.ToString(), setting);
  }

  void AddRule(const std::string& primary,
               const std::string& secondary,
               ContentSetting setting) {
    base::Value::Dict value;
    value.Set("setting", setting);

    prefs_value_.Set(primary + "," + secondary, std::move(value));
  }

  void AddRuleWithoutSettingValue(const std::string& primary,
                                  const std::string& secondary) {
    prefs_value_.Set(primary + "," + secondary, base::Value::Dict());
  }

  void Write() { prefs_->SetDict(GetPrefName(), prefs_value_.Clone()); }

  size_t GetRulesCount() const { return prefs_value_.size(); }

  base::Value GetSettingDirectly(const std::string& primary_pattern,
                                 const std::string& secondary_pattern = "*") {
    const auto* v =
        prefs_value_.Find(primary_pattern + "," + secondary_pattern);
    if (!v) {
      return base::Value();
    }
    return v->GetDict().Find("setting")->Clone();
  }

  base::Value GetContentSetting(const ProviderInterface* provider,
                                const GURL& primary_url,
                                const GURL& secondary_url = GURL::EmptyGURL()) {
    return TestUtils::GetContentSettingValue(
        provider, primary_url, secondary_url, content_type_, false);
  }

 private:
  const std::string& GetPrefName() const {
    return pref_name_.empty() ? info_->pref_name() : pref_name_;
  }
  const raw_ptr<PrefService> prefs_ = nullptr;
  const ContentSettingsType content_type_;
  const std::string pref_name_;
  const raw_ptr<const WebsiteSettingsInfo> info_ = nullptr;
  base::Value::Dict prefs_value_;
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
  DirectAccessContentSettings update(testing_profile()->GetPrefs(),
                                     ContentSettingsType::BRAVE_SHIELDS);

  update.AddRule(ContentSettingsPattern::Wildcard(),
                 ContentSettingsPattern::Wildcard(), CONTENT_SETTING_BLOCK);
  update.Write();

  base::RunLoop().RunUntilIdle();
  // Verify global has reset
  shields_enabled_settings.CheckSettingsAreDefault(example_url);
  provider.ShutdownOnUIThread();
}

TEST_F(BravePrefProviderTest, CosmeticFilteringMigration) {
  constexpr char kFirstParty[] = "https://firstparty";

  DirectAccessContentSettings cosmetic_filtering_v1(
      testing_profile()->GetPrefs(),
      brave_shields::CosmeticFilteringSetting::kContentSettingsType,
      "profile.content_settings.exceptions.cosmeticFiltering");

  // BLOCK_THIRD_PARTY
  cosmetic_filtering_v1.AddRule("brave.b3p", "*", CONTENT_SETTING_BLOCK);
  cosmetic_filtering_v1.AddRule("brave.b3p", kFirstParty,
                                CONTENT_SETTING_ALLOW);

  // ALLOW
  cosmetic_filtering_v1.AddRule("brave.allow", "*", CONTENT_SETTING_ALLOW);
  cosmetic_filtering_v1.AddRule("brave.allow", kFirstParty,
                                CONTENT_SETTING_ALLOW);

  // BLOCK
  cosmetic_filtering_v1.AddRule("brave.block", "*", CONTENT_SETTING_BLOCK);
  cosmetic_filtering_v1.AddRule("brave.block", kFirstParty,
                                CONTENT_SETTING_BLOCK);

  // Missing setting value https://github.com/brave/brave-browser/issues/49861
  cosmetic_filtering_v1.AddRuleWithoutSettingValue("brave.missing", "*");
  cosmetic_filtering_v1.AddRuleWithoutSettingValue("brave.missing",
                                                   kFirstParty);

  EXPECT_EQ(8u, cosmetic_filtering_v1.GetRulesCount());
  cosmetic_filtering_v1.Write();

  testing_profile()->GetPrefs()->ClearPref(
      "brave.cosmetic_filtering_migration");
  BravePrefProvider provider(
      testing_profile()->GetPrefs(), false /* incognito */,
      true /* store_last_modified */, false /* restore_session */);

  DirectAccessContentSettings cosmetic_filtering_v2(
      testing_profile()->GetPrefs(),
      brave_shields::CosmeticFilteringSetting::kContentSettingsType);

  EXPECT_EQ(3u, cosmetic_filtering_v2.GetRulesCount());

  const auto block3p = brave_shields::CosmeticFilteringSetting::ToValue(
      brave_shields::ControlType::BLOCK_THIRD_PARTY);
  const auto allow = brave_shields::CosmeticFilteringSetting::ToValue(
      brave_shields::ControlType::ALLOW);
  const auto block = brave_shields::CosmeticFilteringSetting::ToValue(
      brave_shields::ControlType::BLOCK);

  // Check there is no first-party rule anymore.
  EXPECT_EQ(base::Value(),
            cosmetic_filtering_v2.GetSettingDirectly("brave.b3p", kFirstParty));
  EXPECT_EQ(block3p,
            cosmetic_filtering_v2.GetSettingDirectly("brave.b3p", "*"));
  EXPECT_EQ(block3p, cosmetic_filtering_v2.GetContentSetting(
                         &provider, GURL("https://brave.b3p")));

  // Check there is no first-party rule anymore.
  EXPECT_EQ(base::Value(), cosmetic_filtering_v2.GetSettingDirectly(
                               "brave.allow", kFirstParty));
  EXPECT_EQ(allow,
            cosmetic_filtering_v2.GetSettingDirectly("brave.allow", "*"));
  EXPECT_EQ(allow, cosmetic_filtering_v2.GetContentSetting(
                       &provider, GURL("https://brave.allow")));

  // Check there is no first-party rule anymore.
  EXPECT_EQ(base::Value(), cosmetic_filtering_v2.GetSettingDirectly(
                               "brave.block", kFirstParty));
  EXPECT_EQ(block,
            cosmetic_filtering_v2.GetSettingDirectly("brave.block", "*"));
  EXPECT_EQ(block, cosmetic_filtering_v2.GetContentSetting(
                       &provider, GURL("https://brave.block")));

  provider.ShutdownOnUIThread();
}

}  //  namespace content_settings
