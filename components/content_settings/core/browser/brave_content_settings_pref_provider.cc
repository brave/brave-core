/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "brave/components/google_sign_in_permission/google_sign_in_permission_util.h"
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
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace content_settings {

namespace {

constexpr char kObsoleteShieldCookies[] =
    "profile.content_settings.exceptions.shieldsCookies";
constexpr char kBraveShieldsFPSettingsMigration[] =
    "brave.shields_fp_settings_migration";

constexpr char kExpirationPath[] = "expiration";
constexpr char kLastModifiedPath[] = "last_modified";
constexpr char kSessionModelPath[] = "model";
constexpr char kSettingPath[] = "setting";
constexpr char kPerResourcePath[] = "per_resource";

std::unique_ptr<Rule> CloneRule(const Rule* original_rule) {
  DCHECK(original_rule);
  return std::make_unique<Rule>(
      original_rule->primary_pattern, original_rule->secondary_pattern,
      original_rule->value.Clone(), original_rule->metadata);
}

bool IsActive(const Rule* cookie_rule,
              const std::vector<std::unique_ptr<Rule>>& shield_rules) {
  DCHECK(cookie_rule);
  // don't include default rules in the iterator
  if (cookie_rule->primary_pattern == ContentSettingsPattern::Wildcard() &&
      cookie_rule->secondary_pattern == ContentSettingsPattern::Wildcard()) {
    return false;
  }

  for (const auto& shield_rule : shield_rules) {
    auto primary_compare =
        shield_rule->primary_pattern.Compare(cookie_rule->secondary_pattern);
    if (primary_compare == ContentSettingsPattern::IDENTITY ||
        primary_compare == ContentSettingsPattern::SUCCESSOR) {
      return ValueToContentSetting(shield_rule->value) != CONTENT_SETTING_BLOCK;
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

  const auto& plugins =
      prefs->GetDict("profile.content_settings.exceptions.plugins");
  prefs->SetDict("brave.migrate.content_settings.exceptions.plugins",
                 plugins.Clone());

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

  // This path is no longer registered upstream but we still need it to migrate
  // Shields settings away from ResourceIdentifier.
  if (!registry->defaults()->GetValue(
          "profile.content_settings.exceptions.plugins", nullptr)) {
    registry->RegisterDictionaryPref(
        "profile.content_settings.exceptions.plugins");
  }

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
  const auto& shields_cookies = prefs_->GetDict(kObsoleteShieldCookies);
  if (version < 4) {
    prefs_->SetDict("profile.content_settings.exceptions.shieldsCookiesV3",
                    shields_cookies.Clone());
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
              base::SequencedTaskRunner::GetCurrentDefault(),
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
  // defaults, e.g. `profile.default_content_setting_values.images`.
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
    content_settings::mojom::SessionModel session_model =
        GetSessionModelFromDictionary(*settings_dict, kSessionModelPath);

    const base::Value::Dict* resource_dict =
        settings_dict->FindDictByDottedPath(kPerResourcePath);
    if (resource_dict) {
      base::Time last_modified =
          base::ValueToTime(settings_dict->Find(kLastModifiedPath))
              .value_or(base::Time());
      for (const auto&& [resource_key, resource_value] : *resource_dict) {
        const std::string& resource_identifier(resource_key);
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

        DCHECK(resource_value.is_int());
        int setting = resource_value.GetInt();
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
    content_settings::mojom::SessionModel session_model,
    int setting) {
  // Non-supported preference paths should have been filtered out already.
  CHECK(prefs_->HasPrefPath(preference_path))
      << "Attempted to migrate unsupported shields setting.";

  ScopedDictPrefUpdate update(prefs_, preference_path);

  base::Value::Dict* shield_settings = update->EnsureDict(patterns_string);
  DCHECK(shield_settings);

  shield_settings->Set(
      kExpirationPath,
      base::NumberToString(
          expiration.ToDeltaSinceWindowsEpoch().InMicroseconds()));
  shield_settings->Set(
      kLastModifiedPath,
      base::NumberToString(
          last_modified.ToDeltaSinceWindowsEpoch().InMicroseconds()));
  shield_settings->Set(kSessionModelPath, static_cast<int>(session_model));
  shield_settings->Set(kSettingPath, setting);
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

  auto rule_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_COOKIES,
      /*off_the_record*/ false,
      content_settings::PartitionKey::WipGetDefault());

  using OldRule = std::pair<ContentSettingsPattern, ContentSettingsPattern>;
  std::vector<OldRule> old_rules;
  std::vector<std::unique_ptr<Rule>> new_rules;

  // Find rules that can be migrated and create replacement rules for them.
  while (rule_iterator && rule_iterator->HasNext()) {
    auto old_rule = rule_iterator->Next();
    old_rules.emplace_back(old_rule->primary_pattern,
                           old_rule->secondary_pattern);
    if (old_rule->primary_pattern == wildcard &&
        (old_rule->secondary_pattern == wildcard ||
         old_rule->secondary_pattern == first_party)) {
      // Remove default rules from BRAVE_COKIES because it's already mapped to
      // the Chromium prefs.
      continue;
    }
    if (old_rule->secondary_pattern == wildcard && !new_rules.empty() &&
        new_rules.back()->secondary_pattern == old_rule->primary_pattern &&
        old_rule->value == new_rules.back()->value) {
      // Remove the "first-party" rule because it is a predecessor of a general
      // rule that we are going to add.
      new_rules.pop_back();
    }

    std::unique_ptr<Rule> new_rule = std::make_unique<Rule>(
        // Exchange primary and secondary patterns.
        old_rule->secondary_pattern, old_rule->primary_pattern,
        std::move(old_rule->value), old_rule->metadata);
    // Replace first party placeholder with actual pattern
    if (new_rule->primary_pattern == first_party) {
      new_rule->primary_pattern =
          content_settings::CreateDomainPattern(GURL(base::StrCat(
              {"https://" + new_rule->secondary_pattern.GetHost() + "/"})));
    }
    new_rules.push_back(std::move(new_rule));
  }
  rule_iterator.reset();

  ClearAllContentSettingsRules(ContentSettingsType::BRAVE_COOKIES,
                               content_settings::PartitionKey::WipGetDefault());
  for (auto&& rule : new_rules) {
    ContentSettingConstraints constraints;
    constraints.set_session_model(rule->metadata.session_model());
    SetWebsiteSettingInternal(rule->primary_pattern, rule->secondary_pattern,
                              ContentSettingsType::BRAVE_COOKIES,
                              std::move(rule->value), std::move(constraints));
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
  std::vector<std::unique_ptr<Rule>> new_rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(
      content_type,
      /*off_the_record*/ false,
      content_settings::PartitionKey::WipGetDefault());
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    auto new_primary_pattern =
        ConvertPatternToWildcardSchemeAndPort(rule->primary_pattern);
    auto new_secondary_pattern =
        ConvertPatternToWildcardSchemeAndPort(rule->secondary_pattern);
    if (new_primary_pattern || new_secondary_pattern) {
      old_rules.emplace_back(rule->primary_pattern, rule->secondary_pattern);
      new_rules.emplace_back(std::make_unique<Rule>(
          new_primary_pattern.value_or(rule->primary_pattern),
          new_secondary_pattern.value_or(rule->secondary_pattern),
          rule->value.Clone(), rule->metadata));
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
    ContentSettingConstraints constraints;
    constraints.set_session_model(new_rules[i]->metadata.session_model());

    SetWebsiteSettingInternal(
        new_rules[i]->primary_pattern, new_rules[i]->secondary_pattern,
        content_type,
        ContentSettingToValue(ValueToContentSetting(new_rules[i]->value)),
        std::move(constraints));
  }
}

void BravePrefProvider::MigrateFingerprintingSettings() {
  if (prefs_->GetBoolean(kBraveShieldsFPSettingsMigration) || off_the_record_)
    return;

  // Find rules that can be migrated and create replacement rules for them.
  std::vector<std::unique_ptr<Rule>> rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, false,
      content_settings::PartitionKey::WipGetDefault());
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    rules.emplace_back(CloneRule(rule.get()));
  }
  rule_iterator.reset();

  // Migrate.
  for (const auto& fp_rule : rules) {
    if (fp_rule->secondary_pattern == ContentSettingsPattern::Wildcard() &&
        fp_rule->value == CONTENT_SETTING_BLOCK) {
#if BUILDFLAG(IS_ANDROID)
      ContentSettingConstraints constraints;
      constraints.set_session_model(fp_rule->metadata.session_model());
      SetWebsiteSettingInternal(
          fp_rule->primary_pattern, fp_rule->secondary_pattern,
          ContentSettingsType::BRAVE_FINGERPRINTING_V2,
          ContentSettingToValue(CONTENT_SETTING_ASK), std::move(constraints));
#endif  // BUILDFLAG(IS_ANDROID)
    }
  }

  prefs_->SetBoolean(kBraveShieldsFPSettingsMigration, true);
}

void BravePrefProvider::MigrateFingerprintingSetingsToOriginScoped() {
  if (off_the_record_)
    return;

  // Find rules that can be migrated and create replacement rules for them.
  std::vector<std::unique_ptr<Rule>> rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, false,
      content_settings::PartitionKey::WipGetDefault());
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    rules.emplace_back(CloneRule(rule.get()));
  }
  rule_iterator.reset();

  // Migrate.
  for (const auto& fp_rule : rules) {
    if (fp_rule->secondary_pattern ==
        ContentSettingsPattern::FromString("https://balanced/*")) {
      // delete the "balanced" override
      SetWebsiteSettingInternal(
          fp_rule->primary_pattern, fp_rule->secondary_pattern,
          ContentSettingsType::BRAVE_FINGERPRINTING_V2,
          ContentSettingToValue(CONTENT_SETTING_DEFAULT), {});
      // replace with ask
      SetWebsiteSettingInternal(fp_rule->primary_pattern,
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
    const ContentSettingConstraints& constraints,
    const PartitionKey& partition_key) {
  const auto cookie_is_found_in =
      [&primary_pattern = std::as_const(primary_pattern),
       &secondary_pattern = std::as_const(secondary_pattern),
       &in_value = std::as_const(in_value)](
          const std::vector<std::unique_ptr<Rule>>& rules) {
        for (const auto& rule : rules) {
          if (rule->primary_pattern == primary_pattern &&
              rule->secondary_pattern == secondary_pattern &&
              rule->value != in_value) {
            return true;
          }
        }
        return false;
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
  return PrefProvider::SetWebsiteSetting(
      primary_pattern, secondary_pattern, content_type, std::move(value),
      constraints, content_settings::PartitionKey::WipGetDefault());
}

bool BravePrefProvider::SetWebsiteSettingInternal(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    base::Value&& in_value,
    const ContentSettingConstraints& constraints,
    const PartitionKey& partition_key) {
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

    base::Time last_visited = constraints.track_last_visit_for_autoexpiration()
                                  ? GetCoarseVisitedTime(base::Time::Now())
                                  : base::Time();

    RuleMetaData metadata;
    metadata.set_last_modified(modified_time);
    metadata.set_last_visited(last_visited);
    metadata.SetExpirationAndLifetime(constraints.expiration(),
                                      base::TimeDelta());
    metadata.set_session_model(constraints.session_model());

    GetPref(content_type)
        ->SetWebsiteSetting(primary_pattern, secondary_pattern,
                            std::move(in_value), metadata, partition_key);
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
                                         constraints, partition_key);
}

std::unique_ptr<RuleIterator> BravePrefProvider::GetRuleIterator(
    ContentSettingsType content_type,
    bool incognito,
    const PartitionKey& partition_key) const {
  if (content_type == ContentSettingsType::COOKIES) {
    const auto& rules = cookie_rules_.at(incognito);
    return rules.GetRuleIterator(content_type);
  }

  return PrefProvider::GetRuleIterator(content_type, incognito, partition_key);
}

std::unique_ptr<Rule> BravePrefProvider::GetRule(
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    bool off_the_record,
    const PartitionKey& partition_key) const {
  if (content_type == ContentSettingsType::COOKIES) {
    const auto& rules = cookie_rules_.at(off_the_record);
    base::AutoLock auto_lock(rules.GetLock());
    return rules.GetRule(primary_url, secondary_url, content_type);
  }

  return PrefProvider::GetRule(primary_url, secondary_url, content_type,
                               off_the_record, partition_key);
}

void BravePrefProvider::UpdateCookieRules(ContentSettingsType content_type,
                                          bool incognito) {
  std::vector<std::unique_ptr<Rule>> rules;
  auto old_rules = std::move(brave_cookie_rules_[incognito]);
  auto old_shields_down_rules = std::move(brave_shield_down_rules_[incognito]);
  brave_cookie_rules_[incognito].clear();

  const bool google_sign_in_flag_enabled =
      google_sign_in_permission::IsGoogleSignInFeatureEnabled();

  // If Google Sign-In permission feature flag is disabled,
  // we add 3p cookie exception globally for Google/Firebase auth domains.
  // TODO(ssahib): Remove this once we no longer need to support the flag.
  if (!google_sign_in_flag_enabled &&
      prefs_->FindPreference(kGoogleLoginControlType) &&
      prefs_->GetBoolean(kGoogleLoginControlType)) {
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
    RuleMetaData metadata;
    metadata.SetExpirationAndLifetime(base::Time(), base::TimeDelta());
    metadata.set_session_model(content_settings::mojom::SessionModel::DURABLE);
    const auto google_auth_rule = std::make_unique<Rule>(
        google_sign_in_permission::GetGoogleAuthPattern(),
        ContentSettingsPattern::Wildcard(),
        ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata);
    rules.emplace_back(CloneRule(google_auth_rule.get()));
    brave_cookie_rules_[incognito].emplace_back(
        CloneRule(google_auth_rule.get()));

    const auto firebase_rule = std::make_unique<Rule>(
        google_sign_in_permission::GetFirebaseAuthPattern(),
        ContentSettingsPattern::Wildcard(),
        ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata);
    rules.emplace_back(CloneRule(firebase_rule.get()));
    brave_cookie_rules_[incognito].emplace_back(CloneRule(firebase_rule.get()));
  } else if (google_sign_in_flag_enabled) {
    // Google Sign-In feature:
    // Add per-site cookie exception for Google/Firebase auth domains.
    // Get all sites that have BRAVE_GOOGLE_SIGN_IN turned on, and add exception
    // for them
    auto google_sign_in_content_setting_it = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_GOOGLE_SIGN_IN, incognito,
        content_settings::PartitionKey::WipGetDefault());
    while (google_sign_in_content_setting_it &&
           google_sign_in_content_setting_it->HasNext()) {
      const auto google_sign_in_rule =
          google_sign_in_content_setting_it->Next();

      RuleMetaData metadata;
      metadata.SetExpirationAndLifetime(
          google_sign_in_rule->metadata.expiration(), base::TimeDelta());
      metadata.set_session_model(google_sign_in_rule->metadata.session_model());
      // The embedding pattern for the cookie rule will be the primary pattern
      // for the BRAVE_GOOGLE_SIGN_IN permission.
      // We want to get all subdomains for the cookie rule...
      auto embedding_pattern = ContentSettingsPattern::ToDomainWildcardPattern(
          google_sign_in_rule->primary_pattern);
      // ... but if that doesn't work, fallback to stored pattern.
      if (!embedding_pattern.IsValid()) {
        embedding_pattern = google_sign_in_rule->primary_pattern;
      }
      const auto google_auth_rule = std::make_unique<Rule>(
          google_sign_in_permission::GetGoogleAuthPattern(), embedding_pattern,
          google_sign_in_rule->value.Clone(), metadata);
      rules.emplace_back(CloneRule(google_auth_rule.get()));
      brave_cookie_rules_[incognito].emplace_back(
          CloneRule(google_auth_rule.get()));

      const auto firebase_rule = std::make_unique<Rule>(
          google_sign_in_permission::GetFirebaseAuthPattern(),
          embedding_pattern, google_sign_in_rule->value.Clone(), metadata);
      rules.emplace_back(CloneRule(firebase_rule.get()));
      brave_cookie_rules_[incognito].emplace_back(
          CloneRule(firebase_rule.get()));
    }
  }

  // Non-pref based exceptions should go in the cookie_settings_base.cc
  // chromium_src override.

  // Add chromium cookies.
  {
    auto chromium_cookies_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::COOKIES, incognito,
        content_settings::PartitionKey::WipGetDefault());
    while (chromium_cookies_iterator && chromium_cookies_iterator->HasNext()) {
      rules.emplace_back(CloneRule(chromium_cookies_iterator->Next().get()));
    }
  }

  // Collect shield rules.
  std::vector<std::unique_ptr<Rule>> shield_rules;
  {
    auto brave_shields_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_SHIELDS, incognito,
        content_settings::PartitionKey::WipGetDefault());
    while (brave_shields_iterator && brave_shields_iterator->HasNext()) {
      shield_rules.emplace_back(
          CloneRule(brave_shields_iterator->Next().get()));
    }
  }

  // Add brave cookies after checking shield status.
  {
    auto brave_cookies_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_COOKIES, incognito,
        content_settings::PartitionKey::WipGetDefault());
    // Matching cookie rules against shield rules.
    while (brave_cookies_iterator && brave_cookies_iterator->HasNext()) {
      auto rule = brave_cookies_iterator->Next();
      if (IsActive(rule.get(), shield_rules)) {
        rules.emplace_back(CloneRule(rule.get()));
        brave_cookie_rules_[incognito].emplace_back(CloneRule(rule.get()));
      }
    }
  }

  // Adding shields down rules (they always override cookie rules).
  for (const auto& shield_rule : shield_rules) {
    // There is no global shields rule, so if we have one ignore it. It would
    // get replaced with EnsureNoWildcardEntries().
    if (shield_rule->primary_pattern.MatchesAllHosts()) {
      LOG(ERROR) << "Found a wildcard shields rule which matches all hosts.";
      continue;
    }

    // Shields down.
    if (ValueToContentSetting(shield_rule->value) == CONTENT_SETTING_BLOCK) {
      RuleMetaData metadata;
      metadata.SetExpirationAndLifetime(base::Time(), base::TimeDelta());
      metadata.set_session_model(
          content_settings::mojom::SessionModel::DURABLE);

      rules.emplace_back(std::make_unique<Rule>(
          ContentSettingsPattern::Wildcard(), shield_rule->primary_pattern,
          ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata));
      brave_shield_down_rules_[incognito].emplace_back(std::make_unique<Rule>(
          ContentSettingsPattern::Wildcard(), shield_rule->primary_pattern,
          ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata));
      brave_cookie_rules_[incognito].emplace_back(std::make_unique<Rule>(
          ContentSettingsPattern::Wildcard(), shield_rule->primary_pattern,
          ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata));
    }
  }

  // Get the list of changes.
  std::vector<std::unique_ptr<Rule>> brave_cookie_updates;
  for (const auto& new_rule : brave_cookie_rules_[incognito]) {
    auto match =
        base::ranges::find_if(old_rules, [&new_rule](const auto& old_rule) {
          // we want an exact match here because any change to the rule
          // is an update
          return new_rule->primary_pattern == old_rule->primary_pattern &&
                 new_rule->secondary_pattern == old_rule->secondary_pattern &&
                 ValueToContentSetting(new_rule->value) ==
                     ValueToContentSetting(old_rule->value);
        });
    if (match == old_rules.end()) {
      brave_cookie_updates.emplace_back(CloneRule(new_rule.get()));
    }
  }

  // Find any removed rules.
  for (const auto& old_rule : old_rules) {
    auto match = base::ranges::find_if(
        brave_cookie_rules_[incognito], [&old_rule](const auto& new_rule) {
          // We only care about the patterns here because we're looking for
          // deleted rules, not changed rules.
          return new_rule->primary_pattern == old_rule->primary_pattern &&
                 new_rule->secondary_pattern == old_rule->secondary_pattern;
        });
    if (match == brave_cookie_rules_[incognito].end()) {
      brave_cookie_updates.emplace_back(std::make_unique<Rule>(
          old_rule->primary_pattern, old_rule->secondary_pattern, base::Value(),
          old_rule->metadata));
    }
  }
  {
    base::AutoLock lock(cookie_rules_[incognito].GetLock());
    cookie_rules_[incognito].clear();
    for (auto&& r : rules) {
      cookie_rules_[incognito].SetValue(
          r->primary_pattern, r->secondary_pattern,
          ContentSettingsType::COOKIES, std::move(r->value), r->metadata);
    }
  }

  // Notify brave cookie changes as ContentSettingsType::COOKIES.
  if (initialized_ &&
      (content_type == ContentSettingsType::BRAVE_COOKIES ||
       content_type == ContentSettingsType::BRAVE_SHIELDS ||
       content_type == ContentSettingsType::BRAVE_GOOGLE_SIGN_IN)) {
    NotifyChanges(brave_cookie_updates, incognito);
  }
}

void BravePrefProvider::NotifyChanges(
    const std::vector<std::unique_ptr<Rule>>& rules,
    bool incognito) {
  for (const auto& rule : rules) {
    Notify(rule->primary_pattern, rule->secondary_pattern,
           ContentSettingsType::COOKIES, /*partition_key=*/nullptr);
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
      content_type == ContentSettingsType::BRAVE_SHIELDS ||
      content_type == ContentSettingsType::BRAVE_GOOGLE_SIGN_IN) {
    OnCookieSettingsChanged(content_type);
  }
}

}  // namespace content_settings
