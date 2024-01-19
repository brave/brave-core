// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_controller.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/feed_v2_builder.h"
#include "brave/components/brave_news/browser/locales_helper.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/browser/suggestions_controller.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "brave/components/brave_private_cdn/private_cdn_request_helper.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_news {
namespace {

// The favicon size we desire. The favicons are rendered at 24x24 pixels but
// they look quite a bit nicer if we get a 48x48 pixel icon and downscale it.
constexpr uint32_t kDesiredFaviconSizePixels = 48;
// Since we have two boolean prefs for the News enabled status, a delay
// will be used so that we only report the histogram once for both pref updates.
constexpr base::TimeDelta kP3AEnabledReportTimeDelay = base::Seconds(3);
}  // namespace

bool GetIsEnabled(PrefService* prefs) {
  bool should_show = prefs->GetBoolean(prefs::kNewTabPageShowToday);
  bool opted_in = prefs->GetBoolean(prefs::kBraveNewsOptedIn);
  return should_show && opted_in;
}

// static
void BraveNewsController::RegisterProfilePrefs(PrefRegistrySimple* registry) {
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

BraveNewsController::BraveNewsController(
    PrefService* prefs,
    favicon::FaviconService* favicon_service,
    brave_ads::AdsService* ads_service,
    history::HistoryService* history_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs),
      favicon_service_(favicon_service),
      ads_service_(ads_service),
      api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      private_cdn_request_helper_(GetNetworkTrafficAnnotationTag(),
                                  url_loader_factory),
      history_service_(history_service),
      url_loader_factory_(url_loader_factory),
      news_metrics_(prefs),
      direct_feed_controller_(prefs_, url_loader_factory),
      unsupported_publisher_migrator_(prefs_,
                                      &direct_feed_controller_,
                                      &api_request_helper_),
      publishers_controller_(prefs_,
                             &direct_feed_controller_,
                             &unsupported_publisher_migrator_,
                             &api_request_helper_,
                             &news_metrics_),
      channels_controller_(prefs_, &publishers_controller_, &news_metrics_),
      feed_controller_(&publishers_controller_,
                       &direct_feed_controller_,
                       &channels_controller_,
                       history_service,
                       url_loader_factory,
                       prefs_),
      suggestions_controller_(prefs_,
                              &publishers_controller_,
                              &api_request_helper_,
                              history_service),
      publishers_observation_(this),
      weak_ptr_factory_(this) {
  DCHECK(prefs_);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);

  // Set up preference listeners
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kBraveNewsOptedIn,
      base::BindRepeating(&BraveNewsController::OnOptInChange,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kNewTabPageShowToday,
      base::BindRepeating(&BraveNewsController::OnOptInChange,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveNewsChannels,
      base::BindRepeating(&BraveNewsController::HandleSubscriptionsChanged,
                          base::Unretained(this)));

  publishers_observation_.Observe(&publishers_controller_);

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
  receivers_.Add(this, std::move(receiver));
}

void BraveNewsController::ClearHistory() {
  // TODO(petemill): Clear history once/if we actually store
  // feed cache somewhere.
}

bool BraveNewsController::MaybeInitFeedV2() {
  if (!GetIsEnabled(prefs_) ||
      !base::FeatureList::IsEnabled(
          brave_news::features::kBraveNewsFeedUpdate)) {
    return false;
  }

  if (!feed_v2_builder_) {
    feed_v2_builder_ = std::make_unique<FeedV2Builder>(
        publishers_controller_, channels_controller_, suggestions_controller_,
        *prefs_.get(), *history_service_.get(), url_loader_factory_);
  }

  return true;
}

mojo::PendingRemote<mojom::BraveNewsController>
BraveNewsController::MakeRemote() {
  mojo::PendingRemote<mojom::BraveNewsController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BraveNewsController::GetLocale(GetLocaleCallback callback) {
  if (!GetIsEnabled(prefs_)) {
    std::move(callback).Run("");
    return;
  }
  publishers_controller_.GetLocale(std::move(callback));
}

void BraveNewsController::GetFeed(GetFeedCallback callback) {
  if (!GetIsEnabled(prefs_)) {
    std::move(callback).Run(brave_news::mojom::Feed::New());
    return;
  }
  feed_controller_.GetOrFetchFeed(std::move(callback));
}

void BraveNewsController::GetFollowingFeed(GetFollowingFeedCallback callback) {
  if (!MaybeInitFeedV2()) {
    std::move(callback).Run(mojom::FeedV2::New());
    return;
  }

  feed_v2_builder_->BuildFollowingFeed(std::move(callback));
}

void BraveNewsController::GetChannelFeed(const std::string& channel,
                                         GetChannelFeedCallback callback) {
  if (!MaybeInitFeedV2()) {
    std::move(callback).Run(mojom::FeedV2::New());
    return;
  }

  feed_v2_builder_->BuildChannelFeed(channel, std::move(callback));
}

void BraveNewsController::GetPublisherFeed(const std::string& publisher_id,
                                           GetPublisherFeedCallback callback) {
  if (!MaybeInitFeedV2()) {
    std::move(callback).Run(mojom::FeedV2::New());
    return;
  }

  feed_v2_builder_->BuildPublisherFeed(publisher_id, std::move(callback));
}

void BraveNewsController::EnsureFeedV2IsUpdating() {
  if (!MaybeInitFeedV2()) {
    return;
  }

  feed_v2_builder_->EnsureFeedIsUpdating();
}

void BraveNewsController::GetFeedV2(GetFeedV2Callback callback) {
  if (!MaybeInitFeedV2()) {
    std::move(callback).Run(mojom::FeedV2::New());
    return;
  }

  feed_v2_builder_->BuildAllFeed(std::move(callback));
}

void BraveNewsController::GetSignals(GetSignalsCallback callback) {
  if (!MaybeInitFeedV2()) {
    std::move(callback).Run({});
    return;
  }

  feed_v2_builder_->GetSignals(std::move(callback));
}

void BraveNewsController::GetPublishers(GetPublishersCallback callback) {
  if (!GetIsEnabled(prefs_)) {
    std::move(callback).Run({});
    return;
  }
  publishers_controller_.GetOrFetchPublishers(std::move(callback));
}

void BraveNewsController::AddPublishersListener(
    mojo::PendingRemote<mojom::PublishersListener> listener) {
  // As we've just bound a new listener, let it know about our publishers.
  // Note: We don't add the listener to the set until |GetPublishers| has
  // returned to avoid invoking the listener twice if a fetch is in progress.
  GetPublishers(base::BindOnce(
      [](BraveNewsController* controller,
         mojo::PendingRemote<mojom::PublishersListener> listener,
         Publishers publishers) {
        auto id = controller->publishers_listeners_.Add(std::move(listener));
        auto* added_listener = controller->publishers_listeners_.Get(id);
        if (added_listener) {
          auto event = mojom::PublishersEvent::New();
          event->addedOrUpdated = std::move(publishers);
          added_listener->Changed(std::move(event));
        }
      },
      base::Unretained(this), std::move(listener)));
}

void BraveNewsController::GetSuggestedPublisherIds(
    GetSuggestedPublisherIdsCallback callback) {
  suggestions_controller_.GetSuggestedPublisherIds(std::move(callback));
}

void BraveNewsController::FindFeeds(const GURL& possible_feed_or_site_url,
                                    FindFeedsCallback callback) {
  direct_feed_controller_.FindFeeds(possible_feed_or_site_url,
                                    std::move(callback));
}

void BraveNewsController::GetChannels(GetChannelsCallback callback) {
  if (!GetIsEnabled(prefs_)) {
    std::move(callback).Run({});
    return;
  }
  channels_controller_.GetAllChannels(std::move(callback));
}

void BraveNewsController::AddChannelsListener(
    mojo::PendingRemote<mojom::ChannelsListener> listener) {
  channels_controller_.AddListener(std::move(listener));
}

void BraveNewsController::SetChannelSubscribed(
    const std::string& locale,
    const std::string& channel_id,
    bool subscribed,
    SetChannelSubscribedCallback callback) {
  auto result =
      channels_controller_.SetChannelSubscribed(locale, channel_id, subscribed);

  // When channels are changed, see if it affects the feed.
  if (MaybeInitFeedV2()) {
    feed_v2_builder_->RecheckFeedHash();
  }

  std::move(callback).Run(std::move(result));
}

void BraveNewsController::SubscribeToNewDirectFeed(
    const GURL& feed_url,
    SubscribeToNewDirectFeedCallback callback) {
  // Verify the url points at a valid feed
  VLOG(1) << "SubscribeToNewDirectFeed: " << feed_url.spec();
  if (!feed_url.is_valid()) {
    std::move(callback).Run(false, false, std::nullopt);
    return;
  }
  direct_feed_controller_.VerifyFeedUrl(
      feed_url,
      base::BindOnce(
          [](const GURL& feed_url, SubscribeToNewDirectFeedCallback callback,
             BraveNewsController* controller, const bool is_valid,
             const std::string& feed_title) {
            VLOG(1) << "Is new feed valid? " << is_valid
                    << " Title: " << feed_title;
            if (!is_valid) {
              std::move(callback).Run(false, false, std::nullopt);
              return;
            }

            if (!controller->direct_feed_controller_.AddDirectFeedPref(
                    feed_url, feed_title)) {
              std::move(callback).Run(true, true, std::nullopt);
              return;
            }

            std::vector<mojom::PublisherPtr> direct_feeds;
            ParseDirectPublisherList(
                controller->prefs_->GetDict(prefs::kBraveNewsDirectFeeds),
                &direct_feeds);
            auto event = mojom::PublishersEvent::New();
            for (auto& feed : direct_feeds) {
              auto publisher_id = feed->publisher_id;
              event->addedOrUpdated[publisher_id] = std::move(feed);
            }

            for (const auto& listener : controller->publishers_listeners_) {
              listener->Changed(event->Clone());
            }

            // Mark feed as requiring update
            // TODO(petemill): expose function to mark direct feeds as dirty
            // and not require re-download of sources.json
            controller->publishers_controller_.EnsurePublishersIsUpdating();
            // Pass publishers to callback, waiting for updated publishers list
            controller->publishers_controller_.GetOrFetchPublishers(
                base::BindOnce(
                    [](SubscribeToNewDirectFeedCallback callback,
                       Publishers publishers) {
                      std::move(callback).Run(
                          true, false,
                          std::optional<Publishers>(std::move(publishers)));
                    },
                    std::move(callback)),
                true);

            controller->news_metrics_.RecordDirectFeedsTotal();
            controller->news_metrics_.RecordWeeklyAddedDirectFeedsCount(1);
          },
          feed_url, std::move(callback), base::Unretained(this)));
}

void BraveNewsController::RemoveDirectFeed(const std::string& publisher_id) {
  direct_feed_controller_.RemoveDirectFeedPref(publisher_id);

  // Mark feed as requiring update
  publishers_controller_.EnsurePublishersIsUpdating();

  news_metrics_.RecordDirectFeedsTotal();
  news_metrics_.RecordWeeklyAddedDirectFeedsCount(-1);

  for (const auto& receiver : publishers_listeners_) {
    auto event = mojom::PublishersEvent::New();
    event->removed.push_back(publisher_id);
    receiver->Changed(std::move(event));
  }
}

void BraveNewsController::GetImageData(const GURL& padded_image_url,
                                       GetImageDataCallback callback) {
  // Validate
  VLOG(2) << "getimagedata " << padded_image_url.spec();
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

                  auto bytes = result.bitmap_data;
                  std::vector<uint8_t> bytes_vec(
                      bytes->front_as<uint8_t>(),
                      bytes->front_as<uint8_t>() + bytes->size());
                  std::move(callback).Run(std::move(bytes_vec));
                },
                std::move(callback)),
            &controller->task_tracker_);
      },
      base::Unretained(this), publisher_id, std::move(callback)));
}

void BraveNewsController::SetPublisherPref(const std::string& publisher_id,
                                           mojom::UserEnabled new_status) {
  VLOG(1) << "set publisher pref: " << new_status;
  GetPublishers(base::BindOnce(
      [](const std::string& publisher_id, mojom::UserEnabled new_status,
         BraveNewsController* controller, Publishers publishers) {
        if (!publishers.contains(publisher_id)) {
          LOG(ERROR) << "Attempted to set publisher pref which didn't exist: "
                     << publisher_id;
          return;
        }
        const auto& publisher = publishers[publisher_id];
        if (publisher->type == mojom::PublisherType::DIRECT_SOURCE) {
          // TODO(petemill): possible allow disable or enable, but for now
          // the only thing to do with this type is to remove the direct feed
          // if requested.
          if (new_status == mojom::UserEnabled::DISABLED) {
            controller->RemoveDirectFeed(publisher_id);
          }
        } else {
          ScopedDictPrefUpdate update(controller->prefs_,
                                      prefs::kBraveNewsSources);
          if (new_status == mojom::UserEnabled::NOT_MODIFIED) {
            update->Remove(publisher_id);
          } else {
            update->Set(publisher_id,
                        (new_status == mojom::UserEnabled::ENABLED));
          }
          controller->publishers_controller_.EnsurePublishersIsUpdating();
        }
      },
      publisher_id, new_status, base::Unretained(this)));
}

void BraveNewsController::ClearPrefs() {
  ScopedDictPrefUpdate update(prefs_, prefs::kBraveNewsSources);
  update->clear();
  // Force an update of publishers and feed to include or ignore
  // content from the affected publisher.
  publishers_controller_.EnsurePublishersIsUpdating();
}

void BraveNewsController::IsFeedUpdateAvailable(
    const std::string& displayed_feed_hash,
    IsFeedUpdateAvailableCallback callback) {
  feed_controller_.DoesFeedVersionDiffer(displayed_feed_hash,
                                         std::move(callback));
}

void BraveNewsController::AddFeedListener(
    mojo::PendingRemote<mojom::FeedListener> listener) {
  if (MaybeInitFeedV2()) {
    feed_v2_builder_->AddListener(std::move(listener));
  } else {
    feed_controller_.AddListener(std::move(listener));
  }
}
void BraveNewsController::SetConfiguration(
    mojom::ConfigurationPtr configuration,
    SetConfigurationCallback callback) {
  prefs_->SetBoolean(prefs::kBraveNewsOptedIn, configuration->isOptedIn);
  prefs_->SetBoolean(prefs::kNewTabPageShowToday, configuration->showOnNTP);
  prefs_->SetBoolean(prefs::kBraveNewsOpenArticlesInNewTab,
                     configuration->openArticlesInNewTab);
  std::move(callback).Run();
}

void BraveNewsController::AddConfigurationListener(
    mojo::PendingRemote<mojom::ConfigurationListener> listener) {
  auto id = configuration_listeners_.Add(std::move(listener));

  auto event = mojom::Configuration::New();
  event->isOptedIn = prefs_->GetBoolean(prefs::kBraveNewsOptedIn);
  event->showOnNTP = prefs_->GetBoolean(prefs::kNewTabPageShowToday);
  event->openArticlesInNewTab =
      prefs_->GetBoolean(prefs::kBraveNewsOpenArticlesInNewTab);
  configuration_listeners_.Get(id)->Changed(std::move(event));
}

void BraveNewsController::GetDisplayAd(GetDisplayAdCallback callback) {
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
         std::optional<base::Value::Dict> ad_data) {
        if (!ad_data) {
          VLOG(1) << "GetDisplayAd: no ad";
          std::move(callback).Run(nullptr);
          return;
        }
        VLOG(1) << "GetDisplayAd: GOT ad";
        // Convert to our mojom entity.
        // TODO(petemill): brave_ads seems to use mojom, perhaps we can receive
        // and send to callback the actual typed mojom struct from brave_ads?
        auto ad = mojom::DisplayAd::New();
        ad->uuid = *ad_data->FindString("uuid");
        ad->creative_instance_id = *ad_data->FindString("creativeInstanceId");
        if (const auto* value = ad_data->FindString("ctaText")) {
          ad->cta_text = *value;
        }
        ad->dimensions = *ad_data->FindString("dimensions");
        ad->title = *ad_data->FindString("title");
        ad->description = *ad_data->FindString("description");
        ad->image = mojom::Image::NewPaddedImageUrl(
            GURL(*ad_data->FindString("imageUrl")));
        ad->target_url = GURL(*ad_data->FindString("targetUrl"));
        std::move(callback).Run(std::move(ad));
      },
      std::move(callback));
  ads_service_->MaybeServeInlineContentAd("900x750", std::move(on_ad_received));
}

void BraveNewsController::OnInteractionSessionStarted() {
  news_metrics_.RecordAtSessionStart();
}

void BraveNewsController::OnPromotedItemView(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  if (ads_service_ && !item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->TriggerPromotedContentAdEvent(
        item_id, creative_instance_id,
        brave_ads::mojom::PromotedContentAdEventType::kViewed,
        /*intentional*/ base::DoNothing());
  }
}

void BraveNewsController::OnPromotedItemVisit(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  if (ads_service_ && !item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->TriggerPromotedContentAdEvent(
        item_id, creative_instance_id,
        brave_ads::mojom::PromotedContentAdEventType::kClicked,
        /*intentional*/ base::DoNothing());
  }
}

void BraveNewsController::OnNewCardsViewed(uint16_t card_views) {
  news_metrics_.RecordTotalActionCount(p3a::ActionType::kCardView, card_views);
}

void BraveNewsController::OnCardVisited(uint32_t depth) {
  news_metrics_.RecordTotalActionCount(p3a::ActionType::kCardVisit, 1);
  news_metrics_.RecordVisitCardDepth(depth);
}

void BraveNewsController::OnSidebarFilterUsage() {
  news_metrics_.RecordTotalActionCount(p3a::ActionType::kSidebarFilterUsage, 1);
}

void BraveNewsController::OnDisplayAdVisit(
    const std::string& item_id,
    const std::string& creative_instance_id) {
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
      brave_ads::mojom::InlineContentAdEventType::kViewed,
      /*intentional*/ base::DoNothing());

  news_metrics_.RecordWeeklyDisplayAdsViewedCount(true);
}

void BraveNewsController::CheckForPublishersUpdate() {
  if (!GetIsEnabled(prefs_)) {
    return;
  }
  publishers_controller_.EnsurePublishersIsUpdating();
}

void BraveNewsController::CheckForFeedsUpdate() {
  if (!GetIsEnabled(prefs_)) {
    return;
  }
  feed_controller_.UpdateIfRemoteChanged();
  EnsureFeedV2IsUpdating();
}

void BraveNewsController::Prefetch() {
  VLOG(1) << "PREFETCHING: ensuring feed has been retrieved";

  if (MaybeInitFeedV2()) {
    feed_v2_builder_->BuildAllFeed(base::DoNothing());
  } else {
    feed_controller_.EnsureFeedIsCached();
  }
}

void BraveNewsController::OnOptInChange() {
  p3a_enabled_report_timer_.Start(
      FROM_HERE, kP3AEnabledReportTimeDelay,
      base::BindOnce(&p3a::NewsMetrics::RecordFeatureEnabledChange,
                     base::Unretained(&news_metrics_)));
  ConditionallyStartOrStopTimer();

  auto event = mojom::Configuration::New();
  event->isOptedIn = prefs_->GetBoolean(prefs::kBraveNewsOptedIn);
  event->showOnNTP = prefs_->GetBoolean(prefs::kNewTabPageShowToday);
  event->openArticlesInNewTab =
      prefs_->GetBoolean(prefs::kBraveNewsOpenArticlesInNewTab);
  for (const auto& listener : configuration_listeners_) {
    listener->Changed(event->Clone());
  }
}

void BraveNewsController::ConditionallyStartOrStopTimer() {
  // If the user has just enabled the feature for the first time,
  // make sure we're setup or migrated.
  MaybeInitPrefs();
  // Refresh data on an interval only if Brave News is enabled
  if (GetIsEnabled(prefs_)) {
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

    GetPublishers(base::BindOnce(
        [](BraveNewsController* controller, Publishers publishers) {
          auto event = brave_news::mojom::PublishersEvent::New();
          event->addedOrUpdated = std::move(publishers);
          for (const auto& listener : controller->publishers_listeners_) {
            listener->Changed(event->Clone());
          }
        },
        base::Unretained(this)));
  } else {
    VLOG(1) << "STOPPING TIMERS";
    timer_feed_update_.Stop();
    timer_publishers_update_.Stop();
    timer_prefetch_.Stop();
    VLOG(1) << "REMOVING DATA FROM MEMORY";
    feed_controller_.ClearCache();
    publishers_controller_.ClearCache();
    feed_v2_builder_ = nullptr;
  }
}

void BraveNewsController::HandleSubscriptionsChanged() {
  if (GetIsEnabled(prefs_)) {
    VLOG(1) << "HandleSubscriptionsChanged: Ensuring feed is updated";
    feed_controller_.EnsureFeedIsUpdating();
  } else {
    VLOG(1) << "HandleSubscriptionsChanged: News not enabled, doing nothing.";
  }
}

void BraveNewsController::MaybeInitPrefs() {
  if (GetIsEnabled(prefs_)) {
    // We had a bug where you could be subscribed to a channel in the empty
    // locale in earlier versions of Brave News. If so, we should remove it.
    // After this has been out for a bit we can remove it.
    // https://github.com/brave/brave-browser/issues/26596
    if (prefs_->GetDict(prefs::kBraveNewsChannels).contains("")) {
      ScopedDictPrefUpdate update(prefs_, prefs::kBraveNewsChannels);
      update->Remove("");
    }

    const auto& channels = prefs_->GetDict(prefs::kBraveNewsChannels);
    if (channels.empty()) {
      publishers_controller_.GetLocale(base::BindOnce(
          [](ChannelsController* channels_controller,
             const std::string& locale) {
            // This could happen, if we're offline, or the API is down at the
            // moment.
            if (locale.empty()) {
              return;
            }
            channels_controller->SetChannelSubscribed(locale,
                                                      kTopSourcesChannel, true);
          },
          base::Unretained(&channels_controller_)));
    }
  }
}

void BraveNewsController::OnPublishersUpdated(
    brave_news::PublishersController*) {
  // TODO(fallaciousreasoning): Make this more granular. It's fine for now
  // but we should be able to just let the frontend know what's changed.
  auto event = mojom::PublishersEvent::New();
  for (const auto& [id, publisher] :
       publishers_controller_.GetLastPublishers()) {
    event->addedOrUpdated.insert({id, publisher->Clone()});
  }

  for (const auto& observer : publishers_listeners_) {
    observer->Changed(event->Clone());
  }
}

void BraveNewsController::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (!GetIsEnabled(prefs_)) {
    return;
  }

  // Ensure publishers are fetched (this won't do anything if they are). This
  // handles the case where Brave News is started with no network.
  publishers_controller_.GetOrFetchPublishers(base::DoNothing());
}

}  // namespace brave_news
