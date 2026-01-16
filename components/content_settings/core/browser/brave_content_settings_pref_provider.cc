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
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
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

constexpr char kCosmeticFilteringMigration[] =
    "brave.cosmetic_filtering_migration";

std::unique_ptr<Rule> CloneRule(const Rule& original_rule) {
  return std::make_unique<Rule>(
      original_rule.primary_pattern, original_rule.secondary_pattern,
      original_rule.value.Clone(), original_rule.metadata.Clone());
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

  ClearWildcards();

  MigrateCosmeticFilteringSettings();

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
  registry->RegisterIntegerPref(kBraveShieldsSettingsVersion, 4);

  registry->RegisterDictionaryPref(GetShieldsSettingUserPrefsPath(
      brave_shields::kObsoleteCosmeticFiltering));
  registry->RegisterBooleanPref(kCosmeticFilteringMigration, false);
}

void BravePrefProvider::ClearWildcards() {
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

void BravePrefProvider::MigrateCosmeticFilteringSettings() {
  if (off_the_record_ || prefs_->GetBoolean(kCosmeticFilteringMigration)) {
    return;
  }

  const auto& cosmetic_filtering =
      prefs_->GetDict(GetShieldsSettingUserPrefsPath(
          brave_shields::kObsoleteCosmeticFiltering));
  const auto* info = WebsiteSettingsRegistry::GetInstance()->Get(
      ContentSettingsType::BRAVE_COSMETIC_FILTERING);

  base::Value::Dict clone;
  for (const auto rule : cosmetic_filtering) {
    // Premigrate values to be consistent with base::Value::Dict() default
    // value.
    clone.Set(rule.first,
              base::Value::Dict().Set("setting", rule.second.Clone()));
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
    base::Value&& in_value,
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
                            std::move(in_value), std::move(metadata));
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
                                         content_type, std::move(in_value),
                                         constraints);
}

std::unique_ptr<RuleIterator> BravePrefProvider::GetRuleIterator(
    ContentSettingsType content_type,
    bool off_the_record) const {
  if (content_type == ContentSettingsType::COOKIES) {
    const auto& rules = cookie_rules_.at(off_the_record);
    return rules.GetRuleIterator(content_type);
  }

  return PrefProvider::GetRuleIterator(content_type, off_the_record);
}

std::unique_ptr<Rule> BravePrefProvider::GetRule(
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    bool off_the_record) const {
  if (content_type == ContentSettingsType::COOKIES) {
    const auto& rules = cookie_rules_.at(off_the_record);
    base::AutoLock auto_lock(rules.GetLock());
    return rules.GetRule(primary_url, secondary_url, content_type);
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

  if (find_cookie(brave_cookie_rules_.at(incognito))) {
    return CookieType::kCustomShieldsCookie;
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
        ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());
    rules.emplace_back(CloneRule(*google_auth_rule));
    brave_cookie_rules_[incognito].emplace_back(CloneRule(*google_auth_rule));

    const auto firebase_rule = std::make_unique<Rule>(
        google_sign_in_permission::GetFirebaseAuthPattern(),
        ContentSettingsPattern::Wildcard(),
        ContentSettingToValue(CONTENT_SETTING_ALLOW), std::move(metadata));
    rules.emplace_back(CloneRule(*firebase_rule));
    brave_cookie_rules_[incognito].emplace_back(CloneRule(*firebase_rule));
  } else if (google_sign_in_flag_enabled) {
    // Google Sign-In feature:
    // Add per-site cookie exception for Google/Firebase auth domains.
    // Get all sites that have BRAVE_GOOGLE_SIGN_IN turned on, and add exception
    // for them
    auto google_sign_in_content_setting_it = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_GOOGLE_SIGN_IN, incognito);
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
          google_sign_in_rule->value.Clone(), metadata.Clone());
      rules.emplace_back(CloneRule(*google_auth_rule));
      brave_cookie_rules_[incognito].emplace_back(CloneRule(*google_auth_rule));

      const auto firebase_rule = std::make_unique<Rule>(
          google_sign_in_permission::GetFirebaseAuthPattern(),
          embedding_pattern, google_sign_in_rule->value.Clone(),
          std::move(metadata));
      rules.emplace_back(CloneRule(*firebase_rule));
      brave_cookie_rules_[incognito].emplace_back(CloneRule(*firebase_rule));
    }
  }

  // Non-pref based exceptions should go in the cookie_settings_base.cc
  // chromium_src override.

  // Add chromium cookies.
  {
    auto chromium_cookies_iterator =
        PrefProvider::GetRuleIterator(ContentSettingsType::COOKIES, incognito);
    while (chromium_cookies_iterator && chromium_cookies_iterator->HasNext()) {
      rules.emplace_back(
          CloneRule(CHECK_DEREF(chromium_cookies_iterator->Next().get())));
    }
  }

  // Collect shield rules.
  std::vector<std::unique_ptr<Rule>> shield_rules;
  {
    auto brave_shields_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_SHIELDS, incognito);
    while (brave_shields_iterator && brave_shields_iterator->HasNext()) {
      shield_rules.emplace_back(
          CloneRule(CHECK_DEREF(brave_shields_iterator->Next().get())));
    }
  }

  // Add brave cookies after checking shield status.
  {
    auto brave_cookies_iterator = PrefProvider::GetRuleIterator(
        ContentSettingsType::BRAVE_COOKIES, incognito);
    // Matching cookie rules against shield rules.
    while (brave_cookies_iterator && brave_cookies_iterator->HasNext()) {
      auto rule = brave_cookies_iterator->Next();
      if (IsActive(rule.get(), shield_rules)) {
        rules.emplace_back(CloneRule(CHECK_DEREF(rule.get())));
        brave_cookie_rules_[incognito].emplace_back(
            CloneRule(CHECK_DEREF(rule.get())));
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
          ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone()));
      brave_shield_down_rules_[incognito].emplace_back(std::make_unique<Rule>(
          ContentSettingsPattern::Wildcard(), shield_rule->primary_pattern,
          ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone()));
      brave_cookie_rules_[incognito].emplace_back(std::make_unique<Rule>(
          ContentSettingsPattern::Wildcard(), shield_rule->primary_pattern,
          ContentSettingToValue(CONTENT_SETTING_ALLOW), std::move(metadata)));
    }
  }

  // Get the list of changes.
  std::vector<std::unique_ptr<Rule>> brave_cookie_updates;
  for (const auto& new_rule : brave_cookie_rules_[incognito]) {
    auto match =
        std::ranges::find_if(old_rules, [&new_rule](const auto& old_rule) {
          // we want an exact match here because any change to the rule
          // is an update
          return new_rule->primary_pattern == old_rule->primary_pattern &&
                 new_rule->secondary_pattern == old_rule->secondary_pattern &&
                 ValueToContentSetting(new_rule->value) ==
                     ValueToContentSetting(old_rule->value);
        });
    if (match == old_rules.end()) {
      brave_cookie_updates.emplace_back(CloneRule(CHECK_DEREF(new_rule.get())));
    }
  }

  // Find any removed rules.
  for (const auto& old_rule : old_rules) {
    auto match = std::ranges::find_if(
        brave_cookie_rules_[incognito], [&old_rule](const auto& new_rule) {
          // We only care about the patterns here because we're looking for
          // deleted rules, not changed rules.
          return new_rule->primary_pattern == old_rule->primary_pattern &&
                 new_rule->secondary_pattern == old_rule->secondary_pattern;
        });
    if (match == brave_cookie_rules_[incognito].end()) {
      brave_cookie_updates.emplace_back(std::make_unique<Rule>(
          old_rule->primary_pattern, old_rule->secondary_pattern, base::Value(),
          old_rule->metadata.Clone()));
    }
  }
  {
    base::AutoLock lock(cookie_rules_[incognito].GetLock());
    cookie_rules_[incognito].clear();
    for (auto&& r : rules) {
      cookie_rules_[incognito].SetValue(
          r->primary_pattern, r->secondary_pattern,
          ContentSettingsType::COOKIES, std::move(r->value),
          std::move(r->metadata));
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
      content_type == ContentSettingsType::BRAVE_SHIELDS ||
      content_type == ContentSettingsType::BRAVE_GOOGLE_SIGN_IN) {
    OnCookieSettingsChanged(content_type);
  }
}

}  // namespace content_settings
