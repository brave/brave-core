// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_pref_manager.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/browser/channel_migrator.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace brave_news {

namespace {

base::Value NowAsValue() {
  return base::TimeToValue(base::Time::Now());
}

bool IsTombstone(const base::DictValue& entry) {
  return entry.FindBool(prefs::kBraveNewsKeyDeleted).value_or(false);
}

// Returns |entry| as a dictionary if it is a live (non-tombstone) entry.
const base::DictValue* GetLiveEntry(const base::Value* entry) {
  const auto* dict = entry ? entry->GetIfDict() : nullptr;
  return dict && !IsTombstone(*dict) ? dict : nullptr;
}

base::DictValue MakeTombstone() {
  return base::DictValue()
      .Set(prefs::kBraveNewsKeyDeleted, true)
      .Set(prefs::kBraveNewsKeyChangedAt, NowAsValue());
}

}  // namespace

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
  absl::flat_hash_set<std::string> disabled_publishers;
  absl::flat_hash_set<std::string> enabled_publishers;
  auto& subscriptions = prefs_->GetDict(prefs::kBraveNewsSources);
  for (const auto&& [publisher_id, entry] : subscriptions) {
    const auto* entry_dict = GetLiveEntry(&entry);
    if (!entry_dict) {
      continue;
    }
    auto enabled = entry_dict->FindBool(prefs::kBraveNewsSourcesKeyEnabled);
    if (!enabled) {
      continue;
    }
    if (*enabled) {
      enabled_publishers.insert(publisher_id);
    } else {
      disabled_publishers.insert(publisher_id);
    }
  }

  return SubscriptionsSnapshot(std::move(enabled_publishers),
                               std::move(disabled_publishers), GetDirectFeeds(),
                               GetChannels());
}

void BraveNewsPrefManager::SetPublisherSubscribed(
    const std::string& publisher_id,
    brave_news::mojom::UserEnabled enabled) {
  bool is_direct_feed = std::ranges::contains(
      GetDirectFeeds(), publisher_id,
      [](const auto& direct_feed) { return direct_feed.id; });

  if (is_direct_feed && enabled == mojom::UserEnabled::DISABLED) {
    ScopedDictPrefUpdate update(&*prefs_, prefs::kBraveNewsDirectFeeds);
    // Keep the source so the tombstone can be matched by URL.
    auto tombstone = MakeTombstone();
    if (const auto* source =
            update->FindDict(publisher_id)
                ->FindString(prefs::kBraveNewsDirectFeedsKeySource)) {
      tombstone.Set(prefs::kBraveNewsDirectFeedsKeySource, *source);
    }
    update->Set(publisher_id, std::move(tombstone));
  } else if (!is_direct_feed) {
    ScopedDictPrefUpdate update(&*prefs_, prefs::kBraveNewsSources);
    if (enabled == mojom::UserEnabled::NOT_MODIFIED) {
      if (GetLiveEntry(update->Find(publisher_id))) {
        update->Set(publisher_id, MakeTombstone());
      }
    } else {
      update->Set(publisher_id,
                  base::DictValue()
                      .Set(prefs::kBraveNewsSourcesKeyEnabled,
                           enabled == mojom::UserEnabled::ENABLED)
                      .Set(prefs::kBraveNewsKeyChangedAt, NowAsValue()));
    }
  }
}

std::string BraveNewsPrefManager::AddDirectPublisher(const GURL& url,
                                                     const std::string& title) {
  const auto& direct_feeds = GetDirectFeeds();
  auto match = std::ranges::find_if(
      direct_feeds, [url](const auto& feed) { return feed.url == url; });
  if (match != direct_feeds.end()) {
    return match->id;
  }

  // If the user previously unsubscribed from this feed, revive the tombstone
  // rather than creating a new entry.
  std::string entry_id;
  for (const auto&& [id, entry] :
       prefs_->GetDict(prefs::kBraveNewsDirectFeeds)) {
    const auto* entry_dict = entry.GetIfDict();
    if (!entry_dict || !IsTombstone(*entry_dict)) {
      continue;
    }
    const auto* source =
        entry_dict->FindString(prefs::kBraveNewsDirectFeedsKeySource);
    if (source && GURL(*source) == url) {
      entry_id = id;
      break;
    }
  }

  if (entry_id.empty()) {
    // UUID for each entry as feed url might change via redirects etc
    entry_id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  }
  std::string entry_title = title.empty() ? url.spec() : title;

  ScopedDictPrefUpdate update(&*prefs_, prefs::kBraveNewsDirectFeeds);
  base::DictValue value;
  value.Set(prefs::kBraveNewsDirectFeedsKeySource, url.spec());
  value.Set(prefs::kBraveNewsDirectFeedsKeyTitle, entry_title);
  value.Set(prefs::kBraveNewsKeyChangedAt, NowAsValue());
  update->SetByDottedPath(entry_id, std::move(value));

  return entry_id;
}

void BraveNewsPrefManager::SetChannelSubscribed(const std::string& locale,
                                                const std::string& channel,
                                                bool subscribed) {
  ScopedDictPrefUpdate update(&*prefs_, prefs::kBraveNewsChannels);
  auto* dict = update->EnsureDict(locale);
  if (!subscribed) {
    if (GetLiveEntry(dict->Find(channel))) {
      dict->Set(channel, MakeTombstone());
    }
  } else {
    dict->Set(channel, base::DictValue()
                           .Set(prefs::kBraveNewsChannelsKeySubscribed, true)
                           .Set(prefs::kBraveNewsKeyChangedAt, NowAsValue()));
  }
}

std::vector<DirectFeed> BraveNewsPrefManager::GetDirectFeeds() {
  std::vector<DirectFeed> result;
  const auto& direct_feeds = prefs_->GetDict(prefs::kBraveNewsDirectFeeds);
  for (const auto&& [key, value] : direct_feeds) {
    const auto* entry = GetLiveEntry(&value);
    if (!entry) {
      continue;
    }
    const auto* source =
        entry->FindString(prefs::kBraveNewsDirectFeedsKeySource);
    const auto* title = entry->FindString(prefs::kBraveNewsDirectFeedsKeyTitle);
    if (!source || !title) {
      continue;
    }

    result.push_back({
        .id = key,
        .url = GURL(*source),
        .title = *title,
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
    const auto* entries = channels.GetIfDict();
    if (!entries) {
      continue;
    }

    std::vector<std::string> subscribed;
    for (const auto&& [channel, entry] : *entries) {
      const auto* entry_dict = GetLiveEntry(&entry);
      if (entry_dict &&
          entry_dict->FindBool(prefs::kBraveNewsChannelsKeySubscribed)
              .value_or(false)) {
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
