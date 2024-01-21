// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_BUILDER_H_

#include <cstddef>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/browser/suggestions_controller.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

using BuildFeedCallback = mojom::BraveNewsController::GetFeedV2Callback;
using GetSignalsCallback = mojom::BraveNewsController::GetSignalsCallback;

// An ArticleWeight has a few different components
struct ArticleWeight {
  // The pop_recency of the article. This is used for discover cards, where we
  // don't consider the subscription status or visit_weighting.
  double pop_recency = 0;

  // The complete weighting of the article, combining the pop_score,
  // visit_weighting & subscribed_weighting.
  double weighting = 0;

  // Whether the source which this article comes from has been visited. This
  // only considers Publishers, not Channels.
  bool visited = false;

  // Whether any sources/channels that could cause this article to be shown are
  // subscribed. At this point, disabled sources have already been filtered out.
  bool subscribed = false;
};

using ArticleInfo = std::tuple<mojom::FeedItemMetadataPtr, ArticleWeight>;
using ArticleInfos = std::vector<ArticleInfo>;
using PickArticles = base::RepeatingCallback<int(const ArticleInfos& infos)>;

class FeedV2Builder : public PublishersController::Observer {
 public:
  FeedV2Builder(
      PublishersController& publishers_controller,
      ChannelsController& channels_controller,
      SuggestionsController& suggestions_controller,
      PrefService& prefs,
      history::HistoryService& history_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  FeedV2Builder(const FeedV2Builder&) = delete;
  FeedV2Builder& operator=(const FeedV2Builder&) = delete;
  ~FeedV2Builder() override;

  void AddListener(mojo::PendingRemote<mojom::FeedListener> listener);

  void BuildFollowingFeed(BuildFeedCallback callback);
  void BuildChannelFeed(const std::string& channel, BuildFeedCallback callback);
  void BuildPublisherFeed(const std::string& publisher_id,
                          BuildFeedCallback callback);
  void BuildAllFeed(BuildFeedCallback callback);
  void EnsureFeedIsUpdating();

  void GetSignals(GetSignalsCallback callback);

  void RecheckFeedHash();

  // PublishersController::Observer:
  void OnPublishersUpdated(PublishersController* controller) override;

 private:
  using UpdateCallback = base::OnceCallback<void()>;
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

    explicit UpdateRequest(UpdateSettings settings, UpdateCallback callback);
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

  void UpdateData(UpdateSettings settings,
                  UpdateCallback callback = base::DoNothing());

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

  void GenerateFeed(UpdateSettings settings,
                    mojom::FeedV2TypePtr type,
                    base::OnceCallback<mojom::FeedV2Ptr()> build_feed,
                    BuildFeedCallback callback);

  mojom::FeedV2Ptr GenerateBasicFeed(const FeedItems& items,
                                     PickArticles pick_hero,
                                     PickArticles pick_article);
  mojom::FeedV2Ptr GenerateAllFeed();

  raw_ref<PublishersController> publishers_controller_;
  raw_ref<ChannelsController> channels_controller_;
  raw_ref<SuggestionsController> suggestions_controller_;
  raw_ref<PrefService> prefs_;

  base::ScopedObservation<PublishersController, PublishersController::Observer>
      publishers_observation_{this};

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
