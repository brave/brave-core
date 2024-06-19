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

// Represents a DirectFeed that the user has subscribed to. These are store
// differently to normal publishers, as we need to store the URL and title of
// the feed, rather than just an Id.
struct DirectFeed {
  std::string id;
  GURL url;
  std::string title;
};

// Represents a change in the set of subscriptions. This is used to generate
// change notifications for the front end (probably Java or JavaScript).
struct SubscriptionsDiff {
  // The added or updated subscriptions.
  std::vector<std::string> changed;
  // The removed subscriptions.
  std::vector<std::string> removed;

  SubscriptionsDiff();
  SubscriptionsDiff(const SubscriptionsDiff&) = delete;
  SubscriptionsDiff& operator=(const SubscriptionsDiff&) = delete;
  SubscriptionsDiff(SubscriptionsDiff&&);
  SubscriptionsDiff& operator=(SubscriptionsDiff&&);
  ~SubscriptionsDiff();
};

// A snapshot of the Brave News subscriptions at a point in time. Useful for
// posting work to a background thread. All methods on this class refer to the
// point in time the snapshot was made.
class BraveNewsSubscriptions {
 public:
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

  // Get all the locales that the user has subscribed to channels in.
  std::vector<std::string> GetChannelLocales() const;
  // Get all the locales that the user is subscribed to |channel| in.
  std::vector<std::string> GetChannelLocales(const std::string& channel) const;
  // Determine whether the user is subscribed to |channel| in |locale|.
  bool GetChannelSubscribed(const std::string& locale,
                            const std::string& channel) const;

  // Get the changes to the publisher subscriptions between two snapshots.
  // Useful for notifying the front end of publisher changes.
  SubscriptionsDiff DiffPublishers(const BraveNewsSubscriptions& old) const;
  // Get the changes to the channel subscriptions between two snapshots. Useful
  // for notifying the front end of channel changes.
  SubscriptionsDiff DiffChannels(const BraveNewsSubscriptions& old) const;

  // List of enabled publisher_ids
  const base::flat_set<std::string>& enabled_publishers() const {
    return enabled_publishers_;
  }

  // List of disabled publisher_ids
  const base::flat_set<std::string>& disabled_publishers() const {
    return disabled_publishers_;
  }

  // All subscribed DirectFeeds. Direct feeds are deleted when they're
  // unsubscribed from.
  const std::vector<DirectFeed>& direct_feeds() const { return direct_feeds_; }

  // A map of |locale ==> channels[]| representing the channels subscribed to in
  // different locales.
  const base::flat_map<std::string, std::vector<std::string>> channels() const {
    return channels_;
  }

 private:
  base::flat_set<std::string> enabled_publishers_;
  base::flat_set<std::string> disabled_publishers_;
  std::vector<DirectFeed> direct_feeds_;
  base::flat_map<std::string, std::vector<std::string>> channels_;
};

// Helper class providing a consistent interface for interacting with Brave News
// storage and provides utilities for change notifications.
// Currently this backs onto PrefService.
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

  // Get everything teh user is subscribed to.
  BraveNewsSubscriptions GetSubscriptions();

  // Enables/disables/resets a publisher. When a direct feed is set to a
  // non-enabled state it is deleted.
  void SetPublisherSubscribed(const std::string& publisher_id,
                              brave_news::mojom::UserEnabled enabled);

  // Adds a new entry for a DirectFeed. Direct feeds have a separate entry point
  // for adding new entries because we need to record the |url| and |title| of
  // the feed in order to retrieve it.
  std::string AddDirectPublisher(const GURL& url, const std::string& title);

  // Handles managing subscription to a channel in a locale.
  void SetChannelSubscribed(const std::string& locale,
                            const std::string& channel,
                            bool subscribed);

  // Clears all Brave News related preferences.
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
