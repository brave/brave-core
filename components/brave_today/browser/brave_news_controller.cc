// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_controller.h"

#include <cmath>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "brave/components/brave_today/browser/network.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

// static
void BraveNewsController::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  // Only default brave today to be shown for
  // certain languages on browser startup.
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string language_code = brave_l10n::GetLanguageCode(locale);
  const bool is_english_language = language_code == "en";
  const bool is_japanese_language = language_code == "ja";
  const bool brave_news_enabled_default =
      is_english_language || is_japanese_language;
  registry->RegisterBooleanPref(prefs::kNewTabPageShowToday,
                                brave_news_enabled_default);
  registry->RegisterBooleanPref(prefs::kBraveTodayOptedIn, false);
  registry->RegisterDictionaryPref(prefs::kBraveTodaySources);
  // P3A
  registry->RegisterListPref(prefs::kBraveTodayWeeklySessionCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardViewsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardVisitsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyDisplayAdViewedCount);
}

BraveNewsController::BraveNewsController(
    PrefService* prefs,
    brave_ads::AdsService* ads_service,
    history::HistoryService* history_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs),
      ads_service_(ads_service),
      api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      publishers_controller_(prefs, &api_request_helper_),
      feed_controller_(&publishers_controller_,
                       history_service,
                       &api_request_helper_),
      weak_ptr_factory_(this) {
  DCHECK(prefs);
  // Set up preference listeners
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      prefs::kNewTabPageShowToday,
      base::BindRepeating(&BraveNewsController::ConditionallyStartOrStopTimer,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveTodayOptedIn,
      base::BindRepeating(&BraveNewsController::ConditionallyStartOrStopTimer,
                          base::Unretained(this)));
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

void BraveNewsController::GetFeed(GetFeedCallback callback) {
  feed_controller_.GetOrFetchFeed(std::move(callback));
}

void BraveNewsController::GetPublishers(GetPublishersCallback callback) {
  publishers_controller_.GetOrFetchPublishers(std::move(callback));
}

void BraveNewsController::GetImageData(const GURL& padded_image_url,
                                       GetImageDataCallback callback) {
  // Handler url download response
  auto onPaddedImageResponse = base::BindOnce(
      [](GetImageDataCallback callback, const int status,
         const std::string& body,
         const base::flat_map<std::string, std::string>& headers) {
        // Attempt to remove byte padding
        base::StringPiece body_payload(body.data(), body.size());
        if (status < 200 || status >= 300 ||
            !brave::PrivateCdnHelper::GetInstance()->RemovePadding(
                &body_payload)) {
          // Byte padding removal failed
          absl::optional<std::vector<uint8_t>> args;
          std::move(callback).Run(std::move(args));
        }
        // Unpadding was successful, uint8Array will be easier to move over
        // mojom
        std::vector<uint8_t> image_bytes(body_payload.begin(),
                                         body_payload.end());
        std::move(callback).Run(image_bytes);
      },
      std::move(callback));
  api_request_helper_.Request("GET", padded_image_url, "", "", true,
                              std::move(onPaddedImageResponse),
                              brave::private_cdn_headers);
}

void BraveNewsController::SetPublisherPref(const std::string& publisher_id,
                                           mojom::UserEnabled new_status) {
  DictionaryPrefUpdate update(prefs_, prefs::kBraveTodaySources);
  if (new_status == mojom::UserEnabled::NOT_MODIFIED) {
    update->RemoveKey(publisher_id);
  } else {
    update->SetBoolean(publisher_id,
                       (new_status == mojom::UserEnabled::ENABLED));
  }
  VLOG(1) << "set publisher pref: " << new_status;
  // Force an update of publishers and feed to include or ignore
  // content from the affected publisher.
  // And if in the middle of update, that's ok because
  // consideration of source preferences is done after the remote fetch is
  // completed.
  publishers_controller_.EnsurePublishersIsUpdating();
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
  }
  auto on_ad_received = base::BindOnce(
      [](GetDisplayAdCallback callback, const bool success,
         const std::string& dimensions, const base::DictionaryValue& ad_data) {
        if (!success) {
          VLOG(1) << "GetDisplayAd: no ad";
          std::move(callback).Run(nullptr);
          return;
        }
        VLOG(1) << "GetDisplayAd: GOT ad";
        // Convert to our mojom entity.
        // TODO(petemill): brave_ads seems to use mojom, perhaps we can receive
        // and send to callback the actual typed mojom struct from brave_ads?
        auto ad = mojom::DisplayAd::New();
        ad->uuid = *ad_data.FindStringKey("uuid");
        ad->creative_instance_id = *ad_data.FindStringKey("creativeInstanceId");
        if (ad_data.HasKey("ctaText"))
          ad->cta_text = *ad_data.FindStringKey("ctaText");
        ad->dimensions = *ad_data.FindStringKey("dimensions");
        ad->title = *ad_data.FindStringKey("title");
        ad->description = *ad_data.FindStringKey("description");
        ad->image = mojom::Image::NewPaddedImageUrl(
            GURL(*ad_data.FindStringKey("imageUrl")));
        ad->target_url = GURL(*ad_data.FindStringKey("targetUrl"));
        std::move(callback).Run(std::move(ad));
      },
      std::move(callback));
  ads_service_->GetInlineContentAd("900x750", std::move(on_ad_received));
}

void BraveNewsController::OnInteractionSessionStarted() {
  // Track if user has ever scrolled to Brave Today.
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.HasEverInteracted", 1, 1);
  // Track how many times in the past week
  // user has scrolled to Brave Today.
  WeeklyStorage session_count_storage(prefs_,
                                      prefs::kBraveTodayWeeklySessionCount);
  session_count_storage.AddDelta(1);
  uint64_t total_session_count = session_count_storage.GetWeeklySum();
  constexpr int kSessionCountBuckets[] = {0, 1, 3, 7, 12, 18, 25, 1000};
  const int* it_count =
      std::lower_bound(kSessionCountBuckets, std::end(kSessionCountBuckets),
                       total_session_count);
  int answer = it_count - kSessionCountBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklySessionCount", answer,
                             base::size(kSessionCountBuckets) + 1);
}

void BraveNewsController::OnSessionCardVisitsCountChanged(
    uint16_t cards_visited_session_total_count) {
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  WeeklyStorage storage(prefs_, prefs::kBraveTodayWeeklyCardVisitsCount);
  storage.ReplaceTodaysValueIfGreater(cards_visited_session_total_count);
  // Send the session with the highest count of cards viewed.
  uint64_t total = storage.GetHighestValueInWeek();
  constexpr int kBuckets[] = {0, 1, 3, 6, 10, 15, 100};
  const int* it_count = std::lower_bound(kBuckets, std::end(kBuckets), total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyMaxCardVisitsCount", answer,
                             base::size(kBuckets) + 1);
}

void BraveNewsController::OnPromotedItemView(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  if (ads_service_ && !item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->OnPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::PromotedContentAdEventType::kViewed);
  }
}

void BraveNewsController::OnPromotedItemVisit(
    const std::string& item_id,
    const std::string& creative_instance_id) {
  if (ads_service_ && !item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->OnPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::PromotedContentAdEventType::kClicked);
  }
}

void BraveNewsController::OnSessionCardViewsCountChanged(
    uint16_t cards_viewed_session_total_count) {
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  WeeklyStorage storage(prefs_, prefs::kBraveTodayWeeklyCardViewsCount);
  storage.ReplaceTodaysValueIfGreater(cards_viewed_session_total_count);
  // Send the session with the highest count of cards viewed.
  uint64_t total = storage.GetHighestValueInWeek();
  constexpr int kBuckets[] = {0, 1, 4, 12, 20, 40, 80, 1000};
  const int* it_count = std::lower_bound(kBuckets, std::end(kBuckets), total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyMaxCardViewsCount", answer,
                             base::size(kBuckets) + 1);
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
  ads_service_->OnInlineContentAdEvent(
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
  ads_service_->OnInlineContentAdEvent(
      item_id, creative_instance_id,
      ads::mojom::InlineContentAdEventType::kViewed);
  // Let p3a know an ad was viewed
  WeeklyStorage storage(prefs_, prefs::kBraveTodayWeeklyCardViewsCount);
  storage.AddDelta(1u);
  // Store current weekly total in p3a, ready to send on the next upload
  uint64_t total = storage.GetWeeklySum();
  constexpr int kBuckets[] = {0, 1, 4, 8, 14, 30, 60, 120};
  const int* it_count = std::lower_bound(kBuckets, std::end(kBuckets), total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyDisplayAdsViewedCount", answer,
                             base::size(kBuckets) + 1);
}

void BraveNewsController::CheckForPublishersUpdate() {
  publishers_controller_.EnsurePublishersIsUpdating();
}

void BraveNewsController::CheckForFeedsUpdate() {
  feed_controller_.UpdateIfRemoteChanged();
}

void BraveNewsController::Prefetch() {
  VLOG(1) << "PREFETCHING: ensuring feed has been retrieved";
  feed_controller_.EnsureFeedIsCached();
}

void BraveNewsController::ConditionallyStartOrStopTimer() {
  // Refresh data on an interval only if Brave News is enabled
  bool should_show = prefs_->GetBoolean(prefs::kNewTabPageShowToday);
  bool opted_in = prefs_->GetBoolean(prefs::kBraveTodayOptedIn);
  bool is_enabled = (should_show && opted_in);
  if (is_enabled) {
    VLOG(1) << "STARTING TIMERS";
    if (!timer_feed_update_.IsRunning()) {
      timer_feed_update_.Start(FROM_HERE, base::TimeDelta::FromHours(3), this,
                               &BraveNewsController::CheckForFeedsUpdate);
    }
    if (!timer_publishers_update_.IsRunning()) {
      timer_publishers_update_.Start(
          FROM_HERE, base::TimeDelta::FromDays(1), this,
          &BraveNewsController::CheckForPublishersUpdate);
    }
    if (!timer_prefetch_.IsRunning()) {
      timer_prefetch_.Start(FROM_HERE, base::TimeDelta::FromMinutes(1), this,
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

}  // namespace brave_news
