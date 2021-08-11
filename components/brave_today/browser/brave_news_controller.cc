// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_controller.h"


#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/common/pref_names.h" // TODO: move to this component
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "brave/components/brave_today/browser/urls.h"
#include "brave/components/brave_today/browser/feed_parsing.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {
  net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_news_controller", R"(
      semantics {
        sender: "Brave News Controller"
        description:
          "This controller is used to fetch brave news feeds and publisher lists."
        trigger:
          "Triggered by uses of the Brave News feature."
        data:
          "Article JSON"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on the New Tab Page customization."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

namespace brave_news {
using std::move;

BraveNewsController::BraveNewsController(PrefService* prefs,
    brave_ads::AdsService* ads_service,
    history::HistoryService* history_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs),
      ads_service_(ads_service),
      history_service_(history_service),
      api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      weak_ptr_factory_(this) {
  DCHECK(prefs);
  // Set up preference listeners
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(kNewTabPageShowToday,
      base::BindRepeating(&BraveNewsController::ConditionallyStartOrStopTimer,
          base::Unretained(this)));
  pref_change_registrar_.Add(kBraveTodayOptedIn,
      base::BindRepeating(&BraveNewsController::ConditionallyStartOrStopTimer,
          base::Unretained(this)));
  // Monitor kBraveTodaySources and update feed / publisher cache
  // Start timer of updating feeds, if applicable
  ConditionallyStartOrStopTimer();
}

BraveNewsController::~BraveNewsController() = default;

mojo::PendingRemote<mojom::BraveNewsController> BraveNewsController::MakeRemote(
    ) {
  mojo::PendingRemote<mojom::BraveNewsController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BraveNewsController::Bind(
    mojo::PendingReceiver<mojom::BraveNewsController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveNewsController::ClearHistory() {
  // Clear history once we actually store feed cache somewhere
}

void BraveNewsController::GetFeed(GetFeedCallback callback) {
  GetOrFetchFeed(std::move(callback));
}

void BraveNewsController::GetPublishers(GetPublishersCallback callback){
  GetOrFetchPublishers(std::move(callback));
}

void BraveNewsController::GetImageData(const GURL& padded_image_url,
    GetImageDataCallback callback){
  // Handler url download response
  auto onPaddedImageResponse = base::BindOnce(
    [](GetImageDataCallback callback, const int status, const std::string& body,
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
      // Unpadding was successful, uint8Array will be easier to move over mojom
      std::vector<uint8_t> image_bytes(body_payload.begin(), body_payload.end());
      std::move(callback).Run(image_bytes);
    }, std::move(callback));
  api_request_helper_.Request("GET", padded_image_url, "", "", true,
      std::move(onPaddedImageResponse));
}

void BraveNewsController::SetPublisherPref(
    const std::string& publisher_id,
    mojom::UserEnabled new_status){
  DictionaryPrefUpdate update(prefs_, kBraveTodaySources);
  if (new_status == mojom::UserEnabled::NOT_MODIFIED) {
    update->RemoveKey(publisher_id);
  } else {
    update->SetBoolean(publisher_id,
        (new_status == mojom::UserEnabled::ENABLED));
  }
  LOG(ERROR) << "set publisher pref";
  // Force an update of publishers and feed to include or ignore
  // content from the affected publisher.
  PublishersIsStale();
}

void BraveNewsController::ClearPrefs(){
  DictionaryPrefUpdate update(prefs_, kBraveTodaySources);
  update->DictClear();
}

void BraveNewsController::IsFeedUpdateAvailable(
    const std::string& displayed_feed_hash,
    IsFeedUpdateAvailableCallback callback){
  // Get cached feed and compare hash
  auto onFeed = base::BindOnce(
    [](BraveNewsController* controller, IsFeedUpdateAvailableCallback callback,
      std::string original_hash, mojom::FeedPtr feed) {
        std::move(callback).Run(original_hash != feed->hash);
    }, base::Unretained(this), std::move(callback), displayed_feed_hash);
  GetOrFetchFeed(std::move(onFeed));
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
    }, std::move(callback));
    ads_service_->GetInlineContentAd("900x750", std::move(on_ad_received));
}

void BraveNewsController::OnInteractionSessionStarted() {
  // Track if user has ever scrolled to Brave Today.
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.HasEverInteracted", 1, 1);
  // Track how many times in the past week
  // user has scrolled to Brave Today.
  WeeklyStorage session_count_storage(prefs_, kBraveTodayWeeklySessionCount);
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
    uint16_t cards_visited_session_total_count){
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  WeeklyStorage storage(prefs_, kBraveTodayWeeklyCardVisitsCount);
  storage.ReplaceTodaysValueIfGreater(cards_visited_session_total_count);
  // Send the session with the highest count of cards viewed.
  uint64_t total = storage.GetHighestValueInWeek();
  constexpr int kBuckets[] = {0, 1, 3, 6, 10, 15, 100};
  const int* it_count =
      std::lower_bound(kBuckets, std::end(kBuckets),
                      total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyMaxCardVisitsCount", answer,
                             base::size(kBuckets) + 1);
}

void BraveNewsController::OnPromotedItemView(const std::string &item_id,
    const std::string &creative_instance_id) {
  if (!item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->OnPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::PromotedContentAdEventType::kViewed);
  }
}

void BraveNewsController::OnPromotedItemVisit(const std::string &item_id,
    const std::string &creative_instance_id) {
  if (!item_id.empty() && !creative_instance_id.empty()) {
    ads_service_->OnPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::PromotedContentAdEventType::kClicked);
  }
}

void BraveNewsController::OnSessionCardViewsCountChanged(
    uint16_t cards_viewed_session_total_count){
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  WeeklyStorage storage(prefs_, kBraveTodayWeeklyCardViewsCount);
  storage.ReplaceTodaysValueIfGreater(cards_viewed_session_total_count);
  // Send the session with the highest count of cards viewed.
  uint64_t total = storage.GetHighestValueInWeek();
  constexpr int kBuckets[] = {0, 1, 4, 12, 20, 40, 80, 1000};
  const int* it_count =
      std::lower_bound(kBuckets, std::end(kBuckets),
                      total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyMaxCardViewsCount", answer,
                             base::size(kBuckets) + 1);
}

void BraveNewsController::OnDisplayAdVisit(
    const std::string &item_id, const std::string &creative_instance_id) {
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
    const std::string &item_id, const std::string &creative_instance_id) {
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
  WeeklyStorage storage(prefs_, kBraveTodayWeeklyCardViewsCount);
  storage.AddDelta(1u);
  // Store current weekly total in p3a, ready to send on the next upload
  uint64_t total = storage.GetWeeklySum();
  constexpr int kBuckets[] = {0, 1, 4, 8, 14, 30, 60, 120};
  const int* it_count = std::lower_bound(kBuckets, std::end(kBuckets), total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyDisplayAdsViewedCount", answer,
                             base::size(kBuckets) + 1);
}

void BraveNewsController::CheckForFeedsUpdate() {

}

void BraveNewsController::CheckForSourcesUpdate() {

}

void BraveNewsController::PublishersIsStale() {
  Publishers new_publishers;
  publishers_ = std::move(new_publishers);
  FeedIsStale();
}

void BraveNewsController::FeedIsStale() {
  ResetFeed();
  GetFeedCallback do_nothing = base::BindOnce([](mojom::FeedPtr feed){});
  GetOrFetchFeed(std::move(do_nothing));
}

void BraveNewsController::ResetFeed() {
  current_feed_.featured_article = nullptr;
  current_feed_.hash = "";
  current_feed_.pages.clear();
}

void BraveNewsController::GetOrFetchFeed(GetFeedCallback callback) {
  if (!current_feed_.hash.empty()) {
    auto clone = current_feed_.Clone();
    std::move(callback).Run(std::move(clone));
    return;
  }
  UpdateFeed(std::move(callback));
}
void BraveNewsController::GetOrFetchPublishers(GetPublishersCallback callback) {
  // Use memory cache if available
  if (!publishers_.empty()) {
    ProvidePublishersClone(std::move(callback));
    // std::move(callback).Run(publishers_);
    return;
  }
  // TODO: make sure only one at a time
  // fetching_feed_future_.wait();
  // fetching_feed_future_.get()
  // Re-check memory cache now that possible request is finished
  // if (!publishers_.empty()) {
  //   std::move(callback).Run(publishers_);
  //   return;
  // }
  // Perform fetch
  UpdatePublishers(std::move(callback));
}

void BraveNewsController::UpdateFeed(GetFeedCallback callback) {
  GURL feed_url("https://" + brave_today::GetHostname() + "/brave-today/feed." + brave_today::GetRegionUrlPart() + "json");
  auto onRequest = base::BindOnce(
    [](BraveNewsController* controller, GetFeedCallback callback,
      const int status, const std::string& body,
      const base::flat_map<std::string, std::string>& headers) {
        VLOG(1) << "Downloaded feed, status: " << status;
        // TODO(petemill): handle bad response
        // TODO(petemill): avoid callback hell
        auto onPublishers = base::BindOnce(
          [](BraveNewsController* controller, GetFeedCallback callback,
            const std::string& body, Publishers publishers) {
              // TODO(petemill): Handle no publishers
              // Get history hosts
              // TODO(petemill): avoid callback hell
              auto onHistory = base::BindOnce(
                [](BraveNewsController* controller, GetFeedCallback callback,
                    const std::string& body, Publishers publishers,
                    history::QueryResults results) {
                  std::unordered_set<std::string> history_hosts;
                  for (const auto &item : results) {
                    auto host = item.url().host();
                    history_hosts.insert(host);
                  }
                  VLOG(1) << "history hosts # " << history_hosts.size();
                  controller->ResetFeed();
                  ParseFeed(body, &publishers, history_hosts, &controller->current_feed_);
                  auto clone = controller->current_feed_.Clone();
                  std::move(callback).Run(std::move(clone));
                }, base::Unretained(controller), std::move(callback),
                    std::move(body), std::move(publishers));
              history::QueryOptions options;
              options.max_count = 2000;
              options.SetRecentDayRange(14);
              controller->history_service_->QueryHistory(
                  std::u16string(), options, std::move(onHistory),
                  &controller->task_tracker_);
            }, base::Unretained(controller),
              std::move(callback), std::move(body));
        controller->GetOrFetchPublishers(std::move(onPublishers));
    }, base::Unretained(this), std::move(callback));
  api_request_helper_.Request("GET", feed_url, "", "", true,
        std::move(onRequest));
}

void BraveNewsController::UpdatePublishers(GetPublishersCallback callback) {
  GURL sources_url("https://" + brave_today::GetHostname() + "/sources." + brave_today::GetRegionUrlPart() + "json");
  auto onRequest = base::BindOnce(
    [](BraveNewsController* controller, GetPublishersCallback callback,
      const int status, const std::string& body,
      const base::flat_map<std::string, std::string>& headers) {
          VLOG(1) << "Downloaded sources, status: " << status;
          // TODO(petemill): handle bad status or response
          Publishers publisher_list;
          ParsePublisherList(body, &publisher_list);
          // Add user enabled statuses
          const base::DictionaryValue* publisher_prefs =
              controller->prefs_->GetDictionary(kBraveTodaySources);
          for (auto kv : publisher_prefs->DictItems()) {
            auto publisher_id = kv.first;
            auto is_user_enabled = kv.second.GetIfBool();
            if (publisher_list.contains(publisher_id) && is_user_enabled.has_value()) {
              publisher_list[publisher_id]->user_enabled_status =
                (is_user_enabled.value() ? brave_news::mojom::UserEnabled::ENABLED
                                : brave_news::mojom::UserEnabled::DISABLED);

            } else {
              VLOG(1) << "Publisher list did not contain publisher found in"
              "user prefs: " << publisher_id;
            }
          }
          // Set memory cache
          controller->publishers_ = std::move(publisher_list);
          controller->ProvidePublishersClone(std::move(callback));
          // TODO: notify other callbacks / Release lock
          // controller->fetching_sources_lock_.Release();
          // std::move(callback).Run(controller->publishers_);
    }, base::Unretained(this), std::move(callback));
  api_request_helper_.Request("GET", sources_url, "", "", true,
        std::move(onRequest));
}

void BraveNewsController::ProvidePublishersClone(GetPublishersCallback cb) {
  Publishers clone;
  for (auto const& kv : publishers_) {
    clone.insert_or_assign(kv.first, kv.second->Clone());
  }
  std::move(cb).Run(std::move(clone));
}

void BraveNewsController::ConditionallyStartOrStopTimer() {
  // Refresh data on an interval only if Brave News is enabled
  bool should_show = prefs_->GetBoolean(kNewTabPageShowToday);
  bool opted_in = prefs_->GetBoolean(kBraveTodayOptedIn);
  bool is_enabled = (should_show && opted_in);
  if (is_enabled) {
    VLOG(1) << "STARTING TIMERS";
    if (!timer_feed_update_.IsRunning()) {
      timer_feed_update_.Start(FROM_HERE, base::TimeDelta::FromHours(3),
          this, &BraveNewsController::CheckForFeedsUpdate);
    }
    if (!timer_publishers_update_.IsRunning()) {
      timer_publishers_update_.Start(FROM_HERE, base::TimeDelta::FromDays(1),
          this, &BraveNewsController::CheckForSourcesUpdate);
    }
  } else {
    VLOG(1) << "STOPPING TIMERS";
    timer_feed_update_.Stop();
    timer_publishers_update_.Stop();
  }
}

}  // namespace brave_news
