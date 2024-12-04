// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_BUILDER_H_

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/browser/suggestions_controller.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "components/history/core/browser/history_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

using BuildFeedCallback = mojom::BraveNewsController::GetFeedV2Callback;
using GetSignalsCallback = mojom::BraveNewsController::GetSignalsCallback;
using HashCallback = base::OnceCallback<void(const std::string&)>;

class FeedGenerationInfo;

class FeedV2Builder {
 public:
  FeedV2Builder(
      PublishersController& publishers_controller,
      ChannelsController& channels_controller,
      SuggestionsController& suggestions_controller,
      BackgroundHistoryQuerier& history_querier,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      DirectFeedFetcher::Delegate* direct_feed_fetcher_delegate);
  FeedV2Builder(const FeedV2Builder&) = delete;
  FeedV2Builder& operator=(const FeedV2Builder&) = delete;
  ~FeedV2Builder();

  void AddListener(mojo::PendingRemote<mojom::FeedListener> listener);

  void BuildFollowingFeed(const SubscriptionsSnapshot& subscriptions,
                          BuildFeedCallback callback);
  void BuildChannelFeed(const SubscriptionsSnapshot& subscriptions,
                        const std::string& channel,
                        BuildFeedCallback callback);
  void BuildPublisherFeed(const SubscriptionsSnapshot& subscriptions,
                          const std::string& publisher_id,
                          BuildFeedCallback callback);
  void BuildAllFeed(const SubscriptionsSnapshot& subscriptions,
                    BuildFeedCallback callback);

  void GetSignals(const SubscriptionsSnapshot& subscriptions,
                  GetSignalsCallback callback);

  void GetLatestHash(const SubscriptionsSnapshot& subscriptions,
                     bool refetch_data,
                     HashCallback callback);

 private:
  // FeedGenerator's will be called on a different thread. The data in
  // |FeedGenerationInfo| is a copy and can be safely modified.
  using FeedGenerator =
      base::OnceCallback<mojom::FeedV2Ptr(FeedGenerationInfo)>;

  using UpdateCallback = base::OnceClosure;
  struct UpdateSettings {
    bool signals = false;
    bool suggested_publishers = false;
    bool feed = false;
    bool topics = false;
  };

  // If an update is in progress and we request another update it is possible
  // that the original request won't fulfill the requirements of the second
  // request. Consider:
  // 1. Request A comes in. It wants {.signals} to update. We start the update.
  // 2. Request B comes in. It wants {.feed, .topics} to update. We can't change
  //    what A is requesting because it is in progress but we can queue a
  //    subsequent update when A completes.
  // 3. Request C comes in. It wants {.signals} to update. This is fulfilled by
  //    the pending update for A, so we add it's listener to |current_update_|.
  // 4. Request D comes in. It wants {.signals, .topics}. We amend the
  //    |next_update_| to be {.feed,.topics,.signals} as it hasn't started yet,
  //    and add D to the next update.
  // In this way, we only ever have one update in progress, and optionally, one
  // update queued.
  struct UpdateRequest {
    UpdateSettings settings;
    std::vector<UpdateCallback> callbacks;
    SubscriptionsSnapshot subscriptions;

    explicit UpdateRequest(SubscriptionsSnapshot subscriptions,
                           UpdateSettings settings,
                           UpdateCallback callback);
    ~UpdateRequest();
    UpdateRequest(const UpdateRequest&) = delete;
    UpdateRequest& operator=(const UpdateRequest&) = delete;
    UpdateRequest(UpdateRequest&&);
    UpdateRequest& operator=(UpdateRequest&&);

    // Indicates whether this UpdateRequest will fulfill an update with the
    // specified |other_settings|. For example {.signals,.topics} is sufficient
    // for {.signals}, {.topics} and {.signals,.topics} but not for {.feed}.
    bool IsSufficient(const UpdateSettings& other_settings);

    // Merges some settings into this update request and appends a callback. For
    // example, if the UpdateRequest is for {.signals} and |other_settings| is
    // for |.topics| this request will be updated to be {.signals,.topics}.
    void AlsoUpdate(const UpdateSettings& other_settings,
                    UpdateCallback callback);
  };

  static mojom::FeedV2Ptr GenerateBasicFeed(FeedGenerationInfo info,
                                            PickArticles pick_hero,
                                            PickArticles pick_article,
                                            PickArticles pick_peeking);
  static mojom::FeedV2Ptr GenerateAllFeed(FeedGenerationInfo info);

  void UpdateData(const SubscriptionsSnapshot& subscriptions,
                  UpdateSettings settings,
                  UpdateCallback callback);

  void PrepareAndFetch();
  void FetchFeed();
  void OnFetchedFeed(FeedItems items, ETags etags);

  void CalculateSignals();
  void OnCalculatedSignals(Signals signals);

  void GetSuggestedPublisherIds();
  void OnGotSuggestedPublisherIds(
      const std::vector<std::string>& suggested_ids);

  void GetTopics();
  void OnGotTopics(TopicsResult topics);

  void NotifyUpdateCompleted();

  void GenerateFeed(const SubscriptionsSnapshot& subscriptions,
                    UpdateSettings settings,
                    mojom::FeedV2TypePtr type,
                    FeedGenerator generator,
                    BuildFeedCallback callback);

  raw_ref<PublishersController> publishers_controller_;
  raw_ref<ChannelsController> channels_controller_;
  raw_ref<SuggestionsController> suggestions_controller_;

  FeedFetcher fetcher_;
  TopicsFetcher topics_fetcher_;
  SignalCalculator signal_calculator_;

  FeedItems raw_feed_items_;
  ETags feed_etags_;
  std::string hash_;

  Signals signals_;
  std::vector<std::string> suggested_publisher_ids_;
  TopicsResult topics_;
  size_t subscribed_count_ = 0;

  std::optional<UpdateRequest> current_update_;
  std::optional<UpdateRequest> next_update_;

  mojo::RemoteSet<mojom::FeedListener> listeners_;

  base::WeakPtrFactory<FeedV2Builder> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_BUILDER_H_
