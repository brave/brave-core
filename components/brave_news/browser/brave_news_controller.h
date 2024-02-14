// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_CONTROLLER_H_

#include <cstddef>
#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/scoped_observation.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/feed_controller.h"
#include "brave/components/brave_news/browser/feed_v2_builder.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/suggestions_controller.h"
#include "brave/components/brave_news/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_private_cdn/private_cdn_request_helper.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefRegistrySimple;
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

bool GetIsEnabled(PrefService* prefs);

// Browser-side handler for Brave News mojom API, 1 per profile
// Orchestrates FeedController and PublishersController for data, as well as
// owning prefs data.
// Controls remote feed update logic via Timer and prefs values.
class BraveNewsController
    : public KeyedService,
      public mojom::BraveNewsController,
      public PublishersController::Observer,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  BraveNewsController(
      PrefService* prefs,
      favicon::FaviconService* favicon_service,
      brave_ads::AdsService* ads_service,
      history::HistoryService* history_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveNewsController() override;
  BraveNewsController(const BraveNewsController&) = delete;
  BraveNewsController& operator=(const BraveNewsController&) = delete;

  mojo::PendingRemote<mojom::BraveNewsController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveNewsController> receiver);

  // Remove any cache that would identify user browsing history
  void ClearHistory();

  PublishersController* publisher_controller() {
    return &publishers_controller_;
  }

  bool MaybeInitFeedV2();

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

  // PublishersController::Observer:
  void OnPublishersUpdated(brave_news::PublishersController*) override;

  // net::NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

 private:
  void OnOptInChange();
  void ConditionallyStartOrStopTimer();
  void CheckForFeedsUpdate();
  void CheckForPublishersUpdate();
  void HandleSubscriptionsChanged();
  void Prefetch();
  void MaybeInitPrefs();

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<favicon::FaviconService> favicon_service_ = nullptr;
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;
  api_request_helper::APIRequestHelper api_request_helper_;
  brave_private_cdn::PrivateCDNRequestHelper private_cdn_request_helper_;
  raw_ptr<history::HistoryService> history_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  p3a::NewsMetrics news_metrics_;

  DirectFeedController direct_feed_controller_;
  UnsupportedPublisherMigrator unsupported_publisher_migrator_;
  PublishersController publishers_controller_;
  ChannelsController channels_controller_;
  FeedController feed_controller_;
  SuggestionsController suggestions_controller_;
  std::unique_ptr<FeedV2Builder> feed_v2_builder_;

  PrefChangeRegistrar pref_change_registrar_;
  base::OneShotTimer timer_prefetch_;
  base::OneShotTimer p3a_enabled_report_timer_;
  base::RepeatingTimer timer_feed_update_;
  base::RepeatingTimer timer_publishers_update_;
  base::CancelableTaskTracker task_tracker_;

  base::ScopedObservation<PublishersController, PublishersController::Observer>
      publishers_observation_;
  mojo::ReceiverSet<mojom::BraveNewsController> receivers_;
  mojo::RemoteSet<mojom::PublishersListener> publishers_listeners_;
  mojo::RemoteSet<mojom::ConfigurationListener> configuration_listeners_;
  base::WeakPtrFactory<BraveNewsController> weak_ptr_factory_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_CONTROLLER_H_
