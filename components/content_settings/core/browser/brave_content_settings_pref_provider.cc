/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/function_ref.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom-shared.h"
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
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace content_settings {

namespace {

constexpr char kObsoleteShieldCookies[] =
    "profile.content_settings.exceptions.shieldsCookies";
constexpr char kBraveShieldsFPSettingsMigration[] =
    "brave.shields_fp_settings_migration";
constexpr char kCosmeticFilteringMigration[] =
    "brave.cosmetic_filtering_migration";

constexpr char kUnusedSitePermissions[] = "unused_site_permissions";

constexpr char kObsoleteBraveLocalhostPermission[] = "brave_localhost_access";

constexpr char kExpirationPath[] = "expiration";
constexpr char kLastModifiedPath[] = "last_modified";
constexpr char kSessionModelPath[] = "model";
constexpr char kSettingPath[] = "setting";
constexpr char kPerResourcePath[] = "per_resource";

std::unique_ptr<Rule> CloneRule(const Rule& original_rule) {
  return std::make_unique<Rule>(
      original_rule.primary_pattern, original_rule.secondary_pattern,
      original_rule.value.Clone(), original_rule.metadata.Clone());
}

brave_shields::mojom::AutoShredMode Remember1PStorageValueToAutoShredMode(
    const base::Value& value) {
  return ValueToContentSetting(value) == CONTENT_SETTING_BLOCK
             ? brave_shields::mojom::AutoShredMode::LAST_TAB_CLOSED
             : brave_shields::mojom::AutoShredMode::NEVER;
}

// Returns whether |rule| should contribute to the effective setting given the
// Shields state in |shield_rules|. A Brave rule only applies while Shields are
// up for its top-level site; |top_level_pattern| is the pattern that identifies
// that site (the secondary pattern for cookies, the primary pattern for
// JavaScript). Default (wildcard/wildcard) rules are ignored because they are
// already mapped to the Chromium setting.
bool IsActive(const Rule* rule,
              const ContentSettingsPattern& top_level_pattern,
              const std::vector<std::unique_ptr<Rule>>& shield_rules) {
  DCHECK(rule);
  // don't include default rules in the iterator
  if (rule->primary_pattern == ContentSettingsPattern::Wildcard() &&
      rule->secondary_pattern == ContentSettingsPattern::Wildcard()) {
    return false;
  }

  for (const auto& shield_rule : shield_rules) {
    auto primary_compare =
        shield_rule->primary_pattern.Compare(top_level_pattern);
    if (primary_compare == ContentSettingsPattern::IDENTITY ||
        primary_compare == ContentSettingsPattern::SUCCESSOR) {
      return ValueToContentSetting(shield_rule->value) != CONTENT_SETTING_BLOCK;
    }
  }

  return true;
}

// Returns whether |rule| is a JAVASCRIPT rule that Shields authored, as opposed
// to one the user created through the Chromium Site Settings UI. Shields wrote
// its per-site JS rules with a host pattern of the form "*://host/*" (produced
// by content_settings::CreateHostPattern()) paired with a wildcard secondary
// pattern; reconstructing that pattern from the rule's host and comparing lets
// us select only Shields-origin rules during migration.
bool IsShieldsAuthoredJavascriptRule(const Rule& rule) {
  if (rule.secondary_pattern != ContentSettingsPattern::Wildcard()) {
    return false;
  }
  const std::string host = rule.primary_pattern.GetHost();
  if (host.empty()) {
    return false;
  }
  return rule.primary_pattern == content_settings::CreateHostPattern(GURL(
                                     base::StrCat({"https://", host, "/"})));
}

// Computes the set of change notifications needed to move an effective rule set
// from |old_rules| to |new_rules|. A rule is reported as an update when it is
// added or its value changed (exact pattern + value match), and a removed rule
// is reported with an empty value so observers clear it. Shared by the cookie
// and JavaScript effective-rule builders, which differ only in the underlying
// content type.
std::vector<std::unique_ptr<Rule>> ComputeRuleUpdates(
    const std::vector<std::unique_ptr<Rule>>& old_rules,
    const std::vector<std::unique_ptr<Rule>>& new_rules) {
  std::vector<std::unique_ptr<Rule>> updates;

  // Added or changed rules. An exact value match is required because any change
  // to the value is an update.
  for (const auto& new_rule : new_rules) {
    auto match =
        std::ranges::find_if(old_rules, [&new_rule](const auto& old_rule) {
          return new_rule->primary_pattern == old_rule->primary_pattern &&
                 new_rule->secondary_pattern == old_rule->secondary_pattern &&
                 ValueToContentSetting(new_rule->value) ==
                     ValueToContentSetting(old_rule->value);
        });
    if (match == old_rules.end()) {
      updates.emplace_back(CloneRule(CHECK_DEREF(new_rule.get())));
    }
  }

  // Removed rules. Only the patterns matter here because we are looking for
  // deleted rules, not changed ones.
  for (const auto& old_rule : old_rules) {
    auto match =
        std::ranges::find_if(new_rules, [&old_rule](const auto& new_rule) {
          return new_rule->primary_pattern == old_rule->primary_pattern &&
                 new_rule->secondary_pattern == old_rule->secondary_pattern;
        });
    if (match == new_rules.end()) {
      updates.emplace_back(std::make_unique<Rule>(
          old_rule->primary_pattern, old_rule->secondary_pattern, base::Value(),
          old_rule->metadata.Clone()));
    }
  }

  return updates;
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
  DiscardObsoletePreferences();

  pref_change_registrar_.Init(prefs);

  pref_change_registrar_.Add(
      kGoogleLoginControlType,
      base::BindRepeating(&BravePrefProvider::OnCookiePrefsChanged,
                          base::Unretained(this)));

  MigrateShieldsSettings(off_the_record_);
  MigrateFingerprintingSetingsToOriginScoped();
  MigrateCosmeticFilteringSettings();
  MigrateBraveRemember1PStorageToAutoShred();

  OnCookieSettingsChanged(ContentSettingsType::BRAVE_COOKIES);
  OnJavascriptSettingsChanged(ContentSettingsType::BRAVE_JAVASCRIPT);

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

  registry->RegisterDictionaryPref(GetShieldsSettingUserPrefsPath(
      brave_shields::kObsoleteCosmeticFiltering));
  registry->RegisterBooleanPref(kCosmeticFilteringMigration, false);
  registry->RegisterBooleanPref(kBraveRemember1PStorageMigration, false);
  registry->RegisterDictionaryPref(
      GetShieldsSettingUserPrefsPath(kObsoleteBraveLocalhostPermission));
}

void BravePrefProvider::DiscardObsoletePreferences() {
  if (off_the_record_) {
    return;
  }

  prefs_->ClearPref(
      GetShieldsSettingUserPrefsPath(kObsoleteBraveLocalhostPermission));

  // https://github.com/brave/brave-browser/issues/52220
  // When a permission is deleted, a references to the non-existent permission
  // may remain in the UNUSED_SITE_PERMISSIONS content settings. It has the
  // following structure under the `profile.content_settings.exceptions` path:
  //     "unused_site_permissions": {
  //           "host, *": {
  //               "expiration": "time",
  //               "last_modified": "time",
  //               "lifetime": "time",
  //               "setting":
  //               {
  //                   "revoked": [ "content setting name", ... ]
  //                   "revoked-chooser-permissions": [ "content setting name",
  //                   ... ],
  //                   possible other data
  //               }
  //           },
  //           --- // ---
  //     }
  // The code below iterates through all lists within UNUSED_SITE_PERMISSIONS
  // and removes references to obsolete permissions.

  base::DictValue unused_site_permissions =
      prefs_->GetDict(GetShieldsSettingUserPrefsPath(kUnusedSitePermissions))
          .Clone();

  constexpr std::string_view kObsoletePermissions[] = {
      kObsoleteBraveLocalhostPermission};

  bool need_update = false;
  for (std::string_view obsolete_permission : kObsoletePermissions) {
    const base::Value obsolete_value(obsolete_permission);

    for (auto&& value : unused_site_permissions) {
      if (!value.second.is_dict()) {
        continue;
      }
      base::DictValue* setting = value.second.GetDict().FindDict(kSettingPath);
      if (!setting) {
        continue;
      }
      for (auto&& list : *setting) {
        if (!list.second.is_list()) {
          continue;
        }
        if (list.second.GetList().EraseValue(obsolete_value) > 0) {
          need_update = true;
        }
      }
    }
  }

  if (need_update) {
    prefs_->SetDict(GetShieldsSettingUserPrefsPath(kUnusedSitePermissions),
                    std::move(unused_site_permissions));
  }
}

void BravePrefProvider::MigrateShieldsSettings(bool incognito) {
  // Incognito inherits from regular profile, so nothing to do.
  // Guest doesn't inherit, but only keeps settings for the duration of the
  // session, so also nothing to do.
  if (incognito) {
    return;
  }

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
    if (!info) {
      continue;
    }

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

  MigrateShieldsSettingsV4ToV5();
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
    const base::DictValue* settings_dict = value.GetIfDict();
    DCHECK(settings_dict);

    base::Time expiration =
        base::ValueToTime(settings_dict->Find(kExpirationPath))
            .value_or(base::Time());
    content_settings::mojom::SessionModel session_model =
        GetSessionModelFromDictionary(*settings_dict, kSessionModelPath);

    const base::DictValue* resource_dict =
        settings_dict->FindDictByDottedPath(kPerResourcePath);
    if (resource_dict) {
      base::Time last_modified =
          base::ValueToTime(settings_dict->Find(kLastModifiedPath))
              .value_or(base::Time());
      for (const auto&& [resource_key, resource_value] : *resource_dict) {
        const std::string& resource_identifier(resource_key);
        std::string shields_preference_name;

        // These obsolete resource names need to adapt to Shields-owned names,
        // otherwise they would refer to upstream content settings.
        if (resource_identifier == brave_shields::kObsoleteAds) {
          shields_preference_name = brave_shields::kAds;
        } else if (resource_identifier == brave_shields::kObsoleteCookies) {
          shields_preference_name = brave_shields::kObsoleteShieldsCookies;
        } else if (resource_identifier ==
                   brave_shields::kObsoleteCosmeticFiltering) {
          shields_preference_name = resource_identifier;
          // Setup empty value to pass CHECK(HasPrefPath) in
          // MigrateShieldsSettingsFromResourceIdsForOneType.
          const auto pref_path =
              GetShieldsSettingUserPrefsPath(shields_preference_name);
          if (!prefs_->HasPrefPath(pref_path)) {
            prefs_->SetDict(pref_path, base::DictValue());
          }
        }
        if (shields_preference_name.empty()) {
          shields_preference_name = resource_identifier;
        }

        // Protect against non registered paths (unlikely, but possible).
        if (!IsShieldsContentSettingsTypeName(shields_preference_name) &&
            shields_preference_name !=
                brave_shields::kObsoleteCosmeticFiltering) {
          continue;
        }

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

  base::DictValue* shield_settings = update->EnsureDict(patterns_string);
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
  if (prefs_->GetInteger(kBraveShieldsSettingsVersion) != 1) {
    return;
  }

  // All sources in Brave-specific ContentSettingsType(s) we want to migrate.
  for (const auto& content_type : GetShieldsContentSettingsTypes()) {
    MigrateShieldsSettingsV1ToV2ForOneType(content_type);
  }

  // ContentSettingsType::JAVASCRIPT.
  MigrateShieldsSettingsV1ToV2ForOneType(ContentSettingsType::JAVASCRIPT);

  // Mark migration as done.
  prefs_->SetInteger(kBraveShieldsSettingsVersion, 2);
}

void BravePrefProvider::MigrateShieldsSettingsV2ToV3() {
  // Check if migration is needed.
  if (prefs_->GetInteger(kBraveShieldsSettingsVersion) != 2) {
    return;
  }

  const ContentSettingsPattern& wildcard = ContentSettingsPattern::Wildcard();
  const ContentSettingsPattern first_party(
      ContentSettingsPattern::FromString("https://firstParty/*"));

  auto rule_iterator =
      PrefProvider::GetRuleIterator(ContentSettingsType::BRAVE_COOKIES,
                                    /*off_the_record*/ false);

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
        std::move(old_rule->value), std::move(old_rule->metadata));
    // Replace first party placeholder with actual pattern
    if (new_rule->primary_pattern == first_party) {
      new_rule->primary_pattern =
          content_settings::CreateDomainPattern(GURL(base::StrCat(
              {"https://" + new_rule->secondary_pattern.GetHost() + "/"})));
    }
    new_rules.push_back(std::move(new_rule));
  }
  rule_iterator.reset();

  ClearAllContentSettingsRules(ContentSettingsType::BRAVE_COOKIES);
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
  if (prefs_->GetInteger(kBraveShieldsSettingsVersion) != 3) {
    return;
  }

  if (start_version == 3) {
    // Because of
    // https://github.com/brave/brave-browser/issues/24119 the cookies
    // overwritten by the obsolete cookies. Repeat v2 to v3 migration.
    prefs_->SetInteger(kBraveShieldsSettingsVersion, 2);
    MigrateShieldsSettingsV2ToV3();
  }
  prefs_->SetInteger(kBraveShieldsSettingsVersion, 4);
}

void BravePrefProvider::MigrateShieldsSettingsV4ToV5() {
  if (prefs_->GetInteger(kBraveShieldsSettingsVersion) != 4) {
    return;
  }

  // Only JAVASCRIPT rules that Shields itself created should move to
  // BRAVE_JAVASCRIPT; JAVASCRIPT exceptions the user added through the Chromium
  // Site Settings UI must stay put. Shields wrote its per-site JS rules through
  // SetNoScriptControlType(), which always uses a host pattern of the form
  // "*://host/*" (scheme-wildcard, specific host, wildcard path) produced by
  // content_settings::CreateHostPattern(), paired with a wildcard secondary
  // pattern. The Chromium UI instead writes scheme-specific, port-qualified
  // patterns (e.g. "https://host:443"), so matching this exact Shields-authored
  // shape reliably selects only Shields-origin rules.
  std::vector<std::unique_ptr<Rule>> rules_to_migrate;
  auto rule_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::JAVASCRIPT, /*off_the_record*/ false);
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    if (IsShieldsAuthoredJavascriptRule(*rule)) {
      rules_to_migrate.emplace_back(CloneRule(CHECK_DEREF(rule.get())));
    }
  }
  rule_iterator.reset();

  for (const auto& rule : rules_to_migrate) {
    SetWebsiteSettingInternal(rule->primary_pattern, rule->secondary_pattern,
                              ContentSettingsType::JAVASCRIPT,
                              ContentSettingToValue(CONTENT_SETTING_DEFAULT),
                              {});

    ContentSettingConstraints constraints;
    constraints.set_session_model(rule->metadata.session_model());
    SetWebsiteSettingInternal(
        rule->primary_pattern, rule->secondary_pattern,
        ContentSettingsType::BRAVE_JAVASCRIPT,
        ContentSettingToValue(ValueToContentSetting(rule->value)),
        std::move(constraints));
  }

  prefs_->SetInteger(kBraveShieldsSettingsVersion, 5);
}

void BravePrefProvider::MigrateShieldsSettingsV1ToV2ForOneType(
    ContentSettingsType content_type) {
  using OldRule = std::pair<ContentSettingsPattern, ContentSettingsPattern>;
  // Find rules that can be migrated and create replacement rules for them.
  std::vector<OldRule> old_rules;
  std::vector<std::unique_ptr<Rule>> new_rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(content_type,
                                                     /*off_the_record*/ false);
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
          rule->value.Clone(), rule->metadata.Clone()));
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
  if (prefs_->GetBoolean(kBraveShieldsFPSettingsMigration) || off_the_record_) {
    return;
  }

  // Find rules that can be migrated and create replacement rules for them.
  std::vector<std::unique_ptr<Rule>> rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, false);
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    rules.emplace_back(CloneRule(CHECK_DEREF(rule.get())));
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
  if (off_the_record_) {
    return;
  }

  // Find rules that can be migrated and create replacement rules for them.
  std::vector<std::unique_ptr<Rule>> rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, false);
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    rules.emplace_back(CloneRule(CHECK_DEREF(rule.get())));
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

void BravePrefProvider::MigrateBraveRemember1PStorageToAutoShred() {
  if (off_the_record_ || prefs_->GetBoolean(kBraveRemember1PStorageMigration) ||
      !base::FeatureList::IsEnabled(
          brave_shields::features::kBraveShredFeature)) {
    return;
  }

  const auto* default_value_info = WebsiteSettingsRegistry::GetInstance()->Get(
      ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE);
  const base::Value default_value(
      prefs_->GetInteger(default_value_info->default_value_pref_name()));
  const auto auto_shread_mode =
      Remember1PStorageValueToAutoShredMode(default_value);
  // We should only migrate if AutoShredMode is not the default
  if (auto_shread_mode != brave_shields::mojom::AutoShredMode::NEVER) {
    SetWebsiteSettingInternal(
        ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
        ContentSettingsType::BRAVE_AUTO_SHRED,
        brave_shields::AutoShredSetting::ToValue(
            Remember1PStorageValueToAutoShredMode(default_value)),
        {});
  }

  std::vector<std::unique_ptr<Rule>> rules;
  auto rule_iterator =
      GetRuleIterator(ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE, false);
  while (rule_iterator && rule_iterator->HasNext()) {
    auto rule = rule_iterator->Next();
    rules.emplace_back(CloneRule(CHECK_DEREF(rule.get())));
  }
  rule_iterator.reset();

  for (const auto& fp_rule : rules) {
    PrefProvider::SetWebsiteSetting(
        fp_rule->primary_pattern, fp_rule->secondary_pattern,
        ContentSettingsType::BRAVE_AUTO_SHRED,
        brave_shields::AutoShredSetting::ToValue(
            Remember1PStorageValueToAutoShredMode(fp_rule->value)),
        {});
  }
  prefs_->SetBoolean(kBraveRemember1PStorageMigration, true);
}

void BravePrefProvider::MigrateCosmeticFilteringSettings() {
  if (off_the_record_ || prefs_->GetBoolean(kCosmeticFilteringMigration)) {
    return;
  }

  const auto& cosmetic_filtering =
      prefs_->GetDict(GetShieldsSettingUserPrefsPath(
          brave_shields::kObsoleteCosmeticFiltering));
  const auto* info = WebsiteSettingsRegistry::GetInstance()->Get(
      ContentSettingsType::BRAVE_COSMETIC_FILTERING);

  base::DictValue clone;
  for (const auto rule : cosmetic_filtering) {
    // Premigrate values to be consistent with base::DictValue() default
    // value.
    clone.Set(rule.first,
              base::DictValue().Set("setting", rule.second.Clone()));
  }

  prefs_->SetDict(info->pref_name(), std::move(clone));

  std::vector<std::unique_ptr<Rule>> rules;
  {
    auto rule_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_COSMETIC_FILTERING, false);
    while (rule_iterator && rule_iterator->HasNext()) {
      rules.push_back(rule_iterator->Next());
    }
  }

  const auto first_party =
      ContentSettingsPattern::FromString("https://firstParty/*");
  const auto wildcard = ContentSettingsPattern::Wildcard();

  auto merge_values = [](const Rule* fp_rule, const Rule* general_rule) {
    // Same logic as in GetCosmeticFilteringControlType.
    const auto* setting = general_rule->value.GetDict().Find("setting");
    if (!setting) {
      return base::Value();
    }
    if (content_settings::ValueToContentSetting(*setting) ==
        CONTENT_SETTING_ALLOW) {
      return brave_shields::CosmeticFilteringSetting::ToValue(
          brave_shields::ControlType::ALLOW);
    }

    setting = fp_rule->value.GetDict().Find("setting");
    if (!setting) {
      return base::Value();
    }
    if (content_settings::ValueToContentSetting(*setting) !=
        CONTENT_SETTING_BLOCK) {
      return brave_shields::CosmeticFilteringSetting::ToValue(
          brave_shields::ControlType::BLOCK_THIRD_PARTY);
    }
    return brave_shields::CosmeticFilteringSetting::ToValue(
        brave_shields::ControlType::BLOCK);
  };

  auto set_rule_value = [this](Rule* rule, base::Value value) {
    if (!rule) {
      return;
    }
    ContentSettingConstraints constraints;
    if (!value.is_none()) {
      constraints.set_session_model(rule->metadata.session_model());
    }
    SetWebsiteSettingInternal(rule->primary_pattern, rule->secondary_pattern,
                              ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                              std::move(value), constraints);
  };

  // BRAVE_COSMETIC_FILTERING rules are set in pairs:
  //  {(host, https://firstparty) | (host, *)}
  // RuleIterator returns them from more specific to more general,
  // meaning the first-party rule precedes the wildcard one.
  // Migrate only matched pairs; treat all other cases as invalid settings
  // to be dropped.
  Rule* fp_rule = nullptr;
  for (auto& rule : rules) {
    if (rule->secondary_pattern == first_party) {
      if (fp_rule) {
        // No general rule for previous first-party rule -> drop.
        set_rule_value(fp_rule, base::Value());
      }
      fp_rule = rule.get();
    } else if (rule->secondary_pattern == wildcard) {
      if (!fp_rule || rule->primary_pattern != fp_rule->primary_pattern) {
        // No first-party rule or it doesn't match with general rule -> drop
        // both.
        set_rule_value(fp_rule, base::Value());
        set_rule_value(rule.get(), base::Value());
        fp_rule = nullptr;
        continue;
      }
      // General rule matches with the first-party rule -> merge.
      set_rule_value(rule.get(), merge_values(fp_rule, rule.get()));
      set_rule_value(fp_rule, base::Value());
      fp_rule = nullptr;
    }
  }
  prefs_->SetBoolean(kCosmeticFilteringMigration, true);
}

bool BravePrefProvider::SetWebsiteSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const base::Value& in_value,
    const ContentSettingConstraints& constraints) {
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
    if (cookie_is_found_in(
            brave_effective_source_rules_[ContentSettingsType::COOKIES]
                                         [off_the_record_])) {
      // change to type ContentSettingsType::BRAVE_COOKIES
      return SetWebsiteSettingInternal(primary_pattern, secondary_pattern,
                                       ContentSettingsType::BRAVE_COOKIES,
                                       in_value, constraints);
    }
  }

  const auto javascript_is_found_in =
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

  if (content_type == ContentSettingsType::JAVASCRIPT &&
      javascript_is_found_in(
          brave_effective_source_rules_[ContentSettingsType::JAVASCRIPT]
                                       [off_the_record_])) {
    // Effective JAVASCRIPT rules generated from Shields should be edited in
    // the Shields-owned setting so the effective value remains derivable.
    return SetWebsiteSettingInternal(primary_pattern, secondary_pattern,
                                     ContentSettingsType::BRAVE_JAVASCRIPT,
                                     in_value, constraints);
  }

  return SetWebsiteSettingInternal(primary_pattern, secondary_pattern,
                                   content_type, in_value, constraints);
}

bool BravePrefProvider::SetWebsiteSettingForTest(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const base::Value& value,
    const ContentSettingConstraints& constraints) {
  return PrefProvider::SetWebsiteSetting(primary_pattern, secondary_pattern,
                                         content_type, value, constraints);
}

bool BravePrefProvider::SetWebsiteSettingInternal(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const base::Value& in_value,
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
                            in_value.Clone(), std::move(metadata));
    return true;
  }

  if (content_type == ContentSettingsType::BRAVE_FINGERPRINTING_V2 &&
      content_settings::ValueToContentSetting(in_value) !=
          CONTENT_SETTING_DEFAULT &&
      secondary_pattern ==
          ContentSettingsPattern::FromString("https://balanced/*")) {
    return false;
  }

  return PrefProvider::SetWebsiteSetting(primary_pattern, secondary_pattern,
                                         content_type, in_value, constraints);
}

const OriginValueMap* BravePrefProvider::GetBraveEffectiveRules(
    ContentSettingsType content_type,
    bool off_the_record) const {
  // COOKIES and JAVASCRIPT are served from the materialized effective maps so
  // GetRule and GetRuleIterator stay consistent and the Shields-scope
  // precedence computed in UpdateCookieRules/UpdateJavascriptRules remains the
  // single source of truth.
  auto type_it = brave_effective_rules_.find(content_type);
  if (type_it == brave_effective_rules_.end()) {
    return nullptr;
  }
  auto incognito_it = type_it->second.find(off_the_record);
  if (incognito_it == type_it->second.end()) {
    return nullptr;
  }
  return &incognito_it->second;
}

std::unique_ptr<RuleIterator> BravePrefProvider::GetRuleIterator(
    ContentSettingsType content_type,
    bool off_the_record) const {
  if (const OriginValueMap* rules =
          GetBraveEffectiveRules(content_type, off_the_record)) {
    return rules->GetRuleIterator(content_type);
  }

  return PrefProvider::GetRuleIterator(content_type, off_the_record);
}

std::unique_ptr<Rule> BravePrefProvider::GetRule(
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    bool off_the_record) const {
  if (const OriginValueMap* rules =
          GetBraveEffectiveRules(content_type, off_the_record)) {
    base::AutoLock auto_lock(rules->GetLock());
    return rules->GetRule(primary_url, secondary_url, content_type);
  }

  return PrefProvider::GetRule(primary_url, secondary_url, content_type,
                               off_the_record);
}

BravePrefProvider::CookieType BravePrefProvider::GetCookieType(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    const ContentSetting& value,
    bool incognito) const {
  const auto find_cookie =
      [&primary_pattern, &secondary_pattern,
       &value](const std::vector<std::unique_ptr<Rule>>& rules) {
        for (const auto& rule : rules) {
          if (rule->primary_pattern == primary_pattern &&
              rule->secondary_pattern == secondary_pattern &&
              rule->value == value) {
            return true;
          }
        }
        return false;
      };

  if (find_cookie(brave_shield_down_rules_.at(incognito))) {
    return CookieType::kShieldsDownCookie;
  }

  if (auto type_it =
          brave_effective_source_rules_.find(ContentSettingsType::COOKIES);
      type_it != brave_effective_source_rules_.end()) {
    if (auto incognito_it = type_it->second.find(incognito);
        incognito_it != type_it->second.end() &&
        find_cookie(incognito_it->second)) {
      return CookieType::kCustomShieldsCookie;
    }
  }

  const bool google_sign_in_flag_enabled =
      google_sign_in_permission::IsGoogleSignInFeatureEnabled();
  if (!google_sign_in_flag_enabled &&
      prefs_->FindPreference(kGoogleLoginControlType) &&
      prefs_->GetBoolean(kGoogleLoginControlType)) {
    if ((primary_pattern == google_sign_in_permission::GetGoogleAuthPattern() ||
         primary_pattern ==
             google_sign_in_permission::GetFirebaseAuthPattern()) &&
        secondary_pattern == ContentSettingsPattern::Wildcard()) {
      return CookieType::kGoogleSignInCookie;
    }
  }
  return CookieType::kRegularCookie;
}

void BravePrefProvider::UpdateBraveEffectiveRules(
    const EffectiveRuleConfig& config,
    bool incognito,
    ContentSettingsType changed_type,
    base::FunctionRef<void(std::vector<std::unique_ptr<Rule>>&,
                           std::vector<std::unique_ptr<Rule>>&)>
        inject_source_rules,
    base::FunctionRef<void(const std::vector<std::unique_ptr<Rule>>&,
                           std::vector<std::unique_ptr<Rule>>&,
                           std::vector<std::unique_ptr<Rule>>&)>
        apply_shields_down) {
  auto& effective_rules =
      brave_effective_rules_[config.effective_type][incognito];
  auto& source_rules =
      brave_effective_source_rules_[config.effective_type][incognito];

  std::vector<std::unique_ptr<Rule>> rules;
  auto old_rules = std::move(source_rules);
  source_rules.clear();

  // Type-specific source rules that must precede the Shields overrides (e.g.
  // Google Sign-In cookie exceptions). None for JavaScript.
  inject_source_rules(rules, source_rules);

  // Baseline rules from the upstream content type (which shares its type value
  // with the effective type we produce).
  {
    auto chromium_iterator =
        PrefProvider::GetRuleIterator(config.effective_type, incognito);
    while (chromium_iterator && chromium_iterator->HasNext()) {
      rules.emplace_back(
          CloneRule(CHECK_DEREF(chromium_iterator->Next().get())));
    }
  }

  // Current Shields state.
  std::vector<std::unique_ptr<Rule>> shield_rules;
  {
    auto brave_shields_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_SHIELDS, incognito);
    while (brave_shields_iterator && brave_shields_iterator->HasNext()) {
      shield_rules.emplace_back(
          CloneRule(CHECK_DEREF(brave_shields_iterator->Next().get())));
    }
  }

  // Shields-owned per-site overrides, kept only while Shields are up for their
  // top-level site.
  {
    auto brave_iterator =
        PrefProvider::GetRuleIterator(config.brave_type, incognito);
    while (brave_iterator && brave_iterator->HasNext()) {
      auto rule = brave_iterator->Next();
      const ContentSettingsPattern& top_level_pattern =
          config.top_level_is_primary ? rule->primary_pattern
                                      : rule->secondary_pattern;
      // A global (wildcard/wildcard) Shields rule sets the effective default
      // directly. This only matters for JavaScript: cookies derive their
      // default from the Chromium COOKIES setting, so their wildcard rule is
      // excluded by IsActive and never reaches here.
      const bool is_default_rule =
          rule->primary_pattern == ContentSettingsPattern::Wildcard() &&
          rule->secondary_pattern == ContentSettingsPattern::Wildcard();
      if (is_default_rule ||
          IsActive(rule.get(), top_level_pattern, shield_rules)) {
        rules.emplace_back(CloneRule(CHECK_DEREF(rule.get())));
        source_rules.emplace_back(CloneRule(CHECK_DEREF(rule.get())));
      }
    }
  }

  // Type-specific Shields-down overrides (cookies only).
  apply_shields_down(shield_rules, rules, source_rules);

  std::vector<std::unique_ptr<Rule>> updates =
      ComputeRuleUpdates(old_rules, source_rules);
  {
    base::AutoLock lock(effective_rules.GetLock());
    effective_rules.clear();
    for (auto&& r : rules) {
      effective_rules.SetValue(r->primary_pattern, r->secondary_pattern,
                               config.effective_type, std::move(r->value),
                               std::move(r->metadata));
    }
  }

  if (initialized_ && std::ranges::find(config.notify_on_types, changed_type) !=
                          config.notify_on_types.end()) {
    NotifyChanges(updates, config.effective_type, incognito);
  }
}

void BravePrefProvider::UpdateCookieRules(ContentSettingsType content_type,
                                          bool incognito) {
  const EffectiveRuleConfig kConfig = {
      .effective_type = ContentSettingsType::COOKIES,
      .brave_type = ContentSettingsType::BRAVE_COOKIES,
      // For cookies the top-level site is the secondary (embedding) pattern.
      .top_level_is_primary = false,
      .notify_on_types = {ContentSettingsType::BRAVE_COOKIES,
                          ContentSettingsType::BRAVE_SHIELDS,
                          ContentSettingsType::BRAVE_GOOGLE_SIGN_IN}};

  const auto inject_google_sign_in_rules =
      [&](std::vector<std::unique_ptr<Rule>>& rules,
          std::vector<std::unique_ptr<Rule>>& source_rules) {
        const bool google_sign_in_flag_enabled =
            google_sign_in_permission::IsGoogleSignInFeatureEnabled();

        // If Google Sign-In permission feature flag is disabled,
        // we add 3p cookie exception globally for Google/Firebase auth domains.
        // TODO(ssahib): Remove this once we no longer need to support the flag.
        if (!google_sign_in_flag_enabled &&
            prefs_->FindPreference(kGoogleLoginControlType) &&
            prefs_->GetBoolean(kGoogleLoginControlType)) {
          // kGoogleLoginControlType preference adds an exception for
          // accounts.google.com to access cookies in 3p context to allow login
          // using google oauth. The exception is added before all overrides to
          // allow google oauth to work when the user sets custom overrides for
          // a site. For example: Google OAuth will be allowed if the user
          // allows all cookies and sets 3p cookie blocking for a site.
          //
          // We also create the same exception for firebase apps, since they
          // are tightly bound to google, and require google auth to work.
          // See: #5075, #9852, #10367
          //
          // PS: kGoogleLoginControlType preference might not be registered for
          // tests.
          RuleMetaData metadata;
          metadata.SetExpirationAndLifetime(base::Time(), base::TimeDelta());
          metadata.set_session_model(
              content_settings::mojom::SessionModel::DURABLE);
          const auto google_auth_rule = std::make_unique<Rule>(
              google_sign_in_permission::GetGoogleAuthPattern(),
              ContentSettingsPattern::Wildcard(),
              ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());
          rules.emplace_back(CloneRule(*google_auth_rule));
          source_rules.emplace_back(CloneRule(*google_auth_rule));

          const auto firebase_rule = std::make_unique<Rule>(
              google_sign_in_permission::GetFirebaseAuthPattern(),
              ContentSettingsPattern::Wildcard(),
              ContentSettingToValue(CONTENT_SETTING_ALLOW),
              std::move(metadata));
          rules.emplace_back(CloneRule(*firebase_rule));
          source_rules.emplace_back(CloneRule(*firebase_rule));
        } else if (google_sign_in_flag_enabled) {
          // Google Sign-In feature:
          // Add per-site cookie exception for Google/Firebase auth domains.
          // Get all sites that have BRAVE_GOOGLE_SIGN_IN turned on, and add
          // exception for them
          auto google_sign_in_content_setting_it =
              PrefProvider::GetRuleIterator(
                  ContentSettingsType::BRAVE_GOOGLE_SIGN_IN, incognito);
          while (google_sign_in_content_setting_it &&
                 google_sign_in_content_setting_it->HasNext()) {
            const auto google_sign_in_rule =
                google_sign_in_content_setting_it->Next();

            RuleMetaData metadata;
            metadata.SetExpirationAndLifetime(
                google_sign_in_rule->metadata.expiration(), base::TimeDelta());
            metadata.set_session_model(
                google_sign_in_rule->metadata.session_model());
            // The embedding pattern for the cookie rule will be the primary
            // pattern for the BRAVE_GOOGLE_SIGN_IN permission.
            // We want to get all subdomains for the cookie rule...
            auto embedding_pattern =
                ContentSettingsPattern::ToDomainWildcardPattern(
                    google_sign_in_rule->primary_pattern);
            // ... but if that doesn't work, fallback to stored pattern.
            if (!embedding_pattern.IsValid()) {
              embedding_pattern = google_sign_in_rule->primary_pattern;
            }
            const auto google_auth_rule = std::make_unique<Rule>(
                google_sign_in_permission::GetGoogleAuthPattern(),
                embedding_pattern, google_sign_in_rule->value.Clone(),
                metadata.Clone());
            rules.emplace_back(CloneRule(*google_auth_rule));
            source_rules.emplace_back(CloneRule(*google_auth_rule));

            const auto firebase_rule = std::make_unique<Rule>(
                google_sign_in_permission::GetFirebaseAuthPattern(),
                embedding_pattern, google_sign_in_rule->value.Clone(),
                std::move(metadata));
            rules.emplace_back(CloneRule(*firebase_rule));
            source_rules.emplace_back(CloneRule(*firebase_rule));
          }
        }
        // Non-pref based exceptions should go in the cookie_settings_base.cc
        // chromium_src override.
      };

  const auto apply_cookie_shields_down =
      [&](const std::vector<std::unique_ptr<Rule>>& shield_rules,
          std::vector<std::unique_ptr<Rule>>& rules,
          std::vector<std::unique_ptr<Rule>>& source_rules) {
        auto old_shields_down_rules =
            std::move(brave_shield_down_rules_[incognito]);
        // Adding shields down rules (they always override cookie rules).
        for (const auto& shield_rule : shield_rules) {
          // There is no global shields rule, so if we have one ignore it. It
          // would get replaced with EnsureNoWildcardEntries().
          if (shield_rule->primary_pattern.MatchesAllHosts()) {
            LOG(ERROR)
                << "Found a wildcard shields rule which matches all hosts.";
            continue;
          }

          // Shields down.
          if (ValueToContentSetting(shield_rule->value) ==
              CONTENT_SETTING_BLOCK) {
            RuleMetaData metadata;
            metadata.SetExpirationAndLifetime(base::Time(), base::TimeDelta());
            metadata.set_session_model(
                content_settings::mojom::SessionModel::DURABLE);

            rules.emplace_back(std::make_unique<Rule>(
                ContentSettingsPattern::Wildcard(),
                shield_rule->primary_pattern,
                ContentSettingToValue(CONTENT_SETTING_ALLOW),
                metadata.Clone()));
            brave_shield_down_rules_[incognito].emplace_back(
                std::make_unique<Rule>(
                    ContentSettingsPattern::Wildcard(),
                    shield_rule->primary_pattern,
                    ContentSettingToValue(CONTENT_SETTING_ALLOW),
                    metadata.Clone()));
            source_rules.emplace_back(std::make_unique<Rule>(
                ContentSettingsPattern::Wildcard(),
                shield_rule->primary_pattern,
                ContentSettingToValue(CONTENT_SETTING_ALLOW),
                std::move(metadata)));
          }
        }
      };

  UpdateBraveEffectiveRules(kConfig, incognito, content_type,
                            inject_google_sign_in_rules,
                            apply_cookie_shields_down);
}

void BravePrefProvider::UpdateJavascriptRules(ContentSettingsType content_type,
                                              bool incognito) {
  const EffectiveRuleConfig kConfig = {
      .effective_type = ContentSettingsType::JAVASCRIPT,
      .brave_type = ContentSettingsType::BRAVE_JAVASCRIPT,
      // For JavaScript the top-level site is the primary pattern.
      .top_level_is_primary = true,
      .notify_on_types = {ContentSettingsType::BRAVE_JAVASCRIPT,
                          ContentSettingsType::BRAVE_SHIELDS}};

  UpdateBraveEffectiveRules(
      kConfig, incognito, content_type,
      [](std::vector<std::unique_ptr<Rule>>&,
         std::vector<std::unique_ptr<Rule>>&) {},
      [](const std::vector<std::unique_ptr<Rule>>&,
         std::vector<std::unique_ptr<Rule>>&,
         std::vector<std::unique_ptr<Rule>>&) {});
}

void BravePrefProvider::NotifyChanges(
    const std::vector<std::unique_ptr<Rule>>& rules,
    ContentSettingsType content_type,
    bool incognito) {
  for (const auto& rule : rules) {
    Notify(rule->primary_pattern, rule->secondary_pattern, content_type);
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

void BravePrefProvider::OnJavascriptSettingsChanged(
    ContentSettingsType content_type) {
  UpdateJavascriptRules(content_type, true);
  UpdateJavascriptRules(content_type, false);
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

  if (content_type == ContentSettingsType::JAVASCRIPT ||
      content_type == ContentSettingsType::BRAVE_JAVASCRIPT ||
      content_type == ContentSettingsType::BRAVE_SHIELDS) {
    OnJavascriptSettingsChanged(content_type);
  }
}

}  // namespace content_settings
