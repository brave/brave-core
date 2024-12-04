// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/scoped_observation.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/task/sequenced_task_runner.h"
#include "base/timer/timer.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/brave_news_engine.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/initialization_promise.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "brave/components/brave_private_cdn/private_cdn_request_helper.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefService;

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace favicon {
class FaviconService;
}

namespace history {
class HistoryService;
}  // namespace history

namespace brave_news {

// Browser-side handler for Brave News mojom API, 1 per profile
// Orchestrates FeedController and PublishersController for data, as well as
// owning prefs data.
// Controls remote feed update logic via Timer and prefs values.
class BraveNewsController
    : public KeyedService,
      public mojom::BraveNewsController,
      public mojom::BraveNewsInternals,
      public net::NetworkChangeNotifier::NetworkChangeObserver,
      public BraveNewsPrefManager::PrefObserver {
 public:
  BraveNewsController(
      PrefService* prefs,
      favicon::FaviconService* favicon_service,
      brave_ads::AdsService* ads_service,
      history::HistoryService* history_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<DirectFeedFetcher::Delegate>
          direct_feed_fetcher_delegate);
  ~BraveNewsController() override;
  BraveNewsController(const BraveNewsController&) = delete;
  BraveNewsController& operator=(const BraveNewsController&) = delete;

  mojo::PendingRemote<mojom::BraveNewsController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveNewsController> receiver);
  void Bind(mojo::PendingReceiver<mojom::BraveNewsInternals> receiver);

  // Remove any cache that would identify user browsing history
  void ClearHistory();

  BraveNewsPrefManager& prefs() { return pref_manager_; }

  void GetPublisherForSite(const GURL& site_url, GetPublisherCallback callback);
  void GetPublisherForFeed(const GURL& feed_url, GetPublisherCallback callback);

  // mojom::BraveNewsController
  void GetLocale(GetLocaleCallback callback) override;
  void GetFeed(GetFeedCallback callback) override;
  void GetFollowingFeed(GetFollowingFeedCallback callback) override;
  void GetChannelFeed(const std::string& channel,
                      GetChannelFeedCallback callback) override;
  void GetPublisherFeed(const std::string& publisher_id,
                        GetPublisherFeedCallback callback) override;
  void EnsureFeedV2IsUpdating() override;
  void GetFeedV2(GetFeedV2Callback callback) override;
  void GetSignals(GetSignalsCallback callback) override;
  void GetPublishers(GetPublishersCallback callback) override;
  void AddPublishersListener(
      mojo::PendingRemote<mojom::PublishersListener> listener) override;
  void GetSuggestedPublisherIds(
      GetSuggestedPublisherIdsCallback callback) override;
  void FindFeeds(const GURL& possible_feed_or_site_url,
                 FindFeedsCallback callback) override;
  void GetChannels(GetChannelsCallback callback) override;
  void AddChannelsListener(
      mojo::PendingRemote<mojom::ChannelsListener> listener) override;
  void SetChannelSubscribed(const std::string& locale,
                            const std::string& channel_id,
                            bool subscribed,
                            SetChannelSubscribedCallback callback) override;
  void SubscribeToNewDirectFeed(
      const GURL& feed_url,
      SubscribeToNewDirectFeedCallback callback) override;
  void RemoveDirectFeed(const std::string& publisher_id) override;
  void GetImageData(const GURL& padded_image_url,
                    GetImageDataCallback callback) override;
  void GetFavIconData(const std::string& publisher_id,
                      GetFavIconDataCallback callback) override;
  void SetPublisherPref(const std::string& publisher_id,
                        mojom::UserEnabled new_status) override;
  void ClearPrefs() override;
  void IsFeedUpdateAvailable(const std::string& displayed_feed_hash,
                             IsFeedUpdateAvailableCallback callback) override;
  void AddFeedListener(
      mojo::PendingRemote<mojom::FeedListener> listener) override;
  void SetConfiguration(mojom::ConfigurationPtr configuration,
                        SetConfigurationCallback callback) override;
  void AddConfigurationListener(
      mojo::PendingRemote<mojom::ConfigurationListener> listener) override;
  void GetDisplayAd(GetDisplayAdCallback callback) override;
  void OnInteractionSessionStarted() override;

  void OnNewCardsViewed(uint16_t card_views) override;
  void OnCardVisited(uint32_t depth) override;
  void OnSidebarFilterUsage() override;

  void OnPromotedItemView(const std::string& item_id,
                          const std::string& creative_instance_id) override;
  void OnPromotedItemVisit(const std::string& item_id,
                           const std::string& creative_instance_id) override;
  void OnDisplayAdVisit(const std::string& item_id,
                        const std::string& creative_instance_id) override;
  void OnDisplayAdView(const std::string& item_id,
                       const std::string& creative_instance_id) override;

  // mojom::BraveNewsInternals
  void GetVisitedSites(GetVisitedSitesCallback callback) override;

  // net::NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // BraveNewsPrefManager::PrefsObserver:
  void OnConfigChanged() override;
  void OnPublishersChanged() override;
  void OnChannelsChanged() override;

 private:
  void ResetEngine();
  void ConditionallyStartOrStopTimer();
  void CheckForFeedsUpdate();
  void CheckForPublishersUpdate();
  void Prefetch();
  void MaybeInitPrefs();
  void OnInitializingPrefsComplete();
  void OnVerifiedDirectFeedUrl(const GURL& feed_url,
                               SubscribeToNewDirectFeedCallback callback,
                               bool is_valid,
                               const std::string& feed_title);

  void NotifyPublishersChanged(mojom::PublishersEventPtr event);
  void NotifyChannelsChanged(mojom::ChannelsEventPtr event);
  void NotifyFeedChanged(const std::string& hash);

  BackgroundHistoryQuerier MakeHistoryQuerier();

  raw_ptr<favicon::FaviconService> favicon_service_ = nullptr;
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;
  api_request_helper::APIRequestHelper api_request_helper_;
  brave_private_cdn::PrivateCDNRequestHelper private_cdn_request_helper_;
  raw_ptr<history::HistoryService> history_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::unique_ptr<DirectFeedFetcher::Delegate, base::OnTaskRunnerDeleter>
      direct_feed_fetcher_delegate_;

  BraveNewsPrefManager pref_manager_;
  SubscriptionsSnapshot last_subscriptions_;

  p3a::NewsMetrics news_metrics_;

  DirectFeedController direct_feed_controller_;

  // Created on this sequence but lives on |task_runner_|.
  std::unique_ptr<BraveNewsEngine, base::OnTaskRunnerDeleter> engine_;

  base::OneShotTimer timer_prefetch_;
  base::RepeatingTimer timer_feed_update_;
  base::RepeatingTimer timer_publishers_update_;
  // Subscribe to this to know when the initial initializing (and seeding) of
  // preferences is ensured. This happens after the feature is (re-)enabled
  // and after locales and sources are fetched. A valid feed and list of
  // channels might only be available after this event has fired. If News is
  // already enabled and this event has already signalled, then they are already
  // available.
  InitializationPromise initialization_promise_;
  base::CancelableTaskTracker task_tracker_;

  base::ScopedObservation<BraveNewsPrefManager,
                          BraveNewsPrefManager::PrefObserver>
      prefs_observation_{this};
  mojo::ReceiverSet<mojom::BraveNewsController> receivers_;
  mojo::ReceiverSet<mojom::BraveNewsInternals> internals_receivers_;
  mojo::RemoteSet<mojom::PublishersListener> publishers_listeners_;
  mojo::RemoteSet<mojom::ChannelsListener> channels_listeners_;
  mojo::RemoteSet<mojom::FeedListener> feed_listeners_;
  mojo::RemoteSet<mojom::ConfigurationListener> configuration_listeners_;
  base::WeakPtrFactory<BraveNewsController> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_CONTROLLER_H_
