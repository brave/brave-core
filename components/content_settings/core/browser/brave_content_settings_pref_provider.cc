/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/optional.h"
#include "base/task/post_task.h"
#include "brave/common/network_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "components/content_settings/core/browser/content_settings_pref.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace content_settings {

namespace {

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

  return Rule(primary_pattern,
              secondary_pattern,
              rule.value.Clone());
}

class BraveShieldsRuleIterator : public RuleIterator {
 public:
  BraveShieldsRuleIterator(std::vector<Rule>::const_iterator iterator,
                           std::vector<Rule>::const_iterator end)
      : iterator_(iterator),
        end_(end) {}

  bool HasNext() const override {
    return iterator_ != end_;
  }

  Rule Next() override {
    return CloneRule(*(iterator_++));
  }

 private:
  std::vector<Rule>::const_iterator iterator_;
  std::vector<Rule>::const_iterator end_;

  DISALLOW_COPY_AND_ASSIGN(BraveShieldsRuleIterator);
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

BravePrefProvider::BravePrefProvider(PrefService* prefs,
                                     bool off_the_record,
                                     bool store_last_modified)
    : PrefProvider(prefs, off_the_record, store_last_modified),
      weak_factory_(this) {
  brave_pref_change_registrar_.Init(prefs_);
  brave_pref_change_registrar_.Add(
      kGoogleLoginControlType,
      base::BindRepeating(&BravePrefProvider::OnCookiePrefsChanged,
                          base::Unretained(this)));

  WebsiteSettingsRegistry* website_settings =
      WebsiteSettingsRegistry::GetInstance();
  // Makes BravePrefProvder handle plugin type.
  for (const WebsiteSettingsInfo* info : *website_settings) {
    if (info->type() == ContentSettingsType::PLUGINS) {
      content_settings_prefs_.insert(std::make_pair(
          info->type(),
          std::make_unique<ContentSettingsPref>(
              info->type(), prefs_, &brave_pref_change_registrar_,
              info->pref_name(), off_the_record_,
              base::Bind(&PrefProvider::Notify, base::Unretained(this)))));
      break;
    }
  }

  MigrateShieldsSettings(off_the_record);

  AddObserver(this);
  OnCookieSettingsChanged(ContentSettingsType::PLUGINS);
}

BravePrefProvider::~BravePrefProvider() {}

void BravePrefProvider::ShutdownOnUIThread() {
  RemoveObserver(this);
  brave_pref_change_registrar_.RemoveAll();
  PrefProvider::ShutdownOnUIThread();
}

// static
void BravePrefProvider::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  PrefProvider::RegisterProfilePrefs(registry);
  // Register shields settings migration pref.
  registry->RegisterIntegerPref(kBraveShieldsSettingsVersion, 1);
}

void BravePrefProvider::MigrateShieldsSettings(bool incognito) {
  // Incognito inherits from regular profile, so nothing to do.
  // Guest doesn't inherit, but only keeps settings for the duration of the
  // session, so also nothing to do.
  if (incognito)
    return;
  MigrateShieldsSettingsV1ToV2();
}

void BravePrefProvider::MigrateShieldsSettingsV1ToV2() {
  // Check if migration is needed.
  if (prefs_->GetInteger(kBraveShieldsSettingsVersion) != 1)
    return;

  // All sources in ContentSettingsType::PLUGINS we want to migrate.
  for (const auto& resource_id : GetShieldsResourceIDs()) {
    MigrateShieldsSettingsV1ToV2ForOneType(ContentSettingsType::PLUGINS,
                                           resource_id);
  }

  // ContentSettingsType::JAVASCRIPT.
  MigrateShieldsSettingsV1ToV2ForOneType(ContentSettingsType::JAVASCRIPT,
                                         std::string());

  // Mark migration as done.
  prefs_->SetInteger(kBraveShieldsSettingsVersion, 2);
}

void BravePrefProvider::MigrateShieldsSettingsV1ToV2ForOneType(
    ContentSettingsType content_type,
    const std::string& resource_id) {
  using OldRule = std::pair<ContentSettingsPattern, ContentSettingsPattern>;
  // Find rules that can be migrated and create replacement rules for them.
  std::vector<OldRule> old_rules;
  std::vector<Rule> new_rules;
  auto rule_iterator = PrefProvider::GetRuleIterator(content_type, resource_id,
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
          rule.value.Clone());
    }
  }
  rule_iterator.reset();

  // Migrate.
  DCHECK_EQ(old_rules.size(), new_rules.size());
  for (size_t i = 0; i < old_rules.size(); i++) {
    // Remove current setting.
    PrefProvider::SetWebsiteSetting(
        old_rules[i].first, old_rules[i].second, content_type, resource_id,
        ContentSettingToValue(CONTENT_SETTING_DEFAULT));
    // Add new setting.
    PrefProvider::SetWebsiteSetting(
        new_rules[i].primary_pattern, new_rules[i].secondary_pattern,
        content_type, resource_id,
        ContentSettingToValue(ValueToContentSetting(&(new_rules[i].value))));
  }
}

bool BravePrefProvider::SetWebsiteSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    std::unique_ptr<base::Value>&& in_value) {
  // Flash's setting shouldn't be reached here.
  // Its content type is plugin and id is empty string.
  // One excpetion is default setting. It can be persisted.
  if (content_type == ContentSettingsType::PLUGINS &&
      resource_identifier.empty()) {
    DCHECK(primary_pattern == ContentSettingsPattern::Wildcard() &&
           secondary_pattern == ContentSettingsPattern::Wildcard());
  }

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

      // change to type PLUGINS
      return PrefProvider::SetWebsiteSetting(plugin_primary_pattern,
                                             plugin_secondary_pattern,
                                             ContentSettingsType::PLUGINS,
                                             brave_shields::kCookies,
                                             std::move(in_value));
    }
  }

  return PrefProvider::SetWebsiteSetting(primary_pattern, secondary_pattern,
                                         content_type, resource_identifier,
                                         std::move(in_value));
}

std::unique_ptr<RuleIterator> BravePrefProvider::GetRuleIterator(
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier,
      bool incognito) const {
  if (content_type == ContentSettingsType::COOKIES) {
    return std::make_unique<BraveShieldsRuleIterator>(
        cookie_rules_.at(incognito).begin(),
        cookie_rules_.at(incognito).end());
  }

  // Early return. We don't store flash plugin setting in preference.
  if (content_type == ContentSettingsType::PLUGINS &&
      resource_identifier == "")
    return nullptr;

  return PrefProvider::GetRuleIterator(content_type,
                                       resource_identifier,
                                       incognito);
}

void BravePrefProvider::UpdateCookieRules(ContentSettingsType content_type,
                                          bool incognito) {
  auto& rules = cookie_rules_[incognito];
  auto old_rules = std::move(brave_cookie_rules_[incognito]);

  rules.clear();

  // kGoogleLoginControlType preference adds an exception for
  // accounts.google.com to access cookies in 3p context to allow login using
  // google oauth. The exception is added before all overrides to allow google
  // oauth to work when the user sets custom overrides for a site.
  // For example: Google OAuth will be allowed if the user allows all cookies
  // and sets 3p cookie blocking for a site.
  if (prefs_->GetBoolean(kGoogleLoginControlType)) {
      auto rule = Rule(ContentSettingsPattern::FromString(kGoogleOAuthPattern),
                       ContentSettingsPattern::Wildcard(),
                       ContentSettingToValue(CONTENT_SETTING_ALLOW)->Clone());
      rules.push_back(CloneRule(rule));
      brave_cookie_rules_[incognito].push_back(CloneRule(rule));
  }
  // non-pref based exceptions should go in the cookie_settings_base.cc
  // chromium_src override

  // add chromium cookies
  auto chromium_cookies_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::COOKIES,
      "",
      incognito);
  while (chromium_cookies_iterator && chromium_cookies_iterator->HasNext()) {
    rules.push_back(CloneRule(chromium_cookies_iterator->Next()));
  }
  chromium_cookies_iterator.reset();

  auto brave_shields_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::PLUGINS,
      brave_shields::kBraveShields,
      incognito);

  // collect shield rules
  std::vector<Rule> shield_rules;
  while (brave_shields_iterator && brave_shields_iterator->HasNext()) {
    shield_rules.push_back(CloneRule(brave_shields_iterator->Next()));
  }

  brave_shields_iterator.reset();

  // add brave cookies after checking shield status
  auto brave_cookies_iterator = PrefProvider::GetRuleIterator(
      ContentSettingsType::PLUGINS,
      brave_shields::kCookies,
      incognito);

  // Matching cookie rules against shield rules.
  while (brave_cookies_iterator && brave_cookies_iterator->HasNext()) {
    auto rule = brave_cookies_iterator->Next();
    if (IsActive(rule, shield_rules)) {
      rules.push_back(CloneRule(rule, true));
      brave_cookie_rules_[incognito].push_back(CloneRule(rule, true));
    }
  }

  // Adding shields down rules (they always override cookie rules).
  for (const auto& shield_rule : shield_rules) {
    // There is no global shields rule
    if (shield_rule.primary_pattern.MatchesAllHosts())
      NOTREACHED();

    // Shields down.
    if (ValueToContentSetting(&shield_rule.value) == CONTENT_SETTING_BLOCK) {
      rules.push_back(
          Rule(ContentSettingsPattern::Wildcard(),
               shield_rule.primary_pattern,
               ContentSettingToValue(CONTENT_SETTING_ALLOW)->Clone()));
      brave_cookie_rules_[incognito].push_back(
          Rule(ContentSettingsPattern::Wildcard(),
               shield_rule.primary_pattern,
               ContentSettingToValue(CONTENT_SETTING_ALLOW)->Clone()));
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
      brave_cookie_updates.push_back(CloneRule(new_rule));
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
      brave_cookie_updates.push_back(
          Rule(old_rule.primary_pattern,
               old_rule.secondary_pattern,
               base::Value()));
    }
  }

  // Notify brave cookie changes as ContentSettingsType::COOKIES
  if (content_type == ContentSettingsType::PLUGINS) {
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
    Notify(rule.primary_pattern,
           rule.secondary_pattern,
           ContentSettingsType::COOKIES,
           "");
  }
}

void BravePrefProvider::OnCookiePrefsChanged(
    const std::string& pref) {
  OnCookieSettingsChanged(ContentSettingsType::PLUGINS);
}

void BravePrefProvider::OnCookieSettingsChanged(
    ContentSettingsType content_type) {
  UpdateCookieRules(content_type, true);
  UpdateCookieRules(content_type, false);
}

void BravePrefProvider::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const std::string& resource_identifier) {
  if (content_type == ContentSettingsType::COOKIES ||
      (content_type == ContentSettingsType::PLUGINS &&
          (resource_identifier == brave_shields::kCookies ||
           resource_identifier == brave_shields::kBraveShields))) {
    OnCookieSettingsChanged(content_type);
  }
}

}  // namespace content_settings
