// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_pref_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/uuid.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/browser/channel_migrator.h"
#include "brave/components/brave_news/browser/locales_helper.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_observer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"

namespace brave_news {

// static
void BraveNewsPrefManager::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kShouldShowToolbarButton, true);
  registry->RegisterBooleanPref(prefs::kNewTabPageShowToday,
                                IsUserInDefaultEnabledLocale());
  registry->RegisterBooleanPref(prefs::kBraveNewsOptedIn, false);
  registry->RegisterDictionaryPref(prefs::kBraveNewsSources);
  registry->RegisterDictionaryPref(prefs::kBraveNewsChannels);
  registry->RegisterDictionaryPref(prefs::kBraveNewsDirectFeeds);
  registry->RegisterBooleanPref(prefs::kBraveNewsOpenArticlesInNewTab, true);

  p3a::NewsMetrics::RegisterProfilePrefs(registry);
}

BraveNewsPrefManager::BraveNewsPrefManager(PrefService& prefs) : prefs_(prefs) {
  pref_change_registrar_.Init(&*prefs_);
  pref_change_registrar_.Add(
      prefs::kBraveNewsChannels,
      base::BindRepeating(&BraveNewsPrefManager::NotifyChannelsChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveNewsDirectFeeds,
      base::BindRepeating(&BraveNewsPrefManager::NotifyPublishersChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveNewsSources,
      base::BindRepeating(&BraveNewsPrefManager::NotifyPublishersChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveNewsOptedIn,
      base::BindRepeating(&BraveNewsPrefManager::NotifyConfigChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kNewTabPageShowToday,
      base::BindRepeating(&BraveNewsPrefManager::NotifyConfigChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveNewsOpenArticlesInNewTab,
      base::BindRepeating(&BraveNewsPrefManager::NotifyConfigChanged,
                          base::Unretained(this)));
}

BraveNewsPrefManager::~BraveNewsPrefManager() = default;

void BraveNewsPrefManager::AddObserver(PrefObserver* observer) {
  observers_.AddObserver(observer);
}

void BraveNewsPrefManager::RemoveObserver(PrefObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool BraveNewsPrefManager::IsEnabled() {
  return brave_news::IsEnabled(&*prefs_);
}

brave_news::mojom::ConfigurationPtr BraveNewsPrefManager::GetConfig() {
  auto result = brave_news::mojom::Configuration::New();
  result->isOptedIn = prefs_->GetBoolean(prefs::kBraveNewsOptedIn);
  result->showOnNTP = prefs_->GetBoolean(prefs::kNewTabPageShowToday);
  result->openArticlesInNewTab =
      prefs_->GetBoolean(prefs::kBraveNewsOpenArticlesInNewTab);

  return result;
}

void BraveNewsPrefManager::SetConfig(
    brave_news::mojom::ConfigurationPtr config) {
  prefs_->SetBoolean(prefs::kBraveNewsOptedIn, config->isOptedIn);
  prefs_->SetBoolean(prefs::kNewTabPageShowToday, config->showOnNTP);
  prefs_->SetBoolean(prefs::kBraveNewsOpenArticlesInNewTab,
                     config->openArticlesInNewTab);
}

SubscriptionsSnapshot BraveNewsPrefManager::GetSubscriptions() {
  std::vector<std::string> disabled_publishers;
  std::vector<std::string> enabled_publishers;
  auto& subscriptions = prefs_->GetDict(prefs::kBraveNewsSources);
  for (const auto&& [publisher_id, subscribed] : subscriptions) {
    if (subscribed.GetBool()) {
      enabled_publishers.push_back(publisher_id);
    } else {
      disabled_publishers.push_back(publisher_id);
    }
  }

  return SubscriptionsSnapshot(std::move(enabled_publishers),
                                std::move(disabled_publishers),
                                GetDirectFeeds(), GetChannels());
}

void BraveNewsPrefManager::SetPublisherSubscribed(
    const std::string& publisher_id,
    brave_news::mojom::UserEnabled enabled) {
  bool is_direct_feed =
      base::Contains(GetDirectFeeds(), publisher_id,
                     [](const auto& direct_feed) { return direct_feed.id; });

  if (is_direct_feed && enabled == mojom::UserEnabled::DISABLED) {
    ScopedDictPrefUpdate update(&*prefs_, prefs::kBraveNewsDirectFeeds);
    update->Remove(publisher_id);
  } else if (!is_direct_feed) {
    ScopedDictPrefUpdate update(&*prefs_, prefs::kBraveNewsSources);
    if (enabled == mojom::UserEnabled::NOT_MODIFIED) {
      update->Remove(publisher_id);
    } else {
      update->Set(publisher_id, enabled == mojom::UserEnabled::ENABLED);
    }
  }
}

std::string BraveNewsPrefManager::AddDirectPublisher(const GURL& url,
                                                     const std::string& title) {
  const auto& direct_feeds = GetDirectFeeds();
  auto match = base::ranges::find_if(
      direct_feeds, [url](const auto& feed) { return feed.url == url; });
  if (match != direct_feeds.end()) {
    return match->id;
  }

  // UUID for each entry as feed url might change via redirects etc
  auto entry_id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  std::string entry_title = title.empty() ? url.spec() : title;

  // We use a dictionary pref, but that's to reserve space for more
  // future customization on a feed. For now we just store a bool, and
  // remove the entire entry if a user unsubscribes from a user feed.
  ScopedDictPrefUpdate update(&*prefs_, prefs::kBraveNewsDirectFeeds);
  base::Value::Dict value;
  value.Set(prefs::kBraveNewsDirectFeedsKeySource, url.spec());
  value.Set(prefs::kBraveNewsDirectFeedsKeyTitle, entry_title);
  update->SetByDottedPath(entry_id, std::move(value));

  return entry_id;
}

void BraveNewsPrefManager::SetChannelSubscribed(const std::string& locale,
                                                const std::string& channel,
                                                bool subscribed) {
  ScopedDictPrefUpdate update(&*prefs_, prefs::kBraveNewsChannels);
  auto* dict = update->EnsureDict(locale);
  if (!subscribed) {
    dict->Remove(channel);
  } else {
    dict->Set(channel, true);
  }
}

std::vector<DirectFeed> BraveNewsPrefManager::GetDirectFeeds() {
  std::vector<DirectFeed> result;
  const auto& direct_feeds = prefs_->GetDict(prefs::kBraveNewsDirectFeeds);
  for (const auto&& [key, value] : direct_feeds) {
    // Non dict values will be flagged as an issue elsewhere.
    if (!value.is_dict()) {
      continue;
    }

    result.push_back({
        .id = key,
        .url = GURL(
            *value.GetDict().FindString(prefs::kBraveNewsDirectFeedsKeySource)),
        .title =
            *value.GetDict().FindString(prefs::kBraveNewsDirectFeedsKeyTitle),
    });
  }
  return result;
}

void BraveNewsPrefManager::ClearPrefs() {
  ScopedDictPrefUpdate update_channels(&*prefs_, prefs::kBraveNewsChannels);
  update_channels->clear();

  ScopedDictPrefUpdate update_direct(&*prefs_, prefs::kBraveNewsDirectFeeds);
  update_direct->clear();

  ScopedDictPrefUpdate update_sources(&*prefs_, prefs::kBraveNewsSources);
  update_sources->clear();
}

base::flat_map<std::string, std::vector<std::string>>
BraveNewsPrefManager::GetChannels() {
  base::flat_map<std::string, std::vector<std::string>> result;

  auto& pref = prefs_->GetDict(prefs::kBraveNewsChannels);

  for (const auto&& [locale, channels] : pref) {
    std::vector<std::string> subscribed;

    auto& entries = channels.GetDict();
    for (const auto&& [channel, is_subscribed] : entries) {
      if (is_subscribed.GetIfBool().value_or(false)) {
        subscribed.push_back(GetMigratedChannel(channel));
      }
    }

    if (!subscribed.empty()) {
      result[locale] = std::move(subscribed);
    }
  }
  return result;
}

void BraveNewsPrefManager::NotifyChannelsChanged() {
  for (auto& obs : observers_) {
    obs.OnChannelsChanged();
  }
}

void BraveNewsPrefManager::NotifyPublishersChanged() {
  for (auto& obs : observers_) {
    obs.OnPublishersChanged();
  }
}

void BraveNewsPrefManager::NotifyConfigChanged() {
  for (auto& obs : observers_) {
    obs.OnConfigChanged();
  }
}

}  // namespace brave_news
