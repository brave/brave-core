// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_controller.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/bind_post_task.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/brave_news_engine.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/locales_helper.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "brave/components/brave_news/common/to_value.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "brave/components/brave_private_cdn/private_cdn_request_helper.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_news {
namespace {

// The favicon size we desire. The favicons are rendered at 24x24 pixels but
// they look quite a bit nicer if we get a 48x48 pixel icon and downscale it.
constexpr uint32_t kDesiredFaviconSizePixels = 48;

// Creates a ChangeEvent from a lookup of all possible items and a diff.
template <class MojomType>
base::Value CreateChangeEvent(std::string_view state_key,
                              SubscriptionsDiff diff,
                              base::flat_map<std::string, MojomType> lookup) {
  auto event = base::Value::Dict();
  for (const auto& changed_id : diff.changed) {
    auto it = lookup.find(changed_id);
    if (it == lookup.end()) {
      continue;
    }
    event.Set(changed_id, mojom::ToValue(it->second));
  }

  // Set removed items to null
  for (const auto& removed_id : diff.removed) {
    event.Set(removed_id, base::Value(base::Value::Type::NONE));
  }

  base::Value result = base::Value(base::Value::Type::DICT);
  result.GetDict().Set(state_key, std::move(event));
  return result;
}

}  // namespace

// Invokes a method on the BraveNewsEngine in a background thread and invokes
// |CB| on the current thread.
#define IN_ENGINE(Method, CB, ...)                                     \
  task_runner_->PostTask(                                              \
      FROM_HERE,                                                       \
      base::BindOnce(                                                  \
          &BraveNewsEngine::Method, engine_->AsWeakPtr(),              \
          pref_manager_.GetSubscriptions() __VA_OPT__(, ) __VA_ARGS__, \
          base::BindPostTaskToCurrentDefault(CB)))

// Invokes a method on the BraveNewsEngine in a background thread. Unlike
// |IN_ENGINE| it doesn't take a reply callback (it's Fire and Forget).
#define IN_ENGINE_FF(Method)                                         \
  task_runner_->PostTask(                                            \
      FROM_HERE,                                                     \
      base::BindOnce(&BraveNewsEngine::Method, engine_->AsWeakPtr(), \
                     pref_manager_.GetSubscriptions()))

BraveNewsController::BraveNewsController(
    PrefService* prefs,
    favicon::FaviconService* favicon_service,
    brave_ads::AdsService* ads_service,
    history::HistoryService* history_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::unique_ptr<DirectFeedFetcher::Delegate> direct_feed_fetcher_delegate)
    : favicon_service_(favicon_service),
      ads_service_(ads_service),
      api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      private_cdn_request_helper_(GetNetworkTrafficAnnotationTag(),
                                  url_loader_factory),
      history_service_(history_service),
      url_loader_factory_(url_loader_factory),
      direct_feed_fetcher_delegate_(std::move(direct_feed_fetcher_delegate)),
      pref_manager_(*prefs),
      news_metrics_(prefs, pref_manager_),
      direct_feed_controller_(url_loader_factory,
                              direct_feed_fetcher_delegate_->AsWeakPtr()),
      task_runner_(base::ThreadPool::CreateSingleThreadTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      engine_(nullptr, base::OnTaskRunnerDeleter(task_runner_)),
      initialization_promise_(
          3,
          pref_manager_,
          base::BindRepeating(
              &BraveNewsController::GetLocale,
              // Unretained is safe here because |initialization_promise_| is
              // owned by BraveNewsController.
              base::Unretained(this))) {
  ResetEngine();
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);

  prefs_observation_.Observe(&pref_manager_);

  news_metrics_.RecordAtInit();
  // Monitor kBraveNewsSources and update feed / publisher cache
  // Start timer of updating feeds, if applicable
  ConditionallyStartOrStopTimer();
}

BraveNewsController::~BraveNewsController() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void BraveNewsController::Bind(
    mojo::PendingReceiver<mojom::BraveNewsController> receiver) {
  DVLOG(1) << __FUNCTION__;
  receivers_.Add(this, std::move(receiver));
}

void BraveNewsController::Bind(
    mojo::PendingReceiver<mojom::BraveNewsInternals> receiver) {
  DVLOG(1) << __FUNCTION__;
  internals_receivers_.Add(this, std::move(receiver));
}

void BraveNewsController::ClearHistory() {
  DVLOG(1) << __FUNCTION__;
  // TODO(petemill): Clear history once/if we actually store
  // feed cache somewhere.
}

mojo::PendingRemote<mojom::BraveNewsController>
BraveNewsController::MakeRemote() {
  mojo::PendingRemote<mojom::BraveNewsController> remote;
  DVLOG(1) << __FUNCTION__;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BraveNewsController::GetLocale(GetLocaleCallback callback) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    std::move(callback).Run("");
    return;
  }

  IN_ENGINE(GetLocale, std::move(callback));
}

void BraveNewsController::GetFeed(GetFeedCallback callback) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    std::move(callback).Run(brave_news::mojom::Feed::New());
    return;
  }

  // If we're only recently opted-in but we haven't yet finished adding the
  // top sources subscription (via the async functions in MaybeInitPrefs), we
  // need to wait for that to complete before we can fetch the feed.
  if (!initialization_promise_.complete()) {
    initialization_promise_.OnceInitialized(
        base::BindOnce(&BraveNewsController::GetFeed,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  IN_ENGINE(GetFeed, std::move(callback));
}

void BraveNewsController::GetFollowingFeed(GetFollowingFeedCallback callback) {
  DVLOG(1) << __FUNCTION__;
  IN_ENGINE(GetFollowingFeed, std::move(callback));
}

void BraveNewsController::GetChannelFeed(const std::string& channel,
                                         GetChannelFeedCallback callback) {
  DVLOG(1) << __FUNCTION__;
  IN_ENGINE(GetChannelFeed, std::move(callback), channel);
}

void BraveNewsController::GetPublisherFeed(const std::string& publisher_id,
                                           GetPublisherFeedCallback callback) {
  DVLOG(1) << __FUNCTION__;
  IN_ENGINE(GetPublisherFeed, std::move(callback), publisher_id);
}

void BraveNewsController::GetPublisherForSite(const GURL& site_url,
                                              GetPublisherCallback callback) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    std::move(callback).Run(nullptr);
    return;
  }
  IN_ENGINE(GetPublisherForSite, std::move(callback), site_url);
}

void BraveNewsController::GetPublisherForFeed(const GURL& feed_url,
                                              GetPublisherCallback callback) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    std::move(callback).Run(nullptr);
    return;
  }
  IN_ENGINE(GetPublisherForFeed, std::move(callback), feed_url);
}

void BraveNewsController::EnsureFeedV2IsUpdating() {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    return;
  }
  CheckForFeedsUpdate();
}

void BraveNewsController::GetFeedV2(GetFeedV2Callback callback) {
  DVLOG(1) << __FUNCTION__;
  // If we're only recently opted-in but we haven't yet finished adding the
  // top sources subscription (via the async functions in MaybeInitPrefs), we
  // need to wait for that to complete before we can fetch the feed.
  if (!initialization_promise_.complete()) {
    initialization_promise_.OnceInitialized(
        base::BindOnce(&BraveNewsController::GetFeedV2,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  IN_ENGINE(GetFeedV2, std::move(callback));
}

void BraveNewsController::GetSignals(GetSignalsCallback callback) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    std::move(callback).Run({});
    return;
  }
  IN_ENGINE(GetSignals, std::move(callback));
}

void BraveNewsController::GetPublishers(GetPublishersCallback callback) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    std::move(callback).Run({});
    return;
  }

  IN_ENGINE(GetPublishers, std::move(callback));
}

void BraveNewsController::GetSuggestedPublisherIds(
    GetSuggestedPublisherIdsCallback callback) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    std::move(callback).Run({});
    return;
  }
  IN_ENGINE(GetSuggestedPublisherIds, std::move(callback));
}

void BraveNewsController::RefreshSuggestedPublisherIds() {
  DVLOG(1) << __FUNCTION__;
  GetSuggestedPublisherIds(base::BindOnce(
      [](base::WeakPtr<BraveNewsController> controller,
         const std::vector<std::string>& ids) {
        if (!controller) {
          return;
        }

        auto changed = base::Value(base::Value::Type::DICT);
        changed.GetDict().Set("suggestedPublisherIds", mojom::ToValue(ids));
        controller->NotifyChanged(std::move(changed));
      },
      weak_ptr_factory_.GetWeakPtr()));
}

void BraveNewsController::FindFeeds(const GURL& possible_feed_or_site_url,
                                    FindFeedsCallback callback) {
  DVLOG(1) << __FUNCTION__;
  direct_feed_controller_.FindFeeds(possible_feed_or_site_url,
                                    std::move(callback));
}

void BraveNewsController::GetChannels(GetChannelsCallback callback) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    std::move(callback).Run({});
    return;
  }
  if (!initialization_promise_.complete()) {
    initialization_promise_.OnceInitialized(
        base::BindOnce(&BraveNewsController::GetChannels,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }
  IN_ENGINE(GetChannels, std::move(callback));
}

void BraveNewsController::AddListener(
    mojo::PendingRemote<mojom::Listener> listener) {
  DVLOG(1) << __FUNCTION__;
  auto id = listeners_.Add(std::move(listener));
  GetState(base::BindOnce(
      [](mojo::RemoteSetElementId id,
         base::WeakPtr<BraveNewsController> controller, mojom::StatePtr state) {
        if (!controller) {
          return;
        }

        auto* listener = controller->listeners_.Get(id);
        if (!listener) {
          return;
        }

        listener->Changed(mojom::ToValue(state));
      },
      id, weak_ptr_factory_.GetWeakPtr()));
}

void BraveNewsController::SetChannelSubscribed(
    const std::string& locale,
    const std::string& channel_id,
    bool subscribed,
    SetChannelSubscribedCallback callback) {
  DVLOG(1) << __FUNCTION__;
  pref_manager_.SetChannelSubscribed(locale, channel_id, subscribed);
  IN_ENGINE(GetChannels,
            base::BindOnce(
                [](std::string channel_id,
                   SetChannelSubscribedCallback callback, Channels channels) {
                  auto channel = std::move(channels.at(channel_id));
                  std::move(callback).Run(std::move(channel));
                },
                channel_id, std::move(callback)));
}

void BraveNewsController::SubscribeToNewDirectFeed(
    const GURL& feed_url,
    SubscribeToNewDirectFeedCallback callback) {
  VLOG(1) << __FUNCTION__ << ": " << feed_url.spec();
  // Verify the url points at a valid feed
  if (!feed_url.is_valid()) {
    std::move(callback).Run(false, false, std::nullopt);
    return;
  }
  direct_feed_controller_.VerifyFeedUrl(
      feed_url,
      base::BindOnce(&BraveNewsController::OnVerifiedDirectFeedUrl,
                     base::Unretained(this), feed_url, std::move(callback)));
}

void BraveNewsController::OnVerifiedDirectFeedUrl(
    const GURL& feed_url,
    SubscribeToNewDirectFeedCallback callback,
    const bool is_valid,
    const std::string& feed_title) {
  VLOG(1) << __FUNCTION__ << " Is new feed valid? " << is_valid
          << " Title: " << feed_title;
  if (!is_valid) {
    std::move(callback).Run(false, false, std::nullopt);
    return;
  }

  pref_manager_.AddDirectPublisher(feed_url, feed_title);

  auto direct_feed_config = pref_manager_.GetSubscriptions().direct_feeds();
  std::vector<mojom::PublisherPtr> direct_feeds;

  ParseDirectPublisherList(direct_feed_config, &direct_feeds);
  auto change = base::Value(base::Value::Type::DICT);

  for (auto& feed : direct_feeds) {
    auto publisher_id = feed->publisher_id;
    change.GetDict().SetByDottedPath(
        base::StrCat({"publishers.", publisher_id}), mojom::ToValue(feed));
  }

  NotifyChanged(std::move(change));

  // Mark feed as requiring update
  // TODO(petemill): expose function to mark direct feeds as dirty
  // and not require re-download of sources.json
  IN_ENGINE_FF(EnsurePublishersIsUpdating);

  // Pass publishers to callback, waiting for updated publishers list
  IN_ENGINE(
      GetPublishers,
      base::BindOnce(
          [](SubscribeToNewDirectFeedCallback callback, Publishers publishers) {
            std::move(callback).Run(
                true, false, std::optional<Publishers>(std::move(publishers)));
          },
          std::move(callback)));
}

void BraveNewsController::RemoveDirectFeed(const std::string& publisher_id) {
  DVLOG(1) << __FUNCTION__;
  pref_manager_.SetPublisherSubscribed(publisher_id,
                                       mojom::UserEnabled::DISABLED);

  // Mark feed as requiring update
  IN_ENGINE_FF(EnsurePublishersIsUpdating);

  auto diff = base::Value(base::Value::Type::DICT);
  auto publishers_diff = base::Value::Dict();
  publishers_diff.Set(publisher_id, base::Value(base::Value::Type::NONE));
  diff.GetDict().Set("publishers", std::move(publishers_diff));
  NotifyChanged(std::move(diff));
}

void BraveNewsController::GetImageData(const GURL& padded_image_url,
                                       GetImageDataCallback callback) {
  VLOG(2) << __FUNCTION__ << " " << padded_image_url.spec();
  // Validate
  if (!padded_image_url.is_valid()) {
    std::optional<std::vector<uint8_t>> args;
    std::move(callback).Run(std::move(args));
    return;
  }
  // Use file ending to determine if response
  // will contain (Brave's PrivateCDN) padding or
  // be a direct image
  const auto file_name = padded_image_url.path();
  const std::string ending = ".pad";
  if (file_name.length() >= file_name.max_size() - 1 ||
      file_name.length() <= ending.length()) {
    std::optional<std::vector<uint8_t>> args;
    std::move(callback).Run(std::move(args));
    return;
  }
  const bool is_padded =
      (file_name.compare(file_name.length() - ending.length(), ending.length(),
                         ending) == 0);
  VLOG(3) << "is padded: " << is_padded;
  // Make the request
  private_cdn_request_helper_.DownloadToString(
      padded_image_url,
      base::BindOnce(
          [](GetImageDataCallback callback, const bool is_padded,
             const int response_code, const std::string& body) {
            // Handle the response
            VLOG(3) << "getimagedata response code: " << response_code;
            // Attempt to remove byte padding if applicable
            std::string_view body_payload(body.data(), body.size());
            if (response_code < 200 || response_code >= 300 ||
                (is_padded &&
                 !brave::PrivateCdnHelper::GetInstance()->RemovePadding(
                     &body_payload))) {
              // Byte padding removal failed
              std::move(callback).Run(std::nullopt);
              return;
            }
            // Download (and optional unpadding) was successful,
            // uint8Array will be easier to move over mojom.
            std::vector<uint8_t> image_bytes(body_payload.begin(),
                                             body_payload.end());
            std::move(callback).Run(image_bytes);
          },
          std::move(callback), is_padded));
}

void BraveNewsController::GetFavIconData(const std::string& publisher_id,
                                         GetFavIconDataCallback callback) {
  DVLOG(1) << __FUNCTION__;
  GetPublishers(base::BindOnce(
      [](BraveNewsController* controller, const std::string& publisher_id,
         GetFavIconDataCallback callback, Publishers publishers) {
        // If the publisher doesn't exist, there's nothing we can do.
        const auto& it = publishers.find(publisher_id);
        if (it == publishers.end()) {
          std::move(callback).Run(std::nullopt);
          return;
        }

        // If we have a FavIcon url, use that.
        const auto& publisher = it->second;
        if (publisher->favicon_url) {
          controller->GetImageData(publisher->favicon_url.value(),
                                   std::move(callback));
          return;
        }

        auto source_url = publisher->site_url.is_valid()
                              ? publisher->site_url
                              : publisher->feed_source;
        favicon_base::IconTypeSet icon_types{
            favicon_base::IconType::kFavicon,
            favicon_base::IconType::kTouchIcon};
        controller->favicon_service_->GetRawFaviconForPageURL(
            source_url, icon_types, kDesiredFaviconSizePixels, true,
            base::BindOnce(
                [](GetFavIconDataCallback callback,
                   const favicon_base::FaviconRawBitmapResult& result) {
                  if (!result.is_valid()) {
                    std::move(callback).Run(std::nullopt);
                    return;
                  }

                  std::vector<uint8_t> bytes_vec(result.bitmap_data->begin(),
                                                 result.bitmap_data->end());
                  std::move(callback).Run(std::move(bytes_vec));
                },
                std::move(callback)),
            &controller->task_tracker_);
      },
      base::Unretained(this), publisher_id, std::move(callback)));
}

void BraveNewsController::SetPublisherPref(const std::string& publisher_id,
                                           mojom::UserEnabled new_status) {
  VLOG(1) << __FUNCTION__ << " " << publisher_id << ": " << new_status;
  GetPublishers(base::BindOnce(
      [](const std::string& publisher_id, mojom::UserEnabled new_status,
         BraveNewsController* controller, Publishers publishers) {
        if (!publishers.contains(publisher_id)) {
          LOG(ERROR) << "Attempted to set publisher pref which didn't exist: "
                     << publisher_id;
          return;
        }
        controller->pref_manager_.SetPublisherSubscribed(publisher_id,
                                                         new_status);
        auto subscriptions = controller->pref_manager_.GetSubscriptions();
      },
      publisher_id, new_status, base::Unretained(this)));
}

void BraveNewsController::ClearPrefs() {
  DVLOG(1) << __FUNCTION__;
  pref_manager_.ClearPrefs();
}

void BraveNewsController::IsFeedUpdateAvailable(
    const std::string& displayed_feed_hash,
    IsFeedUpdateAvailableCallback callback) {
  DVLOG(1) << __FUNCTION__;
  IN_ENGINE(CheckForFeedsUpdate,
            base::BindOnce(
                [](const std::string& displayed_feed_hash,
                   IsFeedUpdateAvailableCallback callback,
                   const std::string& latest_hash) {
                  std::move(callback).Run(displayed_feed_hash != latest_hash);
                },
                displayed_feed_hash, std::move(callback)),
            /*refetch_data=*/true);
}

void BraveNewsController::AddFeedListener(
    mojo::PendingRemote<mojom::FeedListener> listener) {
  DVLOG(1) << __FUNCTION__;
  feed_listeners_.Add(std::move(listener));
}

void BraveNewsController::SetConfiguration(
    mojom::ConfigurationPtr configuration,
    SetConfigurationCallback callback) {
  DVLOG(1) << __FUNCTION__;
  pref_manager_.SetConfig(std::move(configuration));
  std::move(callback).Run();
}

void BraveNewsController::GetDisplayAd(GetDisplayAdCallback callback) {
  DVLOG(1) << __FUNCTION__;
  // TODO(petemill): maybe we need to have a way to re-fetch ads_service,
  // since it may have been disabled at time of service creation and enabled
  // some time later.
  if (!ads_service_) {
    VLOG(1) << "GetDisplayAd: no ads service";
    std::move(callback).Run(nullptr);
    return;
  }
  auto on_ad_received = base::BindOnce(
      [](GetDisplayAdCallback callback, const std::string& dimensions,
         base::optional_ref<const brave_ads::InlineContentAdInfo>
             inline_content_ad) {
        if (!inline_content_ad) {
          VLOG(1) << "GetDisplayAd: no ad";
          std::move(callback).Run(nullptr);
          return;
        }
        VLOG(1) << "GetDisplayAd: GOT ad";
        // Convert to our mojom entity.
        // TODO(petemill): brave_ads seems to use mojom, perhaps we can
        // receive and send to callback the actual typed mojom struct from
        // brave_ads?
        auto ad = mojom::DisplayAd::New();
        ad->uuid = inline_content_ad->placement_id;
        ad->creative_instance_id = inline_content_ad->creative_instance_id;
        ad->cta_text = inline_content_ad->cta_text;
        ad->dimensions = inline_content_ad->dimensions;
        ad->title = inline_content_ad->title;
        ad->description = inline_content_ad->description;
        ad->image =
            mojom::Image::NewPaddedImageUrl(inline_content_ad->image_url);
        ad->target_url = inline_content_ad->target_url;
        std::move(callback).Run(std::move(ad));
      },
      std::move(callback));
  ads_service_->MaybeServeInlineContentAd("900x750", std::move(on_ad_received));
}

void BraveNewsController::OnInteractionSessionStarted() {
  DVLOG(1) << __FUNCTION__;
  news_metrics_.RecordAtSessionStart();
}

void BraveNewsController::OnPromotedItemView(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  DVLOG(1) << __FUNCTION__;
  if (ads_service_ && !item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->TriggerPromotedContentAdEvent(
        item_id, creative_instance_id,
        brave_ads::mojom::PromotedContentAdEventType::kViewedImpression,
        /*intentional*/ base::DoNothing());
  }
}

void BraveNewsController::OnPromotedItemVisit(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  DVLOG(1) << __FUNCTION__;
  if (ads_service_ && !item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->TriggerPromotedContentAdEvent(
        item_id, creative_instance_id,
        brave_ads::mojom::PromotedContentAdEventType::kClicked,
        /*intentional*/ base::DoNothing());
  }
}

void BraveNewsController::OnNewCardsViewed(uint16_t card_views) {
  DVLOG(1) << __FUNCTION__;
  news_metrics_.RecordTotalActionCount(p3a::ActionType::kCardView, card_views);
}

void BraveNewsController::OnCardVisited(uint32_t depth) {
  DVLOG(1) << __FUNCTION__;
  news_metrics_.RecordTotalActionCount(p3a::ActionType::kCardVisit, 1);
  news_metrics_.RecordVisitCardDepth(depth);
}

void BraveNewsController::OnSidebarFilterUsage() {
  DVLOG(1) << __FUNCTION__;
  news_metrics_.RecordTotalActionCount(p3a::ActionType::kSidebarFilterUsage, 1);
}

void BraveNewsController::OnDisplayAdVisit(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  DVLOG(1) << __FUNCTION__;
  // Validate
  if (item_id.empty()) {
    LOG(ERROR) << "News: asked to record visit for an ad without ad id";
    return;
  }
  if (creative_instance_id.empty()) {
    LOG(ERROR) << "News: asked to record visit for an ad without "
                  "ad creative instance id";
    return;
  }
  // Let ad service know an ad was visited
  if (!ads_service_) {
    VLOG(1)
        << "News: Asked to record an ad visit but there is no ads service for"
           "this profile!";
    return;
  }
  ads_service_->TriggerInlineContentAdEvent(
      item_id, creative_instance_id,
      brave_ads::mojom::InlineContentAdEventType::kClicked,
      /*intentional*/ base::DoNothing());
}

void BraveNewsController::OnDisplayAdView(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  DVLOG(1) << __FUNCTION__;
  // Validate
  if (item_id.empty()) {
    LOG(ERROR) << "News: asked to record view for an ad without ad id";
    return;
  }
  if (creative_instance_id.empty()) {
    LOG(ERROR) << "News: asked to record view for an ad without "
                  "ad creative instance id";
    return;
  }
  // Let ad service know an ad was viewed
  if (!ads_service_) {
    VLOG(1)
        << "News: Asked to record an ad visit but there is no ads service for"
           "this profile!";
    return;
  }
  ads_service_->TriggerInlineContentAdEvent(
      item_id, creative_instance_id,
      brave_ads::mojom::InlineContentAdEventType::kViewedImpression,
      /*intentional*/ base::DoNothing());

  news_metrics_.RecordWeeklyDisplayAdsViewedCount(true);
}

void BraveNewsController::GetVisitedSites(GetVisitedSitesCallback callback) {
  auto options = GetQueryOptions();
  history_service_->QueryHistory(
      std::u16string(), options,
      base::BindOnce(
          [](GetVisitedSitesCallback callback, history::QueryResults results) {
            std::vector<std::string> urls;
            for (const auto& result : results) {
              urls.push_back(result.url().spec());
            }

            std::move(callback).Run(std::move(urls));
          },
          std::move(callback)),
      &task_tracker_);
}

void BraveNewsController::CheckForPublishersUpdate() {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    return;
  }

  IN_ENGINE_FF(EnsurePublishersIsUpdating);
}

void BraveNewsController::CheckForFeedsUpdate() {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    return;
  }

  IN_ENGINE(CheckForFeedsUpdate,
            base::BindOnce(&BraveNewsController::NotifyFeedChanged,
                           weak_ptr_factory_.GetWeakPtr()),
            /*refetch_data=*/true);
}

void BraveNewsController::Prefetch() {
  VLOG(1) << __FUNCTION__;

  IN_ENGINE_FF(PrefetchFeed);
}

void BraveNewsController::ResetEngine() {
  engine_.reset(
      new BraveNewsEngine(url_loader_factory_->Clone(), MakeHistoryQuerier(),
                          direct_feed_fetcher_delegate_->AsWeakPtr()));
}

void BraveNewsController::ConditionallyStartOrStopTimer() {
  DVLOG(1) << __FUNCTION__;
  // If the user has just enabled the feature for the first time,
  // make sure we're setup or migrated.
  MaybeInitPrefs();
  // Refresh data on an interval only if Brave News is enabled
  if (pref_manager_.IsEnabled()) {
    VLOG(1) << "STARTING TIMERS";
    if (!timer_feed_update_.IsRunning()) {
      timer_feed_update_.Start(FROM_HERE, base::Hours(3), this,
                               &BraveNewsController::CheckForFeedsUpdate);
    }
    if (!timer_publishers_update_.IsRunning()) {
      timer_publishers_update_.Start(
          FROM_HERE, base::Days(1), this,
          &BraveNewsController::CheckForPublishersUpdate);
    }
    if (!timer_prefetch_.IsRunning()) {
      timer_prefetch_.Start(FROM_HERE, base::Minutes(1), this,
                            &BraveNewsController::Prefetch);
    }

    // Notify listeners of the current publishers when BraveNews is enabled.
    GetPublishers(base::BindOnce(
        [](base::WeakPtr<BraveNewsController> controller,
           Publishers publishers) {
          if (!controller) {
            return;
          }

          auto diff = base::Value(base::Value::Type::DICT);
          diff.GetDict().Set("publishers", mojom::ToValue(publishers));
          controller->NotifyChanged(std::move(diff));
        },
        weak_ptr_factory_.GetWeakPtr()));

    // Notify listeners of the current channels when BraveNews is enabled.
    GetChannels(base::BindOnce(
        [](base::WeakPtr<BraveNewsController> controller, Channels channels) {
          if (!controller) {
            return;
          }
          auto diff = base::Value(base::Value::Type::DICT);
          diff.GetDict().Set("channels", mojom::ToValue(channels));
          controller->NotifyChanged(std::move(diff));
        },
        weak_ptr_factory_.GetWeakPtr()));
  } else {
    VLOG(1) << "STOPPING TIMERS";
    timer_feed_update_.Stop();
    timer_publishers_update_.Stop();
    timer_prefetch_.Stop();
    VLOG(1) << "REMOVING DATA FROM MEMORY";

    // Reset our engine so all the caches are deleted.
    ResetEngine();
  }
}

void BraveNewsController::MaybeInitPrefs() {
  DVLOG(1) << __FUNCTION__;
  // When first enabled, we need to create the initial "Top Sources" channel
  // subscription for the revelant locale. We can't do this before opt-in as
  // the list of supported locales needs to be fetched.
  if (!pref_manager_.IsEnabled()) {
    return;
  }

  initialization_promise_.OnceInitialized(
      base::BindOnce(&BraveNewsController::OnInitializingPrefsComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveNewsController::OnInitializingPrefsComplete() {
  DVLOG(1) << __FUNCTION__;
  // Once we've finished initializing prefs, notify channel & publisher
  // listeners.
  GetChannels(base::BindOnce([](Channels channels) {
                auto diff = base::Value(base::Value::Type::DICT);
                diff.GetDict().Set("channels", mojom::ToValue(channels));
                return diff;
              })
                  .Then(base::BindOnce(&BraveNewsController::NotifyChanged,
                                       weak_ptr_factory_.GetWeakPtr())));
}

void BraveNewsController::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    return;
  }

  // Ensure publishers are fetched (this won't do anything if they are). This
  // handles the case where Brave News is started with no network.
  IN_ENGINE_FF(EnsurePublishersIsUpdating);
}

void BraveNewsController::OnConfigChanged() {
  DVLOG(1) << __FUNCTION__;
  ConditionallyStartOrStopTimer();

  auto change = base::Value(base::Value::Type::DICT);
  change.GetDict().Set("config", mojom::ToValue(pref_manager_.GetConfig()));
  NotifyChanged(std::move(change));
}

void BraveNewsController::OnPublishersChanged() {
  DVLOG(1) << __FUNCTION__;
  if (!pref_manager_.IsEnabled()) {
    VLOG(1) << "OnPublishersChanged: News not enabled, doing nothing";
    return;
  }

  VLOG(1) << "OnPublishersChanged: Working out change";
  auto subscriptions = pref_manager_.GetSubscriptions();
  auto diff = subscriptions.DiffPublishers(last_subscriptions_);
  last_subscriptions_ = std::move(subscriptions);

  // When publishers are changed, see if it affects the feed.
  GetPublishers(base::BindOnce(&CreateChangeEvent<mojom::PublisherPtr>,
                               "publishers", std::move(diff))
                    .Then(base::BindOnce(&BraveNewsController::NotifyChanged,
                                         weak_ptr_factory_.GetWeakPtr())));

  // Check for feed update.
  IN_ENGINE(CheckForFeedsUpdate,
            base::BindOnce(&BraveNewsController::NotifyFeedChanged,
                           weak_ptr_factory_.GetWeakPtr()),
            /*refetch_data=*/false);
}

void BraveNewsController::OnChannelsChanged() {
  DVLOG(1) << __FUNCTION__;
  if (pref_manager_.IsEnabled()) {
    VLOG(1) << "OnChannelsChanged: Ensuring feed is updated";
    auto subscriptions = pref_manager_.GetSubscriptions();
    auto diff = subscriptions.DiffChannels(last_subscriptions_);
    last_subscriptions_ = std::move(subscriptions);

    GetChannels(base::BindOnce(&CreateChangeEvent<mojom::ChannelPtr>,
                               "channels", std::move(diff))
                    .Then(base::BindOnce(&BraveNewsController::NotifyChanged,
                                         weak_ptr_factory_.GetWeakPtr())));

    // When channels are changed, see if it affects the feed.
    // TODO: We should fire a callback if an update is available, and notify
    // listeners.
    IN_ENGINE(CheckForFeedsUpdate,
              base::BindOnce(&BraveNewsController::NotifyFeedChanged,
                             weak_ptr_factory_.GetWeakPtr()),
              /*refetch_data=*/false);
  } else {
    VLOG(1) << "OnChannelsChanged: News not enabled, doing nothing.";
  }
}

void BraveNewsController::GetState(BraveNewsEngine::GetStateCallback callback) {
  IN_ENGINE(GetState, base::BindOnce(
                          [](mojom::ConfigurationPtr config,
                             BraveNewsEngine::GetStateCallback callback,
                             mojom::StatePtr state) {
                            state->configuration = std::move(config);
                            std::move(callback).Run(std::move(state));
                          },
                          pref_manager_.GetConfig(), std::move(callback)));
}

void BraveNewsController::NotifyChanged(base::Value change) {
  DVLOG(1) << __FUNCTION__;
  for (const auto& observer : listeners_) {
    observer->Changed(change.Clone());
  }
}

void BraveNewsController::NotifyFeedChanged(const std::string& hash) {
  DVLOG(1) << __FUNCTION__;
  for (const auto& observer : feed_listeners_) {
    observer->OnUpdateAvailable(hash);
  }
}

BackgroundHistoryQuerier BraveNewsController::MakeHistoryQuerier() {
  return brave_news::MakeHistoryQuerier(
      history_service_->AsWeakPtr(),
      base::BindRepeating(
          [](base::WeakPtr<BraveNewsController> controller)
              -> base::CancelableTaskTracker* {
            return controller ? &controller->task_tracker_ : nullptr;
          },
          weak_ptr_factory_.GetWeakPtr()));
}

#undef IN_ENGINE_FF
#undef IN_ENGINE

}  // namespace brave_news
