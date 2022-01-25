/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/post_task.h"
#include "brave/common/network_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "components/content_settings/core/browser/content_settings_pref.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
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

constexpr char kGoogleAuthPattern[] = "https://accounts.google.com/*";
constexpr char kFirebasePattern[] = "https://[*.]firebaseapp.com/*";

const char kExpirationPath[] = "expiration";
const char kLastModifiedPath[] = "last_modified";
const char kSessionModelPath[] = "model";
const char kSettingPath[] = "setting";
const char kPerResourcePath[] = "per_resource";

Rule CloneRule(const Rule& rule, bool reverse_patterns = false) {
  // brave plugin rules incorrectly use first party url as primary
  auto primary_pattern = reverse_patterns ? rule.secondary_pattern
                                          : rule.primary_pattern;
  auto secondary_pattern = reverse_patterns ? rule.primary_pattern
                                            : rule.secondary_pattern;

  if (primary_pattern ==
      ContentSettingsPattern::FromString("https://firstParty/*")) {
    DCHECK(reverse_patterns);  // we should only hit this for brave plugin rules
    if (!secondary_pattern.MatchesAllHosts()) {
      primary_pattern = ContentSettingsPattern::FromString(
          "*://[*.]" +
          net::registry_controlled_domains::GetDomainAndRegistry(
              secondary_pattern.GetHost(),
              net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) +
          "/*");
    } else {
      primary_pattern = secondary_pattern;
    }
  }

  return Rule(primary_pattern, secondary_pattern, rule.value.Clone(),
              rule.expiration, rule.session_model);
}

class BraveShieldsRuleIterator : public RuleIterator {
 public:
  explicit BraveShieldsRuleIterator(std::vector<Rule> rules)
      : rules_(std::move(rules)) {
    iterator_ = rules_.begin();
  }

  BraveShieldsRuleIterator(const BraveShieldsRuleIterator&) = delete;

  BraveShieldsRuleIterator& operator=(const BraveShieldsRuleIterator&) = delete;

  bool HasNext() const override {
    return iterator_ != rules_.end();
  }

  Rule Next() override {
    return CloneRule(*(iterator_++));
  }

 private:
  std::vector<Rule> rules_;
  std::vector<Rule>::const_iterator iterator_;
};


bool IsActive(const Rule& cookie_rule,
              const std::vector<Rule>& shield_rules) {
  // don't include default rules in the iterator
  if (cookie_rule.primary_pattern == ContentSettingsPattern::Wildcard() &&
      (cookie_rule.secondary_pattern == ContentSettingsPattern::Wildcard() ||
       cookie_rule.secondary_pattern ==
          ContentSettingsPattern::FromString("https://firstParty/*"))) {
    return false;
  }

  bool default_value = true;
  for (const auto& shield_rule : shield_rules) {
    auto primary_compare =
        shield_rule.primary_pattern.Compare(cookie_rule.primary_pattern);
    // TODO(bridiver) - verify that SUCCESSOR is correct and not PREDECESSOR
    if (primary_compare == ContentSettingsPattern::IDENTITY ||
        primary_compare == ContentSettingsPattern::SUCCESSOR) {
      // TODO(bridiver) - move this logic into shields_util for allow/block
      return
          ValueToContentSetting(&shield_rule.value) != CONTENT_SETTING_BLOCK;
    }
  }

  return default_value;
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
  pref_change_registrar_.Add(
      kGoogleLoginControlType,
      base::BindRepeating(&BravePrefProvider::OnCookiePrefsChanged,
                          base::Unretained(this)));

  MigrateShieldsSettings(off_the_record);

  OnCookieSettingsChanged(ContentSettingsType::BRAVE_COOKIES);

  // Enable change notifications after initial setup to avoid notification spam
  initialized_ = true;
  AddObserver(this);
}

BravePrefProvider::~BravePrefProvider() {}

void BravePrefProvider::ShutdownOnUIThread() {
  RemoveObserver(this);
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

#if defined(OS_ANDROID)
  // This path is no longer registered upstream but we still need it to migrate
  // Shields settings away from ResourceIdentifier on Android.
  if (!registry->defaults()->GetValue(
          "profile.content_settings.exceptions.plugins", nullptr)) {
    registry->RegisterDictionaryPref(
        "profile.content_settings.exceptions.plugins");
  }
#endif
}

void BravePrefProvider::MigrateShieldsSettings(bool incognito) {
  // Incognito inherits from regular profile, so nothing to do.
  // Guest doesn't inherit, but only keeps settings for the duration of the
  // session, so also nothing to do.
  if (incognito)
    return;

  // Prior to Chromium 88, we used the "plugins" ContentSettingsType along with
  // ResourceIdentifiers to store our settings, which we need to migrate now
  // first of all, before attempting any other migration.
  MigrateShieldsSettingsFromResourceIds();

  // Now carry on with any other migration that we might need.
  MigrateShieldsSettingsV1ToV2();
}

void BravePrefProvider::MigrateShieldsSettingsFromResourceIds() {
  BravePrefProvider::CopyPluginSettingsForMigration(prefs_);

  const base::DictionaryValue* plugins_dictionary = prefs_->GetDictionary(
      "brave.migrate.content_settings.exceptions.plugins");
  if (!plugins_dictionary)
    return;

  for (base::DictionaryValue::Iterator i(*plugins_dictionary); !i.IsAtEnd();
       i.Advance()) {
    const std::string& patterns_string(i.key());
    const base::DictionaryValue* settings_dictionary = nullptr;
    bool is_dictionary = i.value().GetAsDictionary(&settings_dictionary);
    DCHECK(is_dictionary);

    base::Time expiration =
        GetTimeStampFromDictionary(settings_dictionary, kExpirationPath);
    SessionModel session_model =
        GetSessionModelFromDictionary(settings_dictionary, kSessionModelPath);

    const base::DictionaryValue* resource_dictionary = nullptr;
    if (settings_dictionary->GetDictionary(kPerResourcePath,
                                           &resource_dictionary)) {
      base::Time last_modified =
          GetTimeStampFromDictionary(settings_dictionary, kLastModifiedPath);
      for (base::DictionaryValue::Iterator j(*resource_dictionary);
           !j.IsAtEnd(); j.Advance()) {
        const std::string& resource_identifier(j.key());
        std::string shields_preference_name;

        // For "ads" and "cookies" we need to adapt the name to the new one,
        // otherwise it will refer to upstream's "ads" and "cookies" settings.
        if (resource_identifier == brave_shields::kObsoleteAds)
          shields_preference_name = brave_shields::kAds;
        else if (resource_identifier == brave_shields::kObsoleteCookies)
          shields_preference_name = brave_shields::kCookies;
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

        int setting = CONTENT_SETTING_DEFAULT;
        bool is_integer = j.value().is_int();
        if (is_integer)
          setting = j.value().GetInt();
        DCHECK(is_integer);
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
        ContentSettingToValue(ValueToContentSetting(&(new_rules[i].value))),
        {new_rules[i].expiration, new_rules[i].session_model});
  }
}

bool BravePrefProvider::SetWebsiteSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    std::unique_ptr<base::Value>&& in_value,
    const ContentSettingConstraints& constraints) {
  // handle changes to brave cookie settings from chromium cookie settings UI
  if (content_type == ContentSettingsType::COOKIES) {
    auto* value = in_value.get();
    auto match = std::find_if(
        brave_cookie_rules_[off_the_record_].begin(),
        brave_cookie_rules_[off_the_record_].end(),
        [primary_pattern, secondary_pattern, value](const auto& rule) {
          return rule.primary_pattern == primary_pattern &&
                 rule.secondary_pattern == secondary_pattern &&
                 ValueToContentSetting(&rule.value) !=
                    ValueToContentSetting(value); });
    if (match != brave_cookie_rules_[off_the_record_].end()) {
      // swap primary/secondary pattern - see CloneRule
      auto plugin_primary_pattern = secondary_pattern;
      auto plugin_secondary_pattern = primary_pattern;

      // convert to legacy firstParty format for brave plugin settings
      if (plugin_primary_pattern == plugin_secondary_pattern) {
        plugin_secondary_pattern =
            ContentSettingsPattern::FromString("https://firstParty/*");
      }

      // change to type ContentSettingsType::BRAVE_COOKIES
      return SetWebsiteSettingInternal(
          plugin_primary_pattern, plugin_secondary_pattern,
          ContentSettingsType::BRAVE_COOKIES, std::move(in_value), constraints);
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
    std::unique_ptr<base::Value>&& in_value,
    const ContentSettingConstraints& constraints) {
  // PrefProvider ignores default settings so handle them here for shields
  if (content_settings::IsShieldsContentSettingsType(content_type) &&
      primary_pattern == ContentSettingsPattern::Wildcard() &&
      secondary_pattern == ContentSettingsPattern::Wildcard()) {
    base::Time modified_time =
        store_last_modified_ ? base::Time::Now() : base::Time();

    return GetPref(content_type)
        ->SetWebsiteSetting(primary_pattern, secondary_pattern, modified_time,
                            std::move(in_value), constraints);
  }

  return PrefProvider::SetWebsiteSetting(primary_pattern, secondary_pattern,
                                         content_type, std::move(in_value),
                                         constraints);
}

std::unique_ptr<RuleIterator> BravePrefProvider::GetRuleIterator(
      ContentSettingsType content_type,
      bool incognito) const {
  if (content_type == ContentSettingsType::COOKIES) {
    std::vector<Rule> rules;
    for (auto i = cookie_rules_.at(incognito).begin();
         i != cookie_rules_.at(incognito).end();
         ++i) {
      rules.emplace_back(CloneRule(*i));
    }

    return std::make_unique<BraveShieldsRuleIterator>(std::move(rules));
  }

  return PrefProvider::GetRuleIterator(content_type, incognito);
}

void BravePrefProvider::UpdateCookieRules(ContentSettingsType content_type,
                                          bool incognito) {
  auto& rules = cookie_rules_[incognito];
  auto old_rules = std::move(brave_cookie_rules_[incognito]);

  rules.clear();
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
  if (prefs_->GetBoolean(kGoogleLoginControlType)) {
    const auto google_auth_rule = Rule(
        ContentSettingsPattern::FromString(kGoogleAuthPattern),
        ContentSettingsPattern::Wildcard(),
        base::Value::FromUniquePtrValue(
                         ContentSettingToValue(CONTENT_SETTING_ALLOW)),
                     base::Time(), SessionModel::Durable);
    rules.emplace_back(CloneRule(google_auth_rule));
    brave_cookie_rules_[incognito].emplace_back(CloneRule(google_auth_rule));

    const auto firebase_rule = Rule(
        ContentSettingsPattern::FromString(kFirebasePattern),
        ContentSettingsPattern::Wildcard(),
        base::Value::FromUniquePtrValue(
            ContentSettingToValue(CONTENT_SETTING_ALLOW)),
        base::Time(), SessionModel::Durable);
    rules.emplace_back(CloneRule(firebase_rule));
    brave_cookie_rules_[incognito].emplace_back(CloneRule(firebase_rule));
  }
  // non-pref based exceptions should go in the cookie_settings_base.cc
  // chromium_src override

  // add chromium cookies
  auto chromium_cookies_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::COOKIES,
      incognito);
  while (chromium_cookies_iterator && chromium_cookies_iterator->HasNext()) {
    rules.emplace_back(CloneRule(chromium_cookies_iterator->Next()));
  }
  chromium_cookies_iterator.reset();

  auto brave_shields_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_SHIELDS, incognito);

  // collect shield rules
  std::vector<Rule> shield_rules;
  while (brave_shields_iterator && brave_shields_iterator->HasNext()) {
    shield_rules.emplace_back(CloneRule(brave_shields_iterator->Next()));
  }

  brave_shields_iterator.reset();

  // add brave cookies after checking shield status
  auto brave_cookies_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::BRAVE_COOKIES, incognito);

  // Matching cookie rules against shield rules.
  while (brave_cookies_iterator && brave_cookies_iterator->HasNext()) {
    auto rule = brave_cookies_iterator->Next();
    if (IsActive(rule, shield_rules)) {
      rules.emplace_back(CloneRule(rule, true));
      brave_cookie_rules_[incognito].emplace_back(CloneRule(rule, true));
    }
  }

  // Adding shields down rules (they always override cookie rules).
  for (const auto& shield_rule : shield_rules) {
    // There is no global shields rule
    if (shield_rule.primary_pattern.MatchesAllHosts())
      NOTREACHED();

    // Shields down.
    if (ValueToContentSetting(&shield_rule.value) == CONTENT_SETTING_BLOCK) {
      rules.emplace_back(
          Rule(ContentSettingsPattern::Wildcard(),
               shield_rule.primary_pattern,
               base::Value::FromUniquePtrValue(
                   ContentSettingToValue(CONTENT_SETTING_ALLOW)),
               base::Time(), SessionModel::Durable));
      brave_cookie_rules_[incognito].emplace_back(
          Rule(ContentSettingsPattern::Wildcard(),
               shield_rule.primary_pattern,
               base::Value::FromUniquePtrValue(
                   ContentSettingToValue(CONTENT_SETTING_ALLOW)),
               base::Time(), SessionModel::Durable));
    }
  }

  // get the list of changes
  std::vector<Rule> brave_cookie_updates;
  for (const auto& new_rule : brave_cookie_rules_[incognito]) {
    auto match = std::find_if(
        old_rules.begin(),
        old_rules.end(),
        [&new_rule](const auto& old_rule) {
          // we want an exact match here because any change to the rule
          // is an update
          return new_rule.primary_pattern == old_rule.primary_pattern &&
                 new_rule.secondary_pattern == old_rule.secondary_pattern &&
                 ValueToContentSetting(&new_rule.value) ==
                    ValueToContentSetting(&old_rule.value);
        });
    if (match == old_rules.end()) {
      brave_cookie_updates.emplace_back(CloneRule(new_rule));
    }
  }

  // find any removed rules
  for (const auto& old_rule : old_rules) {
    auto match = std::find_if(
        brave_cookie_rules_[incognito].begin(),
        brave_cookie_rules_[incognito].end(),
        [&old_rule](const auto& new_rule) {
          // we only care about the patterns here because we're looking
          // for deleted rules, not changed rules
          return new_rule.primary_pattern == old_rule.primary_pattern &&
                 new_rule.secondary_pattern == old_rule.secondary_pattern;
        });
    if (match == brave_cookie_rules_[incognito].end()) {
      brave_cookie_updates.emplace_back(
          Rule(old_rule.primary_pattern, old_rule.secondary_pattern,
               base::Value(), old_rule.expiration, old_rule.session_model));
    }
  }

  // Notify brave cookie changes as ContentSettingsType::COOKIES
  if (initialized_ && (content_type == ContentSettingsType::BRAVE_COOKIES ||
                       content_type == ContentSettingsType::BRAVE_SHIELDS)) {
    // PostTask here to avoid content settings autolock DCHECK
    base::PostTask(
        FROM_HERE,
        {content::BrowserThread::UI, base::TaskPriority::USER_VISIBLE},
        base::BindOnce(&BravePrefProvider::NotifyChanges,
                       weak_factory_.GetWeakPtr(),
                       std::move(brave_cookie_updates), incognito));
  }
}

void BravePrefProvider::NotifyChanges(const std::vector<Rule>& rules,
                                      bool incognito) {
  for (const auto& rule : rules) {
    Notify(rule.primary_pattern, rule.secondary_pattern,
           ContentSettingsType::COOKIES);
  }
}

void BravePrefProvider::OnCookiePrefsChanged(
    const std::string& pref) {
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
