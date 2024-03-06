// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_PREF_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_PREF_MANAGER_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_observer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_news {

bool GetIsEnabled(PrefService* prefs);

struct DirectFeed {
  std::string id;
  GURL url;
  std::string title;
};

struct SubscriptionsDiff {
  std::vector<std::string> changed;
  std::vector<std::string> removed;

  SubscriptionsDiff();
  SubscriptionsDiff(const SubscriptionsDiff&) = delete;
  SubscriptionsDiff& operator=(const SubscriptionsDiff&) = delete;
  SubscriptionsDiff(SubscriptionsDiff&&);
  SubscriptionsDiff& operator=(SubscriptionsDiff&&);
  ~SubscriptionsDiff();
};

struct BraveNewsSubscriptions {
 public:
  base::flat_set<std::string> enabled_publishers;
  base::flat_set<std::string> disabled_publishers;
  std::vector<DirectFeed> direct_feeds;
  base::flat_map<std::string, std::vector<std::string>> channels;

  BraveNewsSubscriptions();
  BraveNewsSubscriptions(
      base::flat_set<std::string> enabled_publishers,
      base::flat_set<std::string> disabled_publishers,
      std::vector<DirectFeed> direct_feeds,
      base::flat_map<std::string, std::vector<std::string>> channels);
  BraveNewsSubscriptions(const BraveNewsSubscriptions&);
  BraveNewsSubscriptions& operator=(const BraveNewsSubscriptions&);
  BraveNewsSubscriptions(BraveNewsSubscriptions&&);
  BraveNewsSubscriptions& operator=(BraveNewsSubscriptions&&);
  ~BraveNewsSubscriptions();

  std::vector<std::string> GetChannelLocales() const;
  std::vector<std::string> GetChannelLocales(const std::string& channel) const;
  bool GetChannelSubscribed(const std::string& locale,
                            const std::string& channel) const;

  SubscriptionsDiff DiffPublishers(const BraveNewsSubscriptions& old) const;
  SubscriptionsDiff DiffChannels(const BraveNewsSubscriptions& old) const;
};

class BraveNewsPrefManager {
 public:
  class PrefObserver : public base::CheckedObserver {
   public:
    virtual void OnConfigChanged() {}
    virtual void OnPublishersChanged() {}
    virtual void OnChannelsChanged() {}
  };

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  explicit BraveNewsPrefManager(PrefService& prefs);
  ~BraveNewsPrefManager();

  void AddObserver(PrefObserver* observer);
  void RemoveObserver(PrefObserver* observer);

  bool IsEnabled();
  brave_news::mojom::ConfigurationPtr GetConfig();
  void SetConfig(brave_news::mojom::ConfigurationPtr config);

  BraveNewsSubscriptions GetSubscriptions();

  void SetPublisherSubscribed(const std::string& publisher_id,
                              brave_news::mojom::UserEnabled enabled);
  std::string AddDirectPublisher(const GURL& url, const std::string& title);

  void SetChannelSubscribed(const std::string& locale,
                            const std::string& channel,
                            bool subscribed);

  void ClearPrefs();

 private:
  std::vector<DirectFeed> GetDirectFeeds();
  base::flat_map<std::string, std::vector<std::string>> GetChannels();

  void NotifyPublishersChanged();
  void NotifyChannelsChanged();
  void NotifyConfigChanged();

  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<PrefObserver> observers_;

  raw_ref<PrefService> prefs_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_PREF_MANAGER_H_
