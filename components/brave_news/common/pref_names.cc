// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/pref_names.h"

#include <string_view>

#include "base/json/values_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_news/common/locales_helper.h"
#include "brave/components/brave_news/common/p3a_pref_names.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_news {
namespace prefs {

namespace {

bool HasLegacyBoolEntries(const base::DictValue& dict) {
  for (const auto [key, value] : dict) {
    if (value.is_bool()) {
      return true;
    }
  }
  return false;
}

// Converts `{key: bool}` entries into `{key: {<value_key>: bool, changed_at}}`.
void MigrateLegacyBoolEntries(base::DictValue& dict,
                              std::string_view value_key,
                              const base::Value& changed_at) {
  for (auto [key, value] : dict) {
    if (!value.is_bool()) {
      continue;
    }
    value = base::Value(base::DictValue()
                            .Set(value_key, value.GetBool())
                            .Set(kBraveNewsKeyChangedAt, changed_at.Clone()));
  }
}

void MigrateSources(PrefService& prefs, const base::Value& changed_at) {
  if (!HasLegacyBoolEntries(prefs.GetDict(kBraveNewsSources))) {
    return;
  }
  ScopedDictPrefUpdate update(&prefs, kBraveNewsSources);
  MigrateLegacyBoolEntries(update.Get(), kBraveNewsSourcesKeyEnabled,
                           changed_at);
}

void MigrateChannels(PrefService& prefs, const base::Value& changed_at) {
  bool needs_migration = false;
  for (const auto [locale, channels] : prefs.GetDict(kBraveNewsChannels)) {
    if (channels.is_dict() && HasLegacyBoolEntries(channels.GetDict())) {
      needs_migration = true;
      break;
    }
  }
  if (!needs_migration) {
    return;
  }

  ScopedDictPrefUpdate update(&prefs, kBraveNewsChannels);
  for (auto [locale, channels] : update.Get()) {
    if (channels.is_dict()) {
      MigrateLegacyBoolEntries(channels.GetDict(),
                               kBraveNewsChannelsKeySubscribed, changed_at);
    }
  }
}

void MigrateDirectFeeds(PrefService& prefs, const base::Value& changed_at) {
  auto missing_changed_at = [](const base::Value& feed) {
    return feed.is_dict() && !feed.GetDict().contains(kBraveNewsKeyChangedAt);
  };

  bool needs_migration = false;
  for (const auto [id, feed] : prefs.GetDict(kBraveNewsDirectFeeds)) {
    if (missing_changed_at(feed)) {
      needs_migration = true;
      break;
    }
  }
  if (!needs_migration) {
    return;
  }

  ScopedDictPrefUpdate update(&prefs, kBraveNewsDirectFeeds);
  for (auto [id, feed] : update.Get()) {
    if (missing_changed_at(feed)) {
      feed.GetDict().Set(kBraveNewsKeyChangedAt, changed_at.Clone());
    }
  }
}

}  // namespace

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kShouldShowToolbarButton, true);
  registry->RegisterBooleanPref(kNewTabPageShowToday,
                                IsUserInDefaultEnabledLocale());
  registry->RegisterBooleanPref(kBraveNewsOptedIn, false);
  registry->RegisterBooleanPref(kBraveNewsOptInTrial, false);
  registry->RegisterDictionaryPref(kBraveNewsSources);
  registry->RegisterDictionaryPref(kBraveNewsChannels);
  registry->RegisterDictionaryPref(kBraveNewsDirectFeeds);
  registry->RegisterBooleanPref(kBraveNewsOpenArticlesInNewTab, true);
  registry->RegisterBooleanPref(kBraveNewsDisabledByPolicy, false);

  brave_news::p3a::prefs::RegisterProfileNewsMetricsPrefs(registry);
}

void MigrateProfilePrefs(PrefService* prefs) {
  // Added 2026-11: record `changed_at` on subscription entries.
  const base::Value changed_at = base::TimeToValue(base::Time::Now());
  MigrateSources(*prefs, changed_at);
  MigrateChannels(*prefs, changed_at);
  MigrateDirectFeeds(*prefs, changed_at);
}

}  // namespace prefs

bool IsEnabled(PrefService* prefs) {
  if (prefs->IsManagedPreference(prefs::kBraveNewsDisabledByPolicy) &&
      prefs->GetBoolean(prefs::kBraveNewsDisabledByPolicy)) {
    return false;
  }
  return prefs->GetBoolean(prefs::kNewTabPageShowToday) &&
         prefs->GetBoolean(prefs::kBraveNewsOptedIn);
}

}  // namespace brave_news
