/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/containers/contains.h"
#include "base/json/values_util.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "build/build_config.h"
#include "components/content_settings/core/browser/content_settings_info.h"
#include "components/content_settings/core/browser/content_settings_pref.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content_settings {

namespace {

constexpr char kObsoleteShieldCookies[] =
    "profile.content_settings.exceptions.shieldsCookies";
constexpr char kBraveShieldsFPSettingsMigration[] =
    "brave.shields_fp_settings_migration";

constexpr char kGoogleAuthPattern[] =
    "https://accounts.google.com/o/oauth2/auth/*";
constexpr char kFirebasePattern[] = "https://[*.]firebaseapp.com/__/auth/*";

const char kExpirationPath[] = "expiration";
const char kLastModifiedPath[] = "last_modified";
const char kSessionModelPath[] = "model";
const char kSettingPath[] = "setting";
const char kPerResourcePath[] = "per_resource";

Rule CloneRule(const Rule& original_rule) {
  return Rule(original_rule.primary_pattern, original_rule.secondary_pattern,
              original_rule.value.Clone(), original_rule.expiration,
              original_rule.session_model);
}

bool IsActive(const Rule& cookie_rule, const std::vector<Rule>& shield_rules) {
  // don't include default rules in the iterator
  if (cookie_rule.primary_pattern == ContentSettingsPattern::Wildcard() &&
      cookie_rule.secondary_pattern == ContentSettingsPattern::Wildcard()) {
    return false;
  }

  for (const auto& shield_rule : shield_rules) {
    auto primary_compare =
        shield_rule.primary_pattern.Compare(cookie_rule.secondary_pattern);
    if (primary_compare == ContentSettingsPattern::IDENTITY ||
        primary_compare == ContentSettingsPattern::SUCCESSOR) {
      return ValueToContentSetting(shield_rule.value) != CONTENT_SETTING_BLOCK;
    }
  }

  return true;
}

}  // namespace

// static
void BravePrefProvider::CopyPluginSettingsForMigration(PrefService* prefs) {
  if (!prefs->HasPrefPath("profile.content_settings.exceptions.plugins")) {
    return;
  }

  auto* plugins =
      prefs->GetDictionary("profile.content_settings.exceptions.plugins");
  prefs->Set("brave.migrate.content_settings.exceptions.plugins", *plugins);

  // Upstream won't clean this up for ANDROID, need to do it ourselves.
  prefs->ClearPref("profile.content_settings.exceptions.plugins");
}

BravePrefProvider::BravePrefProvider(PrefService* prefs,
                                     bool off_the_record,
                                     bool store_last_modified,
                                     bool restore_session)
    : PrefProvider(prefs, off_the_record, store_last_modified, restore_session),
      initialized_(false),
      store_last_modified_(store_last_modified),
      weak_factory_(this) {
  pref_change_registrar_.Init(prefs);

  pref_change_registrar_.Add(
      kGoogleLoginControlType,
      base::BindRepeating(&BravePrefProvider::OnCookiePrefsChanged,
                          base::Unretained(this)));

  MigrateShieldsSettings(off_the_record_);
  MigrateFingerprintingSettings();
  MigrateFingerprintingSetingsToOriginScoped();

  OnCookieSettingsChanged(ContentSettingsType::BRAVE_COOKIES);

  // Enable change notifications after initial setup to avoid notification spam
  initialized_ = true;
  AddObserver(this);
}

BravePrefProvider::~BravePrefProvider() = default;

void BravePrefProvider::ShutdownOnUIThread() {
  RemoveObserver(this);
  pref_change_registrar_.RemoveAll();
  PrefProvider::ShutdownOnUIThread();
}

// static
void BravePrefProvider::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  PrefProvider::RegisterProfilePrefs(registry);
  // Register shields settings migration pref.
  registry->RegisterIntegerPref(kBraveShieldsSettingsVersion, 1);

  // migration of obsolete plugin prefs
  registry->RegisterDictionaryPref(
      "brave.migrate.content_settings.exceptions.plugins");

#if BUILDFLAG(IS_ANDROID)
  // This path is no longer registered upstream but we still need it to migrate
  // Shields settings away from ResourceIdentifier on Android.
  if (!registry->defaults()->GetValue(
          "profile.content_settings.exceptions.plugins", nullptr)) {
    registry->RegisterDictionaryPref(
        "profile.content_settings.exceptions.plugins");
  }
#endif
  registry->RegisterBooleanPref(kBraveShieldsFPSettingsMigration, false);
  registry->RegisterDictionaryPref(kObsoleteShieldCookies);
}

void BravePrefProvider::MigrateShieldsSettings(bool incognito) {
  // Incognito inherits from regular profile, so nothing to do.
  // Guest doesn't inherit, but only keeps settings for the duration of the
  // session, so also nothing to do.
  if (incognito)
    return;

  const int version = prefs_->GetInteger(kBraveShieldsSettingsVersion);
  auto* shields_cookies = prefs_->GetDictionary(kObsoleteShieldCookies);
  if (shields_cookies) {
    if (version < 4) {
      prefs_->Set("profile.content_settings.exceptions.shieldsCookiesV3",
                  *shields_cookies);
    }
  }

  // Fix any wildcard entries that could cause issues like
  // https://github.com/brave/brave-browser/issues/23113
  constexpr const ContentSettingsType kNoWildcardTypes[] = {
      ContentSettingsType::BRAVE_SHIELDS,
  };

  auto* content_settings =
      content_settings::ContentSettingsRegistry::GetInstance();
  for (const auto content_type : kNoWildcardTypes) {
    const auto* info = content_settings->Get(content_type);
    if (!info)
      continue;

    // We need to bind PostTask to break the stack trace because if we get there
    // from the sync the ChangeProcessor will ignore this update.
    if (!pref_change_registrar_.IsObserved(
            info->website_settings_info()->pref_name())) {
      pref_change_registrar_.Add(
          info->website_settings_info()->pref_name(),
          base::BindPostTask(
              base::SequencedTaskRunnerHandle::Get(),
              base::BindRepeating(&BravePrefProvider::EnsureNoWildcardEntries,
                                  weak_factory_.GetWeakPtr(), content_type)));
    }
    EnsureNoWildcardEntries(content_type);
  }

  // Prior to Chromium 88, we used the "plugins" ContentSettingsType along with
  // ResourceIdentifiers to store our settings, which we need to migrate now
  // first of all, before attempting any other migration.
  MigrateShieldsSettingsFromResourceIds();

  // Now carry on with any other migration that we might need.
  MigrateShieldsSettingsV1ToV2();

  MigrateShieldsSettingsV2ToV3();

  MigrateShieldsSettingsV3ToV4(version);
}

void BravePrefProvider::EnsureNoWildcardEntries(
    ContentSettingsType content_type) {
  // ContentSettingsType::BRAVE_SHIELDS should not have wildcard entries, i.e.
  // there is no global disabled value.
  // TODO(petemill): This should also be done for the other shields
  // content settings types, and we can use default boolean prefs to represent
  // defaults, e.g. `profile.default_content_setting_values.https_everywhere`.
  SetWebsiteSetting(ContentSettingsPattern::Wildcard(),
                    ContentSettingsPattern::Wildcard(), content_type,
                    base::Value(), {});
}

void BravePrefProvider::MigrateShieldsSettingsFromResourceIds() {
  BravePrefProvider::CopyPluginSettingsForMigration(prefs_);

  const auto& plugins_dict =
      prefs_->GetDict("brave.migrate.content_settings.exceptions.plugins");

  for (const auto [key, value] : plugins_dict) {
    const std::string& patterns_string(key);
    const base::Value::Dict* settings_dict = value.GetIfDict();
    DCHECK(settings_dict);

    base::Time expiration =
        base::ValueToTime(settings_dict->Find(kExpirationPath))
            .value_or(base::Time());
    SessionModel session_model =
        GetSessionModelFromDictionary(*settings_dict, kSessionModelPath);

    const base::Value::Dict* resource_dict =
        settings_dict->FindDictByDottedPath(kPerResourcePath);
    if (resource_dict) {
      base::Time last_modified =
          base::ValueToTime(settings_dict->Find(kLastModifiedPath))
              .value_or(base::Time());
      for (const auto [key, value] : *resource_dict) {
        const std::string& resource_identifier(key);
        std::string shields_preference_name;

        // For "ads" and "cookies" we need to adapt the name to the new one,
        // otherwise it will refer to upstream's "ads" and "cookies" settings.
        if (resource_identifier == brave_shields::kObsoleteAds)
          shields_preference_name = brave_shields::kAds;
        else if (resource_identifier == brave_shields::kObsoleteCookies)
          shields_preference_name = brave_shields::kObsoleteShieldsCookies;
        else
          shields_preference_name = resource_identifier;

        // Protect against non registered paths (unlikely, but possible).
        if (!IsShieldsContentSettingsTypeName(shields_preference_name))
          continue;

        // Drop a "global" value of brave shields, that actually shouldn't exist
        // at all since we don't have any global toggle for this.
        if (shields_preference_name == brave_shields::kBraveShields &&
            patterns_string == "*,*") {
          continue;
        }

        DCHECK(value.is_int());
        int setting = value.GetInt();
        DCHECK_NE(CONTENT_SETTING_DEFAULT, setting);

        MigrateShieldsSettingsFromResourceIdsForOneType(
            GetShieldsSettingUserPrefsPath(shields_preference_name),
            patterns_string, expiration, last_modified, session_model, setting);
      }
    }
  }

  // Finally clean this up now that Shields' settings have been migrated.
  prefs_->ClearPref("brave.migrate.content_settings.exceptions.plugins");
}

void BravePrefProvider::MigrateShieldsSettingsFromResourceIdsForOneType(
    const std::string& preference_path,
    const std::string& patterns_string,
    const base::Time& expiration,
    const base::Time& last_modified,
    SessionModel session_model,
    int setting) {
  // Non-supported preference paths should have been filtered out already.
  CHECK(prefs_->HasPrefPath(preference_path))
      << "Attempted to migrate unsupported shields setting.";

  prefs::ScopedDictionaryPrefUpdate update(prefs_, preference_path);
  std::unique_ptr<prefs::DictionaryValueUpdate> shield_settings = update.Get();

  std::unique_ptr<prefs::DictionaryValueUpdate> shield_settings_dictionary;
  bool found = shield_settings->GetDictionaryWithoutPathExpansion(
      patterns_string, &shield_settings_dictionary);

  if (!found) {
    shield_settings_dictionary =
        shield_settings->SetDictionaryWithoutPathExpansion(
            patterns_string, std::make_unique<base::DictionaryValue>());
  }
  DCHECK(shield_settings_dictionary);

  shield_settings_dictionary->SetKey(
      kExpirationPath,
      base::Value(base::NumberToString(
          expiration.ToDeltaSinceWindowsEpoch().InMicroseconds())));
  shield_settings_dictionary->SetKey(
      kLastModifiedPath,
      base::Value(base::NumberToString(
          last_modified.ToDeltaSinceWindowsEpoch().InMicroseconds())));
  shield_settings_dictionary->SetKey(
      kSessionModelPath, base::Value(static_cast<int>(session_model)));
  shield_settings_dictionary->SetKey(kSettingPath, base::Value(setting));
}

void BravePrefProvider::MigrateShieldsSettingsV1ToV2() {
  // Check if migration is needed.
  if (prefs_->GetInteger(kBraveShieldsSettingsVersion) != 1)
    return;

  // All sources in Brave-specific ContentSettingsType(s) we want to migrate.
  for (const auto& content_type : GetShieldsContentSettingsTypes())
    MigrateShieldsSettingsV1ToV2ForOneType(content_type);

  // ContentSettingsType::JAVASCRIPT.
  MigrateShieldsSettingsV1ToV2ForOneType(ContentSettingsType::JAVASCRIPT);

  // Mark migration as done.
  prefs_->SetInteger(kBraveShieldsSettingsVersion, 2);
}

void BravePrefProvider::MigrateShieldsSettingsV2ToV3() {
  // Check if migration is needed.
  if (prefs_->GetInteger(kBraveShieldsSettingsVersion) != 2)
    return;

  const ContentSettingsPattern& wildcard = ContentSettingsPattern::Wildcard();
  const ContentSettingsPattern first_party(
      ContentSettingsPattern::FromString("https://firstParty/*"));

  auto rule_iterator =
      PrefProvider::GetRuleIterator(ContentSettingsType::BRAVE_COOKIES,
                                    /*off_the_record*/ false);

  using OldRule = std::pair<ContentSettingsPattern, ContentSettingsPattern>;
  std::vector<OldRule> old_rules;
  std::vector<Rule> new_rules;

  // Find rules that can be migrated and create replacement rules for them.
  while (rule_iterator && rule_iterator->HasNext()) {
    auto old_rule = rule_iterator->Next();
    old_rules.emplace_back(old_rule.primary_pattern,
                           old_rule.secondary_pattern);
    if (old_rule.primary_pattern == wildcard &&
        (old_rule.secondary_pattern == wildcard ||
         old_rule.secondary_pattern == first_party)) {
      // Remove default rules from BRAVE_COKIES because it's already mapped to
      // the Chromium prefs.
      continue;
    }
    if (old_rule.secondary_pattern == wildcard && !new_rules.empty() &&
        new_rules.back().secondary_pattern == old_rule.primary_pattern &&
        old_rule.value == new_rules.back().value) {
      // Remove the "first-party" rule because it is a predecessor of a general
      // rule that we are going to add.
      new_rules.pop_back();
    }

    Rule new_rule;
    new_rule.expiration = old_rule.expiration;
    new_rule.session_model = old_rule.session_model;
    new_rule.value = std::move(old_rule.value);
    // Exchange primary and secondary patterns.
    new_rule.secondary_pattern = old_rule.primary_pattern;
    new_rule.primary_pattern = old_rule.secondary_pattern;
    // Replace first party placeholder with actual pattern
    if (new_rule.primary_pattern == first_party) {
      new_rule.primary_pattern =
          content_settings::CreatePrimaryPattern(new_rule.secondary_pattern);
    }
    new_rules.push_back(std::move(new_rule));
  }
  rule_iterator.reset();

  ClearAllContentSettingsRules(ContentSettingsType::BRAVE_COOKIES);
  for (auto&& rule : new_rules) {
    SetWebsiteSettingInternal(rule.primary_pattern, rule.secondary_pattern,
                              ContentSettingsType::BRAVE_COOKIES,
                              std::move(rule.value),
                              {rule.expiration, rule.session_model});
  }

  // Mark migration as done.
  prefs_->SetInteger(kBraveShieldsSettingsVersion, 3);
}

void BravePrefProvider::MigrateShieldsSettingsV3ToV4(int start_version) {
  if (prefs_->GetInteger(kBraveShieldsSettingsVersion) != 3)
    return;

  if (start_version == 3) {
    // Because of
    // https://github.com/brave/brave-browser/issues/24119 the cookies
    // overwritten by the obsolete cookies. Repeat v2 to v3 migration.
    prefs_->SetInteger(kBraveShieldsSettingsVersion, 2);
    MigrateShieldsSettingsV2ToV3();
  }
  prefs_->SetInteger(kBraveShieldsSettingsVersion, 4);
}

void BravePrefProvider::MigrateShieldsSettingsV1ToV2ForOneType(
    ContentSettingsType content_type) {
  using OldRule = std::pair<ContentSettingsPattern, ContentSettingsPattern>;
  // Find rules that can be migrated and create replacement rules for them.
  std::vector<OldRule> old_rules;
  std::vector<Rule> new_rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(content_type,
                                                     /*off_the_record*/ false);
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    auto new_primary_pattern =
        ConvertPatternToWildcardSchemeAndPort(rule.primary_pattern);
    auto new_secondary_pattern =
        ConvertPatternToWildcardSchemeAndPort(rule.secondary_pattern);
    if (new_primary_pattern || new_secondary_pattern) {
      old_rules.emplace_back(rule.primary_pattern, rule.secondary_pattern);
      new_rules.emplace_back(
          new_primary_pattern.value_or(rule.primary_pattern),
          new_secondary_pattern.value_or(rule.secondary_pattern),
          rule.value.Clone(), rule.expiration, rule.session_model);
    }
  }
  rule_iterator.reset();

  // Migrate.
  DCHECK_EQ(old_rules.size(), new_rules.size());
  for (size_t i = 0; i < old_rules.size(); i++) {
    // Remove current setting.
    SetWebsiteSettingInternal(
        old_rules[i].first, old_rules[i].second, content_type,
        ContentSettingToValue(CONTENT_SETTING_DEFAULT), {});
    // Add new setting.
    SetWebsiteSettingInternal(
        new_rules[i].primary_pattern, new_rules[i].secondary_pattern,
        content_type,
        ContentSettingToValue(ValueToContentSetting(new_rules[i].value)),
        {new_rules[i].expiration, new_rules[i].session_model});
  }
}

void BravePrefProvider::MigrateFingerprintingSettings() {
  if (prefs_->GetBoolean(kBraveShieldsFPSettingsMigration) || off_the_record_)
    return;

  // Find rules that can be migrated and create replacement rules for them.
  std::vector<Rule> rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, false);
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    rules.emplace_back(CloneRule(rule));
  }
  rule_iterator.reset();

  // Migrate.
  for (const auto& fp_rule : rules) {
    if (fp_rule.secondary_pattern == ContentSettingsPattern::Wildcard() &&
        fp_rule.value == CONTENT_SETTING_BLOCK) {
#if BUILDFLAG(IS_ANDROID)
      SetWebsiteSettingInternal(fp_rule.primary_pattern,
                                fp_rule.secondary_pattern,
                                ContentSettingsType::BRAVE_FINGERPRINTING_V2,
                                ContentSettingToValue(CONTENT_SETTING_ASK),
                                {fp_rule.expiration, fp_rule.session_model});
#endif
    }
  }

  prefs_->SetBoolean(kBraveShieldsFPSettingsMigration, true);
}

void BravePrefProvider::MigrateFingerprintingSetingsToOriginScoped() {
  if (off_the_record_)
    return;

  // Find rules that can be migrated and create replacement rules for them.
  std::vector<Rule> rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, false);
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    rules.emplace_back(CloneRule(rule));
  }
  rule_iterator.reset();

  // Migrate.
  for (const auto& fp_rule : rules) {
    if (fp_rule.secondary_pattern ==
        ContentSettingsPattern::FromString("https://balanced/*")) {
      // delete the "balanced" override
      SetWebsiteSettingInternal(
          fp_rule.primary_pattern, fp_rule.secondary_pattern,
          ContentSettingsType::BRAVE_FINGERPRINTING_V2,
          ContentSettingToValue(CONTENT_SETTING_DEFAULT), {});
      // replace with ask
      SetWebsiteSettingInternal(fp_rule.primary_pattern,
                                ContentSettingsPattern::Wildcard(),
                                ContentSettingsType::BRAVE_FINGERPRINTING_V2,
                                ContentSettingToValue(CONTENT_SETTING_ASK), {});
    }
  }
}

bool BravePrefProvider::SetWebsiteSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    base::Value&& in_value,
    const ContentSettingConstraints& constraints) {
  const auto rule_matcher =
      [&primary_pattern = std::as_const(primary_pattern),
       &secondary_pattern = std::as_const(secondary_pattern),
       &in_value = std::as_const(in_value)](const Rule& rule) {
        return rule.primary_pattern == primary_pattern &&
               rule.secondary_pattern == secondary_pattern &&
               rule.value != in_value;
      };
  const auto cookie_is_found_in =
      [&rule_matcher](const std::vector<Rule>& rules) {
        return base::ranges::any_of(rules, rule_matcher);
      };

  if (content_type == ContentSettingsType::COOKIES) {
    if (cookie_is_found_in(brave_shield_down_rules_[off_the_record_])) {
      // Don't do anything with the generated Shiels-down rules. Unremovable
      // rule.
      return true;
    }
    if (cookie_is_found_in(brave_cookie_rules_[off_the_record_])) {
      // change to type ContentSettingsType::BRAVE_COOKIES
      return SetWebsiteSettingInternal(primary_pattern, secondary_pattern,
                                       ContentSettingsType::BRAVE_COOKIES,
                                       std::move(in_value), constraints);
    }
  }

  return SetWebsiteSettingInternal(primary_pattern, secondary_pattern,
                                   content_type, std::move(in_value),
                                   constraints);
}

bool BravePrefProvider::SetWebsiteSettingForTest(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    base::Value&& value,
    const ContentSettingConstraints& constraints) {
  return PrefProvider::SetWebsiteSetting(primary_pattern, secondary_pattern,
                                         content_type, std::move(value),
                                         constraints);
}

bool BravePrefProvider::SetWebsiteSettingInternal(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    base::Value&& in_value,
    const ContentSettingConstraints& constraints) {
  // PrefProvider ignores default settings so handle them here for shields
  if (content_settings::IsShieldsContentSettingsType(content_type) &&
      primary_pattern == ContentSettingsPattern::Wildcard() &&
      secondary_pattern == ContentSettingsPattern::Wildcard()) {
    if (content_type == ContentSettingsType::BRAVE_COOKIES) {
      // Default value for BRAVE_COOKIES handled in chromium code. This value
      // based on default COOKIES value (which provided by DefaultPrefProvider)
      // and kCookieControlsMode pref (default value in
      // brave::SetDefaultThirdPartyCookieBlockValue).
      return false;
    }
    base::Time modified_time =
        store_last_modified_ ? base::Time::Now() : base::Time();

    GetPref(content_type)
        ->SetWebsiteSetting(primary_pattern, secondary_pattern, modified_time,
                            std::move(in_value), constraints);
    return true;
  }

  if (content_type == ContentSettingsType::BRAVE_FINGERPRINTING_V2 &&
      content_settings::ValueToContentSetting(in_value) !=
          CONTENT_SETTING_DEFAULT &&
      secondary_pattern ==
          ContentSettingsPattern::FromString("https://balanced/*"))
    return false;

  return PrefProvider::SetWebsiteSetting(primary_pattern, secondary_pattern,
                                         content_type, std::move(in_value),
                                         constraints);
}

std::unique_ptr<RuleIterator> BravePrefProvider::GetRuleIterator(
    ContentSettingsType content_type,
    bool incognito) const NO_THREAD_SAFETY_ANALYSIS {
  if (content_type == ContentSettingsType::COOKIES) {
    const auto& rules = cookie_rules_.at(incognito);
    return rules.GetRuleIterator(content_type, &lock_);
  }

  return PrefProvider::GetRuleIterator(content_type, incognito);
}

void BravePrefProvider::UpdateCookieRules(ContentSettingsType content_type,
                                          bool incognito) {
  std::vector<Rule> rules;
  auto old_rules = std::move(brave_cookie_rules_[incognito]);
  auto old_shields_down_rules = std::move(brave_shield_down_rules_[incognito]);

  brave_cookie_rules_[incognito].clear();

  // kGoogleLoginControlType preference adds an exception for
  // accounts.google.com to access cookies in 3p context to allow login using
  // google oauth. The exception is added before all overrides to allow google
  // oauth to work when the user sets custom overrides for a site.
  // For example: Google OAuth will be allowed if the user allows all cookies
  // and sets 3p cookie blocking for a site.
  //
  // We also create the same exception for firebase apps, since they
  // are tightly bound to google, and require google auth to work.
  // See: #5075, #9852, #10367
  //
  // PS: kGoogleLoginControlType preference might not be registered for tests.
  if (prefs_->FindPreference(kGoogleLoginControlType) &&
      prefs_->GetBoolean(kGoogleLoginControlType)) {
    const auto google_auth_rule =
        Rule(ContentSettingsPattern::FromString(kGoogleAuthPattern),
             ContentSettingsPattern::Wildcard(),
             ContentSettingToValue(CONTENT_SETTING_ALLOW), base::Time(),
             SessionModel::Durable);
    rules.emplace_back(CloneRule(google_auth_rule));
    brave_cookie_rules_[incognito].emplace_back(CloneRule(google_auth_rule));

    const auto firebase_rule =
        Rule(ContentSettingsPattern::FromString(kFirebasePattern),
             ContentSettingsPattern::Wildcard(),
             ContentSettingToValue(CONTENT_SETTING_ALLOW), base::Time(),
             SessionModel::Durable);
    rules.emplace_back(CloneRule(firebase_rule));
    brave_cookie_rules_[incognito].emplace_back(CloneRule(firebase_rule));
  }
  // non-pref based exceptions should go in the cookie_settings_base.cc
  // chromium_src override

  // add chromium cookies
  {
    auto chromium_cookies_iterator =
        PrefProvider::GetRuleIterator(ContentSettingsType::COOKIES, incognito);
    while (chromium_cookies_iterator && chromium_cookies_iterator->HasNext()) {
      rules.emplace_back(CloneRule(chromium_cookies_iterator->Next()));
    }
  }

  // collect shield rules
  std::vector<Rule> shield_rules;
  {
    auto brave_shields_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_SHIELDS, incognito);
    while (brave_shields_iterator && brave_shields_iterator->HasNext()) {
      shield_rules.emplace_back(CloneRule(brave_shields_iterator->Next()));
    }
  }

  // add brave cookies after checking shield status
  {
    auto brave_cookies_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_COOKIES, incognito);
    // Matching cookie rules against shield rules.
    while (brave_cookies_iterator && brave_cookies_iterator->HasNext()) {
      auto rule = brave_cookies_iterator->Next();
      if (IsActive(rule, shield_rules)) {
        rules.emplace_back(CloneRule(rule));
        brave_cookie_rules_[incognito].emplace_back(CloneRule(rule));
      }
    }
  }

  // Adding shields down rules (they always override cookie rules).
  for (const auto& shield_rule : shield_rules) {
    // There is no global shields rule, so if we have one ignore it. It would
    // get replaced with EnsureNoWildcardEntries().
    if (shield_rule.primary_pattern.MatchesAllHosts()) {
      LOG(ERROR) << "Found a wildcard shields rule which matches all hosts.";
      continue;
    }

    // Shields down.
    if (ValueToContentSetting(shield_rule.value) == CONTENT_SETTING_BLOCK) {
      rules.emplace_back(ContentSettingsPattern::Wildcard(),
                         shield_rule.primary_pattern,
                         ContentSettingToValue(CONTENT_SETTING_ALLOW),
                         base::Time(), SessionModel::Durable);
      brave_shield_down_rules_[incognito].emplace_back(
          Rule(ContentSettingsPattern::Wildcard(), shield_rule.primary_pattern,
               ContentSettingToValue(CONTENT_SETTING_ALLOW), base::Time(),
               SessionModel::Durable));
      brave_cookie_rules_[incognito].emplace_back(
          Rule(ContentSettingsPattern::Wildcard(), shield_rule.primary_pattern,
               ContentSettingToValue(CONTENT_SETTING_ALLOW), base::Time(),
               SessionModel::Durable));
    }
  }

  // get the list of changes
  std::vector<Rule> brave_cookie_updates;
  for (const auto& new_rule : brave_cookie_rules_[incognito]) {
    auto match =
        base::ranges::find_if(old_rules, [&new_rule](const auto& old_rule) {
          // we want an exact match here because any change to the rule
          // is an update
          return new_rule.primary_pattern == old_rule.primary_pattern &&
                 new_rule.secondary_pattern == old_rule.secondary_pattern &&
                 ValueToContentSetting(new_rule.value) ==
                     ValueToContentSetting(old_rule.value);
        });
    if (match == old_rules.end()) {
      brave_cookie_updates.emplace_back(CloneRule(new_rule));
    }
  }

  // find any removed rules
  for (const auto& old_rule : old_rules) {
    auto match = base::ranges::find_if(
        brave_cookie_rules_[incognito], [&old_rule](const auto& new_rule) {
          // we only care about the patterns here because we're looking
          // for deleted rules, not changed rules
          return new_rule.primary_pattern == old_rule.primary_pattern &&
                 new_rule.secondary_pattern == old_rule.secondary_pattern;
        });
    if (match == brave_cookie_rules_[incognito].end()) {
      brave_cookie_updates.emplace_back(
          old_rule.primary_pattern, old_rule.secondary_pattern, base::Value(),
          old_rule.expiration, old_rule.session_model);
    }
  }
  {
    base::AutoLock auto_lock(lock_);
    cookie_rules_[incognito].clear();
    for (auto&& r : rules) {
      cookie_rules_[incognito].SetValue(
          r.primary_pattern, r.secondary_pattern, ContentSettingsType::COOKIES,
          store_last_modified_ ? base::Time::Now() : base::Time(),
          std::move(r.value), {r.expiration, r.session_model});
    }
  }

  // Notify brave cookie changes as ContentSettingsType::COOKIES
  if (initialized_ && (content_type == ContentSettingsType::BRAVE_COOKIES ||
                       content_type == ContentSettingsType::BRAVE_SHIELDS)) {
    NotifyChanges(brave_cookie_updates, incognito);
  }
}

void BravePrefProvider::NotifyChanges(const std::vector<Rule>& rules,
                                      bool incognito) {
  for (const auto& rule : rules) {
    Notify(rule.primary_pattern, rule.secondary_pattern,
           ContentSettingsType::COOKIES);
  }
}

void BravePrefProvider::OnCookiePrefsChanged(const std::string& pref) {
  OnCookieSettingsChanged(ContentSettingsType::BRAVE_COOKIES);
}

void BravePrefProvider::OnCookieSettingsChanged(
    ContentSettingsType content_type) {
  UpdateCookieRules(content_type, true);
  UpdateCookieRules(content_type, false);
}

void BravePrefProvider::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  if (content_type == ContentSettingsType::COOKIES ||
      content_type == ContentSettingsType::BRAVE_COOKIES ||
      content_type == ContentSettingsType::BRAVE_SHIELDS) {
    OnCookieSettingsChanged(content_type);
  }
}

}  // namespace content_settings
