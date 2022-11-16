// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_controller.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/guid.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "brave/components/brave_private_cdn/private_cdn_request_helper.h"
#include "brave/components/brave_today/browser/brave_news_p3a.h"
#include "brave/components/brave_today/browser/channels_controller.h"
#include "brave/components/brave_today/browser/direct_feed_controller.h"
#include "brave/components/brave_today/browser/locales_helper.h"
#include "brave/components/brave_today/browser/network.h"
#include "brave/components/brave_today/browser/suggestions_controller.h"
#include "brave/components/brave_today/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_today/browser/urls.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_news {
namespace {

// The favicon size we desire. The favicons are rendered at 24x24 pixels but
// they look quite a bit nicer if we get a 48x48 pixel icon and downscale it.
constexpr uint32_t kDesiredFaviconSizePixels = 48;
}  // namespace

// static
void BraveNewsController::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kShouldShowToolbarButton, true);
  registry->RegisterBooleanPref(prefs::kNewTabPageShowToday,
                                IsUserInDefaultEnabledLocale());
  registry->RegisterBooleanPref(prefs::kBraveTodayOptedIn, false);
  registry->RegisterDictionaryPref(prefs::kBraveTodaySources);
  registry->RegisterDictionaryPref(prefs::kBraveNewsChannels);
  registry->RegisterDictionaryPref(prefs::kBraveTodayDirectFeeds);

  p3a::RegisterProfilePrefs(registry);
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
      direct_feed_controller_(prefs_, url_loader_factory),
      unsupported_publisher_migrator_(prefs_,
                                      &direct_feed_controller_,
                                      &api_request_helper_),
      publishers_controller_(prefs_,
                             &direct_feed_controller_,
                             &unsupported_publisher_migrator_,
                             &api_request_helper_),
      channels_controller_(prefs_, &publishers_controller_),
      feed_controller_(&publishers_controller_,
                       &direct_feed_controller_,
                       &channels_controller_,
                       history_service,
                       &api_request_helper_,
                       prefs_),
      suggestions_controller_(prefs_,
                              &publishers_controller_,
                              &api_request_helper_,
                              history_service),
      weak_ptr_factory_(this) {
  DCHECK(prefs_);
  // Set up preference listeners
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kNewTabPageShowToday,
      base::BindRepeating(&BraveNewsController::ConditionallyStartOrStopTimer,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveTodayOptedIn,
      base::BindRepeating(&BraveNewsController::ConditionallyStartOrStopTimer,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveNewsChannels,
      base::BindRepeating(&BraveNewsController::HandleSubscriptionsChanged,
                          base::Unretained(this)));

  p3a::RecordAtInit(prefs_);
  // Monitor kBraveTodaySources and update feed / publisher cache
  // Start timer of updating feeds, if applicable
  ConditionallyStartOrStopTimer();
}

BraveNewsController::~BraveNewsController() = default;

void BraveNewsController::Bind(
    mojo::PendingReceiver<mojom::BraveNewsController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveNewsController::ClearHistory() {
  // TODO(petemill): Clear history once/if we actually store
  // feed cache somewhere.
}

mojo::PendingRemote<mojom::BraveNewsController>
BraveNewsController::MakeRemote() {
  mojo::PendingRemote<mojom::BraveNewsController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BraveNewsController::GetLocale(GetLocaleCallback callback) {
  if (!GetIsEnabled()) {
    std::move(callback).Run("");
    return;
  }
  publishers_controller_.GetLocale(std::move(callback));
}

void BraveNewsController::GetFeed(GetFeedCallback callback) {
  if (!GetIsEnabled()) {
    std::move(callback).Run(brave_news::mojom::Feed::New());
    return;
  }
  feed_controller_.GetOrFetchFeed(std::move(callback));
}

void BraveNewsController::GetPublishers(GetPublishersCallback callback) {
  if (!GetIsEnabled()) {
    std::move(callback).Run({});
    return;
  }
  publishers_controller_.GetOrFetchPublishers(std::move(callback));
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
  if (!GetIsEnabled()) {
    std::move(callback).Run({});
    return;
  }
  channels_controller_.GetAllChannels(std::move(callback));
}

void BraveNewsController::SetChannelSubscribed(
    const std::string& locale,
    const std::string& channel_id,
    bool subscribed,
    SetChannelSubscribedCallback callback) {
  auto result =
      channels_controller_.SetChannelSubscribed(locale, channel_id, subscribed);
  std::move(callback).Run(std::move(result));
}

void BraveNewsController::SubscribeToNewDirectFeed(
    const GURL& feed_url,
    SubscribeToNewDirectFeedCallback callback) {
  // Verify the url points at a valid feed
  VLOG(1) << "SubscribeToNewDirectFeed: " << feed_url.spec();
  if (!feed_url.is_valid()) {
    std::move(callback).Run(false, false, absl::nullopt);
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
              std::move(callback).Run(false, false, absl::nullopt);
              return;
            }

            if (!controller->direct_feed_controller_.AddDirectFeedPref(
                    feed_url, feed_title)) {
              std::move(callback).Run(true, true, absl::nullopt);
              return;
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
                          absl::optional<Publishers>(std::move(publishers)));
                    },
                    std::move(callback)),
                true);

            p3a::RecordDirectFeedsTotal(controller->prefs_);
            p3a::RecordWeeklyAddedDirectFeedsCount(controller->prefs_, 1);
          },
          feed_url, std::move(callback), base::Unretained(this)));
}

void BraveNewsController::RemoveDirectFeed(const std::string& publisher_id) {
  direct_feed_controller_.RemoveDirectFeedPref(publisher_id);

  // Mark feed as requiring update
  publishers_controller_.EnsurePublishersIsUpdating();

  p3a::RecordDirectFeedsTotal(prefs_);
  p3a::RecordWeeklyAddedDirectFeedsCount(prefs_, -1);
}

void BraveNewsController::GetImageData(const GURL& padded_image_url,
                                       GetImageDataCallback callback) {
  // Validate
  VLOG(2) << "getimagedata " << padded_image_url.spec();
  if (!padded_image_url.is_valid()) {
    absl::optional<std::vector<uint8_t>> args;
    std::move(callback).Run(std::move(args));
    return;
  }
  // Use file ending to determine if response
  // will contain (Brave's PrivateCDN) padding or
  // be a direct image
  const auto file_name = padded_image_url.path();
  const std::string ending = ".pad";
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
            base::StringPiece body_payload(body.data(), body.size());
            if (response_code < 200 || response_code >= 300 ||
                (is_padded &&
                 !brave::PrivateCdnHelper::GetInstance()->RemovePadding(
                     &body_payload))) {
              // Byte padding removal failed
              std::move(callback).Run(absl::nullopt);
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
          std::move(callback).Run(absl::nullopt);
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
                    std::move(callback).Run(absl::nullopt);
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
          DictionaryPrefUpdate update(controller->prefs_,
                                      prefs::kBraveTodaySources);
          if (new_status == mojom::UserEnabled::NOT_MODIFIED) {
            update->RemoveKey(publisher_id);
          } else {
            update->SetBoolKey(publisher_id,
                               (new_status == mojom::UserEnabled::ENABLED));
          }
          // Force an update of publishers and feed to include or ignore
          // content from the affected publisher.
          // And if in the middle of update, that's ok because
          // consideration of source preferences is done after the remote fetch
          // is completed.
          controller->publishers_controller_.EnsurePublishersIsUpdating();
        }
      },
      publisher_id, new_status, base::Unretained(this)));
}

void BraveNewsController::ClearPrefs() {
  DictionaryPrefUpdate update(prefs_, prefs::kBraveTodaySources);
  update->DictClear();
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
         absl::optional<base::Value::Dict> ad_data) {
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
        if (const auto* value = ad_data->FindString("ctaText"))
          ad->cta_text = *value;
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
  p3a::RecordAtSessionStart(prefs_);
}

void BraveNewsController::OnSessionCardVisitsCountChanged(
    uint16_t cards_visited_session_total_count) {
  p3a::RecordWeeklyMaxCardVisitsCount(prefs_,
                                      cards_visited_session_total_count);
}

void BraveNewsController::OnPromotedItemView(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  if (ads_service_ && !item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->TriggerPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::PromotedContentAdEventType::kViewed);
  }
}

void BraveNewsController::OnPromotedItemVisit(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  if (ads_service_ && !item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->TriggerPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::PromotedContentAdEventType::kClicked);
  }
}

void BraveNewsController::OnSessionCardViewsCountChanged(
    uint16_t cards_viewed_session_total_count) {
  p3a::RecordWeeklyMaxCardViewsCount(prefs_, cards_viewed_session_total_count);
  p3a::RecordTotalCardViews(prefs_, cards_viewed_session_total_count);
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
      ads::mojom::InlineContentAdEventType::kClicked);
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
      ads::mojom::InlineContentAdEventType::kViewed);

  p3a::RecordWeeklyDisplayAdsViewedCount(prefs_, true);
}

void BraveNewsController::OnDisplayAdPurgeOrphanedEvents() {
  if (!ads_service_) {
    VLOG(1) << "News: Asked to purge orphaned ad events but there is no ads "
               "service for"
               "this profile!";
    return;
  }
  ads_service_->PurgeOrphanedAdEventsForType(
      ads::mojom::AdType::kInlineContentAd, base::DoNothing());
}

void BraveNewsController::CheckForPublishersUpdate() {
  if (!GetIsEnabled()) {
    return;
  }
  publishers_controller_.EnsurePublishersIsUpdating();
}

void BraveNewsController::CheckForFeedsUpdate() {
  if (!GetIsEnabled()) {
    return;
  }
  feed_controller_.UpdateIfRemoteChanged();
}

void BraveNewsController::Prefetch() {
  VLOG(1) << "PREFETCHING: ensuring feed has been retrieved";
  feed_controller_.EnsureFeedIsCached();
}

void BraveNewsController::ConditionallyStartOrStopTimer() {
  // If the user has just enabled the feature for the first time,
  // make sure we're setup or migrated.
  MaybeInitPrefs();
  // Refresh data on an interval only if Brave News is enabled
  if (GetIsEnabled()) {
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
  } else {
    VLOG(1) << "STOPPING TIMERS";
    timer_feed_update_.Stop();
    timer_publishers_update_.Stop();
    timer_prefetch_.Stop();
    VLOG(1) << "REMOVING DATA FROM MEMORY";
    feed_controller_.ClearCache();
    publishers_controller_.ClearCache();
  }
}

bool BraveNewsController::GetIsEnabled() {
  bool should_show = prefs_->GetBoolean(prefs::kNewTabPageShowToday);
  bool opted_in = prefs_->GetBoolean(prefs::kBraveTodayOptedIn);
  bool is_enabled = (should_show && opted_in);
  return is_enabled;
}

void BraveNewsController::HandleSubscriptionsChanged() {
  if (GetIsEnabled()) {
    VLOG(1) << "HandleSubscriptionsChanged: Ensuring feed is updated";
    feed_controller_.EnsureFeedIsUpdating();
  } else {
    VLOG(1) << "HandleSubscriptionsChanged: News not enabled, doing nothing.";
  }
}

void BraveNewsController::MaybeInitPrefs() {
  if (GetIsEnabled() && base::FeatureList::IsEnabled(
                            brave_today::features::kBraveNewsV2Feature)) {
    const auto& channels = prefs_->GetDict(prefs::kBraveNewsChannels);
    if (channels.empty() && GetIsEnabled()) {
      publishers_controller_.GetLocale(base::BindOnce(
          [](ChannelsController* channels_controller,
             const std::string& locale) {
            channels_controller->SetChannelSubscribed(locale,
                                                      kTopSourcesChannel, true);
          },
          base::Unretained(&channels_controller_)));
    }
  }
}

}  // namespace brave_news
