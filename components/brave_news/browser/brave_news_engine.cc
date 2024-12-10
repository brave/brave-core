// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_engine.h"

#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_controller.h"
#include "brave/components/brave_news/browser/feed_v2_builder.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/suggestions_controller.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {
BraveNewsEngine::BraveNewsEngine(
    std::unique_ptr<network::PendingSharedURLLoaderFactory>
        pending_shared_url_loader_factory,
    BackgroundHistoryQuerier history_querier,
    base::WeakPtr<DirectFeedFetcher::Delegate> direct_feed_fetcher_delegate)
    : pending_shared_url_loader_factory_(
          std::move(pending_shared_url_loader_factory)),
      history_querier_(std::move(history_querier)),
      direct_feed_fetcher_delegate_(direct_feed_fetcher_delegate) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

BraveNewsEngine::~BraveNewsEngine() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void BraveNewsEngine::GetLocale(SubscriptionsSnapshot snapshot,
                                m::GetLocaleCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetPublishersController()->GetLocale(snapshot, std::move(callback));
}

void BraveNewsEngine::GetSignals(SubscriptionsSnapshot snapshot,
                                 m::GetSignalsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* builder = MaybeFeedV2Builder();
  CHECK(builder);

  builder->GetSignals(snapshot, std::move(callback));
}

void BraveNewsEngine::GetPublishers(SubscriptionsSnapshot snapshot,
                                    m::GetPublishersCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetPublishersController()->GetOrFetchPublishers(snapshot,
                                                  std::move(callback));
}

void BraveNewsEngine::GetPublisherForSite(SubscriptionsSnapshot snapshot,
                                          GURL site_url,
                                          GetPublisherCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetPublishersController()->GetPublisherForSite(snapshot, site_url,
                                                 std::move(callback));
}

void BraveNewsEngine::GetPublisherForFeed(SubscriptionsSnapshot snapshot,
                                          GURL feed_url,
                                          GetPublisherCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetPublishersController()->GetPublisherForFeed(snapshot, feed_url,
                                                 std::move(callback));
}

void BraveNewsEngine::EnsurePublishersIsUpdating(
    SubscriptionsSnapshot snapshot) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetPublishersController()->EnsurePublishersIsUpdating(snapshot);
}

void BraveNewsEngine::GetChannels(SubscriptionsSnapshot snapshot,
                                  m::GetChannelsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetChannelsController()->GetAllChannels(snapshot, std::move(callback));
}

void BraveNewsEngine::GetSuggestedPublisherIds(
    SubscriptionsSnapshot snapshot,
    m::GetSuggestedPublisherIdsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetSuggestionsController()->GetSuggestedPublisherIds(snapshot,
                                                       std::move(callback));
}

void BraveNewsEngine::GetFeed(SubscriptionsSnapshot snapshot,
                              m::GetFeedCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* builder = MaybeFeedV1Builder()) {
    builder->GetOrFetchFeed(snapshot, std::move(callback));
  } else {
    std::move(callback).Run(mojom::Feed::New());
  }
}

void BraveNewsEngine::GetFeedV2(SubscriptionsSnapshot snapshot,
                                m::GetFeedV2Callback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* builder = MaybeFeedV2Builder();
  CHECK(builder);

  builder->BuildAllFeed(snapshot, std::move(callback));
}

void BraveNewsEngine::GetFollowingFeed(SubscriptionsSnapshot snapshot,
                                       m::GetFollowingFeedCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* builder = MaybeFeedV2Builder();
  CHECK(builder);

  builder->BuildFollowingFeed(snapshot, std::move(callback));
}

void BraveNewsEngine::GetChannelFeed(SubscriptionsSnapshot snapshot,
                                     const std::string& channel,
                                     m::GetPublisherFeedCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* builder = MaybeFeedV2Builder();
  CHECK(builder);

  builder->BuildChannelFeed(snapshot, channel, std::move(callback));
}

void BraveNewsEngine::GetPublisherFeed(SubscriptionsSnapshot snapshot,
                                       const std::string& publisher_id,
                                       m::GetPublisherFeedCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* builder = MaybeFeedV2Builder();
  CHECK(builder);

  builder->BuildPublisherFeed(snapshot, publisher_id, std::move(callback));
}

void BraveNewsEngine::CheckForFeedsUpdate(SubscriptionsSnapshot snapshot,
                                          bool refetch_data,
                                          HashCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* builder = MaybeFeedV2Builder()) {
    builder->GetLatestHash(snapshot, refetch_data, std::move(callback));
    return;
  }

  auto* builder = MaybeFeedV1Builder();
  CHECK(builder);

  builder->UpdateIfRemoteChanged(snapshot, std::move(callback));
}

void BraveNewsEngine::PrefetchFeed(SubscriptionsSnapshot snapshot) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* builder = MaybeFeedV2Builder()) {
    builder->BuildAllFeed(snapshot, base::DoNothing());
    return;
  }

  auto* builder = MaybeFeedV1Builder();
  CHECK(builder);
  builder->EnsureFeedIsCached(snapshot);
}

FeedV2Builder* BraveNewsEngine::MaybeFeedV2Builder() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!base::FeatureList::IsEnabled(features::kBraveNewsFeedUpdate)) {
    return nullptr;
  }

  if (!feed_v2_builder_) {
    feed_v2_builder_ = std::make_unique<FeedV2Builder>(
        *GetPublishersController(), *GetChannelsController(),
        *GetSuggestionsController(), history_querier_,
        GetSharedURLLoaderFactory(), direct_feed_fetcher_delegate_);
  }

  return feed_v2_builder_.get();
}

FeedController* BraveNewsEngine::MaybeFeedV1Builder() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (base::FeatureList::IsEnabled(features::kBraveNewsFeedUpdate)) {
    return nullptr;
  }

  if (!feed_controller_) {
    feed_controller_ = std::make_unique<FeedController>(
        GetPublishersController(), history_querier_,
        GetSharedURLLoaderFactory(), direct_feed_fetcher_delegate_);
  }

  return feed_controller_.get();
}

scoped_refptr<network::SharedURLLoaderFactory>
BraveNewsEngine::GetSharedURLLoaderFactory() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto factory = network::SharedURLLoaderFactory::Create(
      std::move(pending_shared_url_loader_factory_));
  pending_shared_url_loader_factory_ = factory->Clone();
  return factory;
}

api_request_helper::APIRequestHelper* BraveNewsEngine::GetApiRequestHelper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!api_request_helper_) {
    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            GetNetworkTrafficAnnotationTag(), GetSharedURLLoaderFactory());
  }

  return api_request_helper_.get();
}

PublishersController* BraveNewsEngine::GetPublishersController() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!publishers_controller_) {
    publishers_controller_ =
        std::make_unique<PublishersController>(GetApiRequestHelper());
  }

  return publishers_controller_.get();
}

ChannelsController* BraveNewsEngine::GetChannelsController() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!channels_controller_) {
    channels_controller_ =
        std::make_unique<ChannelsController>(GetPublishersController());
  }

  return channels_controller_.get();
}

SuggestionsController* BraveNewsEngine::GetSuggestionsController() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!suggestions_controller_) {
    suggestions_controller_ = std::make_unique<SuggestionsController>(
        GetPublishersController(), GetApiRequestHelper(), history_querier_);
  }

  return suggestions_controller_.get();
}

base::WeakPtr<BraveNewsEngine> BraveNewsEngine::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace brave_news
