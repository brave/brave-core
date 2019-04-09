/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>
#include <vector>
#include <algorithm>
#include <utility>

#include "bat/ads/ads_client.h"
#include "bat/ads/notification_info.h"
#include "bat/ads/confirmation_type.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/search_providers.h"
#include "bat/ads/internal/locale_helper.h"
#include "bat/ads/internal/uri_helper.h"
#include "bat/ads/internal/time_helper.h"
#include "bat/ads/internal/static_values.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "base/rand_util.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ads {

AdsImpl::AdsImpl(AdsClient* ads_client) :
    is_first_run_(true),
    is_foreground_(false),
    media_playing_({}),
    last_shown_tab_id_(0),
    last_shown_tab_url_(""),
    previous_tab_url_(""),
    page_score_cache_({}),
    last_shown_notification_info_(NotificationInfo()),
    collect_activity_timer_id_(0),
    delivering_notifications_timer_id_(0),
    sustained_ad_interaction_timer_id_(0),
    next_easter_egg_timestamp_in_seconds_(0),
    client_(std::make_unique<Client>(this, ads_client)),
    bundle_(std::make_unique<Bundle>(this, ads_client)),
    ads_serve_(std::make_unique<AdsServe>(this, ads_client, bundle_.get())),
    user_model_(nullptr),
    is_initialized_(false),
    is_confirmations_ready_(false),
    ads_client_(ads_client) {
}

AdsImpl::~AdsImpl() = default;

void AdsImpl::Initialize() {
  if (!ads_client_->IsAdsEnabled()) {
    BLOG(INFO) << "Deinitializing as Ads are disabled";

    Deinitialize();
    return;
  }

  if (IsInitialized()) {
    BLOG(INFO) << "Already initialized";

    return;
  }

  client_->LoadState();
}

void AdsImpl::InitializeStep2() {
  client_->SetLocales(ads_client_->GetLocales());

  LoadUserModel();
}

void AdsImpl::InitializeStep3() {
  is_initialized_ = true;

  BLOG(INFO) << "Successfully initialized";

  is_foreground_ = ads_client_->IsForeground();

  ads_client_->SetIdleThreshold(kIdleThresholdInSeconds);

  NotificationAllowedCheck(false);

  if (IsMobile()) {
    StartDeliveringNotifications(kDeliverNotificationsAfterSeconds);
  }

  ConfirmAdUUIDIfAdEnabled();

  ads_serve_->DownloadCatalog();
}

void AdsImpl::Deinitialize() {
  if (!IsInitialized()) {
    BLOG(WARNING) << "Failed to deinitialize as not initialized";

    return;
  }

  BLOG(INFO) << "Deinitializing";

  ads_serve_->Reset();

  StopDeliveringNotifications();

  StopSustainingAdInteraction();

  RemoveAllHistory();

  bundle_->Reset();
  user_model_.reset();

  last_shown_notification_info_ = NotificationInfo();

  page_score_cache_.clear();

  is_first_run_ = true;
  is_initialized_ = false;
  is_foreground_ = false;
}

bool AdsImpl::IsInitialized() {
  if (!is_initialized_ ||
      !ads_client_->IsAdsEnabled() ||
      !user_model_->IsInitialized()) {
    return false;
  }

  return true;
}

void AdsImpl::LoadUserModel() {
  auto locale = client_->GetLocale();
  auto callback = std::bind(&AdsImpl::OnUserModelLoaded, this, _1, _2);
  ads_client_->LoadUserModelForLocale(locale, callback);
}

void AdsImpl::OnUserModelLoaded(const Result result, const std::string& json) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load user model";

    return;
  }

  BLOG(INFO) << "Successfully loaded user model";

  InitializeUserModel(json);

  if (!IsInitialized()) {
    InitializeStep3();
  }
}

void AdsImpl::InitializeUserModel(const std::string& json) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  BLOG(INFO) << "Initializing user model";

  user_model_.reset(usermodel::UserModel::CreateInstance());
  user_model_->InitializePageClassifier(json);

  BLOG(INFO) << "Initialized user model";
}

bool AdsImpl::IsMobile() const {
  ClientInfo client_info;
  ads_client_->GetClientInfo(&client_info);

  if (client_info.platform != ANDROID_OS && client_info.platform != IOS) {
    return false;
  }

  return true;
}

void AdsImpl::OnForeground() {
  is_foreground_ = true;
  GenerateAdReportingForegroundEvent();
}

void AdsImpl::OnBackground() {
  is_foreground_ = false;
  GenerateAdReportingBackgroundEvent();
}

bool AdsImpl::IsForeground() const {
  return is_foreground_;
}

void AdsImpl::OnIdle() {
  // TODO(Terry Mancey): Implement Log (#44)
  // 'Idle state changed', { idleState: action.get('idleState') }

  BLOG(INFO) << "Browser state changed to idle";
}

void AdsImpl::OnUnIdle() {
  // TODO(Terry Mancey): Implement Log (#44)
  // 'Idle state changed', { idleState: action.get('idleState') }

  BLOG(INFO) << "Browser state changed to unidle";

  client_->UpdateLastUserIdleStopTime();

  if (IsMobile()) {
    return;
  }

  NotificationAllowedCheck(true);
}

void AdsImpl::OnMediaPlaying(const int32_t tab_id) {
  auto tab = media_playing_.find(tab_id);
  if (tab != media_playing_.end()) {
    // Media is already playing for this tab
    return;
  }

  BLOG(INFO) << "OnMediaPlaying for tab id: " << tab_id;

  media_playing_.insert({tab_id, true});
}

void AdsImpl::OnMediaStopped(const int32_t tab_id) {
  auto tab = media_playing_.find(tab_id);
  if (tab == media_playing_.end()) {
    // Media is not playing for this tab
    return;
  }

  BLOG(INFO) << "OnMediaStopped for tab id: " << tab_id;

  media_playing_.erase(tab_id);
}

bool AdsImpl::IsMediaPlaying() const {
  if (media_playing_.empty()) {
    return false;
  }

  return true;
}

void AdsImpl::TabUpdated(
    const int32_t tab_id,
    const std::string& url,
    const bool is_active,
    const bool is_incognito) {
  if (is_incognito) {
    return;
  }

  client_->UpdateLastUserActivity();

  if (is_active) {
    BLOG(INFO) << "TabUpdated.IsFocused for tab id: " << tab_id
        << " and url: " << url;

    last_shown_tab_id_ = tab_id;
    previous_tab_url_ = last_shown_tab_url_;
    last_shown_tab_url_ = url;

    TestShoppingData(url);
    TestSearchState(url);

    FocusInfo focus_info;
    focus_info.tab_id = tab_id;
    GenerateAdReportingFocusEvent(focus_info);
  } else {
    BLOG(INFO) << "TabUpdated.IsBlurred for tab id: " << tab_id
        << " and url: " << url;

    BlurInfo blur_info;
    blur_info.tab_id = tab_id;
    GenerateAdReportingBlurEvent(blur_info);
  }
}

void AdsImpl::TabClosed(const int32_t tab_id) {
  BLOG(INFO) << "TabClosed for tab id: " << tab_id;

  OnMediaStopped(tab_id);

  DestroyInfo destroy_info;
  destroy_info.tab_id = tab_id;
  GenerateAdReportingDestroyEvent(destroy_info);
}

void AdsImpl::RemoveAllHistory() {
  client_->RemoveAllHistory();

  ConfirmAdUUIDIfAdEnabled();
}

void AdsImpl::ConfirmAdUUIDIfAdEnabled() {
  if (!ads_client_->IsAdsEnabled()) {
    StopCollectingActivity();
    return;
  }

  client_->UpdateAdUUID();

  if (_is_debug) {
    StartCollectingActivity(kDebugOneHourInSeconds);
  } else {
    StartCollectingActivity(base::Time::kSecondsPerHour);
  }
}

bool AdsImpl::IsSupportedRegion() {
  auto supported_regions = {"US", "CA", "DE", "FR", "GB"};

  auto locale = ads_client_->GetAdsLocale();
  auto region = helper::Locale::GetCountryCode(locale);

  if (std::find(supported_regions.begin(), supported_regions.end(), region)
      == supported_regions.end()) {
    return false;
  }

  return true;
}

void AdsImpl::SetConfirmationsIsReady(const bool is_ready) {
  is_confirmations_ready_ = is_ready;
}

void AdsImpl::ChangeLocale(const std::string& locale) {
  if (!IsInitialized()) {
    return;
  }

  auto locales = ads_client_->GetLocales();

  if (std::find(locales.begin(), locales.end(), locale) != locales.end()) {
    BLOG(INFO) << "Change Localed to " << locale;
    client_->SetLocale(locale);
  } else {
    std::string closest_match_for_locale = "";
    auto language_code = helper::Locale::GetLanguageCode(locale);
    if (std::find(locales.begin(), locales.end(),
        language_code) != locales.end()) {
      closest_match_for_locale = language_code;
    } else {
      closest_match_for_locale = kDefaultLanguageCode;
    }

    BLOG(INFO) << "Locale not found, so changed Locale to closest match: "
        << closest_match_for_locale;
    client_->SetLocale(closest_match_for_locale);
  }

  LoadUserModel();
}

void AdsImpl::ClassifyPage(const std::string& url, const std::string& html) {
  if (!IsInitialized()) {
    BLOG(INFO) << "Site visited " << url << ", not initialized";

    return;
  }

  if (UrlHostsMatch(url, last_shown_notification_info_.url)) {
    BLOG(INFO) << "Site visited " << url
        << ", URL is from last shown notification";

    return;
  }

  if (!IsSupportedUrl(url)) {
    BLOG(INFO) << "Site visited " << url << ", unsupported URL";

    return;
  }

  if (TestSearchState(url)) {
    BLOG(INFO) << "Site visited " << url << ", URL is a search engine";

    return;
  }

  TestShoppingData(url);

  auto page_score = user_model_->ClassifyPage(html);
  auto winning_category = GetWinningCategory(page_score);
  if (winning_category.empty()) {
    BLOG(INFO) << "Site visited " << url
        << ", not enough content to classify page";

    return;
  }

  client_->SetLastPageClassification(winning_category);

  client_->AppendPageScoreToPageScoreHistory(page_score);

  CachePageScore(last_shown_tab_url_, page_score);

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Site visited', { url, immediateWinner, winnerOverTime }

  auto winner_over_time_category = GetWinnerOverTimeCategory();

  BLOG(INFO) << "Site visited " << url << ", immediateWinner is "
      << winning_category << " and winnerOverTime is "
      << winner_over_time_category << ", previous tab url "
      << previous_tab_url_;

  if (last_shown_tab_url_ == url) {
    LoadInfo load_info;
    load_info.tab_id = last_shown_tab_id_;
    load_info.tab_url = last_shown_tab_url_;
    load_info.tab_classification = winning_category;
    GenerateAdReportingLoadEvent(load_info);
  }

  CheckEasterEgg(url);
}

std::string AdsImpl::GetWinnerOverTimeCategory() {
  auto page_score_history = client_->GetPageScoreHistory();
  if (page_score_history.size() == 0) {
    return "";
  }

  uint64_t count = page_score_history.front().size();

  std::vector<double> winner_over_time_page_score(count);
  std::fill(winner_over_time_page_score.begin(),
      winner_over_time_page_score.end(), 0);

  for (const auto& page_score : page_score_history) {
    if (page_score.size() != count) {
      return "";
    }

    for (size_t i = 0; i < page_score.size(); i++) {
      winner_over_time_page_score[i] += page_score[i];
    }
  }

  return GetWinningCategory(winner_over_time_page_score);
}

std::string AdsImpl::GetWinningCategory(
    const std::vector<double>& page_score) {
  return user_model_->WinningCategory(page_score);
}

std::string AdsImpl::GetWinningCategory(const std::string& html) {
  auto page_score = user_model_->ClassifyPage(html);
  return GetWinningCategory(page_score);
}

void AdsImpl::CachePageScore(
    const std::string& url,
    const std::vector<double>& page_score) {
  auto cached_page_score = page_score_cache_.find(url);

  if (cached_page_score == page_score_cache_.end()) {
    page_score_cache_.insert({url, page_score});
  } else {
    cached_page_score->second = page_score;
  }
}

void AdsImpl::TestShoppingData(const std::string& url) {
  if (!IsInitialized()) {
    return;
  }

  if (UrlHostsMatch(url, kShoppingStateUrl)) {
    client_->FlagShoppingState(url, 1.0);
  } else {
    client_->UnflagShoppingState();
  }
}

bool AdsImpl::TestSearchState(const std::string& url) {
  if (!IsInitialized()) {
    return false;
  }

  auto is_search_engine = SearchProviders::IsSearchEngine(url);
  if (is_search_engine) {
    client_->FlagSearchState(url, 1.0);
  } else {
    client_->UnflagSearchState(url);
  }

  return is_search_engine;
}

void AdsImpl::ServeSampleAd() {
  if (!IsInitialized()) {
    return;
  }

  auto callback = std::bind(&AdsImpl::OnLoadSampleBundle, this, _1, _2);
  ads_client_->LoadSampleBundle(callback);
}

void AdsImpl::OnLoadSampleBundle(
    const Result result,
    const std::string& json) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load sample bundle";

    return;
  }

  BLOG(INFO) << "Successfully loaded sample bundle";

  BundleState state;
  std::string error_description;
  std::string json_schema = ads_client_->LoadJsonSchema(_bundle_schema_name);
  auto json_result = state.FromJson(json, json_schema, &error_description);
  if (json_result != SUCCESS) {
    BLOG(ERROR) << "Failed to parse sample bundle (" << error_description
        << "): " << json;

    return;
  }

  // TODO(Terry Mancey): Sample bundle state should be persisted on the Client
  // in a database so that sample ads can be fetched from the database rather
  // than parsing the JSON each time, and be consistent with GetAds, therefore
  // the below code should be abstracted into GetAdForSampleCategory once the
  // necessary changes have been made in Brave Core by Brian Johnson

  auto categories = state.categories.begin();
  auto categories_count = state.categories.size();
  if (categories_count == 0) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no categories' }

    BLOG(INFO) << "Notification not made: No sample bundle categories";

    return;
  }

  auto category_rand = base::RandInt(0, categories_count - 1);
  std::advance(categories, static_cast<int64_t>(category_rand));

  auto category = categories->first;
  auto ads = categories->second;

  auto ads_count = ads.size();
  if (ads_count == 0) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ads for category', category }

    BLOG(INFO) << "Notification not made: No sample bundle ads found for \""
        << category << "\" sample category";

    return;
  }

  auto ad_rand = base::RandInt(0, ads_count - 1);
  auto ad = ads.at(ad_rand);

  ShowAd(ad, category);
}

void AdsImpl::CheckEasterEgg(const std::string& url) {
  if (!_is_testing) {
    return;
  }

  auto now_in_seconds = helper::Time::NowInSeconds();

  if (UrlHostsMatch(url, kEasterEggUrl) &&
      next_easter_egg_timestamp_in_seconds_ < now_in_seconds) {
    BLOG(INFO) << "Collect easter egg";

    CheckReadyAdServe(true);

    next_easter_egg_timestamp_in_seconds_ =
        now_in_seconds + kNextEasterEggStartsInSeconds;

    BLOG(INFO) << "Next easter egg available in "
        << next_easter_egg_timestamp_in_seconds_ << " seconds";
  }
}

void AdsImpl::CheckReadyAdServe(const bool forced) {
  if (!IsInitialized() || !bundle_->IsReady()) {
    BLOG(INFO) << "Notification not made: Not initialized";

    return;
  }

  if (!forced) {
    if (!is_confirmations_ready_) {
      BLOG(INFO) << "Notification not made: Confirmations not ready";

      return;
    }

    if (!IsMobile() && !IsForeground()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'not in foreground' }

      BLOG(INFO) << "Notification not made: Not in foreground";

      return;
    }

    if (IsMediaPlaying()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'media playing in browser' }

      BLOG(INFO) << "Notification not made: Media playing in browser";

      return;
    }

    if (!IsAllowedToShowAds()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'not allowed based on history' }

      BLOG(INFO) << "Notification not made: Not allowed based on history";

      return;
    }
  }

  auto category = GetWinnerOverTimeCategory();
  ServeAdFromCategory(category);
}

void AdsImpl::ServeAdFromCategory(const std::string& category) {
  BLOG(INFO) << "Notification for category " << category;

  std::string catalog_id = bundle_->GetCatalogId();
  if (catalog_id.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ad catalog' }

    BLOG(INFO) << "Notification not made: No ad catalog";

    return;
  }

  if (category.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no classified pages) for
    // winnerOverTime', category, winnerOverTime, arbitraryKey }

    BLOG(INFO) << "Notification not made: category is empty";

    return;
  }

  auto locale = ads_client_->GetAdsLocale();
  auto region = helper::Locale::GetCountryCode(locale);

  auto callback = std::bind(&AdsImpl::OnGetAds, this, _1, _2, _3);
  ads_client_->GetAds(category, callback);
}

void AdsImpl::OnGetAds(
    const Result result,
    const std::string& category,
    const std::vector<AdInfo>& ads) {
  if (result != SUCCESS) {
    auto pos = category.find_last_of('-');
    if (pos != std::string::npos) {
      std::string new_category = category.substr(0, pos);

      BLOG(INFO) << "Notification not made: No ads found in \"" << category
          << "\" category, trying again with \"" << new_category
          << "\" category";

      auto callback = std::bind(&AdsImpl::OnGetAds, this, _1, _2, _3);
      ads_client_->GetAds(new_category, callback);

      return;
    }

    if (ads.empty()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'no ads for category', category }

      BLOG(INFO) << "Notification not made: No ads found in \"" << category
          << "\" category";

      return;
    }
  }

  auto ads_unseen = GetUnseenAds(ads);

  if (ads_unseen.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ad (or permitted ad) for
    // winnerOverTime', category, winnerOverTime, arbitraryKey }

    BLOG(INFO) << "Notification not made: No ad (or permitted ad) for \""
        << category << "\" category";

    return;
  }

  auto rand = base::RandInt(0, ads_unseen.size() - 1);
  auto ad = ads_unseen.at(rand);
  ShowAd(ad, category);
}

std::vector<AdInfo> AdsImpl::GetUnseenAds(
    const std::vector<AdInfo>& ads) {
  std::vector<AdInfo> ads_unseen = {};

  for (const auto& ad : ads) {
    std::deque<uint64_t> creative_set = {};
    auto creative_set_history = client_->GetCreativeSetHistory();
    if (creative_set_history.find(ad.creative_set_id)
        != creative_set_history.end()) {
      creative_set = creative_set_history.at(ad.creative_set_id);
    }

    if (creative_set.size() >= ad.total_max) {
      continue;
    }

    auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

    if (!HistoryRespectsRollingTimeConstraint(
        creative_set, day_window, ad.per_day)) {
      continue;
    }

    std::deque<uint64_t> campaign = {};
    auto campaign_history = client_->GetCampaignHistory();
    if (campaign_history.find(ad.campaign_id)
        != campaign_history.end()) {
      campaign = campaign_history.at(ad.campaign_id);
    }

    if (!HistoryRespectsRollingTimeConstraint(
        campaign, day_window, ad.daily_cap)) {
      continue;
    }

    ads_unseen.push_back(ad);
  }

  return ads_unseen;
}

bool AdsImpl::IsAdValid(const AdInfo& ad_info) {
  if (ad_info.advertiser.empty() ||
      ad_info.notification_text.empty() ||
      ad_info.notification_url.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'incomplete ad information',
    // category, winnerOverTime, arbitraryKey, notificationUrl,
    // notificationText, advertiser

    BLOG(INFO) << "Notification not made: Incomplete ad information"
        << std::endl << "  advertiser: " << ad_info.advertiser
        << std::endl << "  notificationText: " << ad_info.notification_text
        << std::endl << "  notificationUrl: " << ad_info.notification_url
        << std::endl << "  creativeSetId: " << ad_info.notification_url
        << std::endl << "  uuid: " << ad_info.notification_url;

    return false;
  }

  return true;
}

bool AdsImpl::ShowAd(
    const AdInfo& ad_info,
    const std::string& category) {
  if (!IsAdValid(ad_info)) {
    return false;
  }

  auto notification_info = std::make_unique<NotificationInfo>();
  notification_info->advertiser = ad_info.advertiser;
  notification_info->category = category;
  notification_info->text = ad_info.notification_text;
  notification_info->url = helper::Uri::GetUri(ad_info.notification_url);
  notification_info->creative_set_id = ad_info.creative_set_id;
  notification_info->uuid = ad_info.uuid;

  last_shown_notification_info_ = NotificationInfo(*notification_info);

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Notification shown', {category, winnerOverTime, arbitraryKey,
  // notificationUrl, notificationText, advertiser, uuid, hierarchy}

  BLOG(INFO) << "Notification shown:"
      << std::endl << "  campaign_id: " << ad_info.campaign_id
      << std::endl << "  category: " << category
      << std::endl << "  winnerOverTime: " << GetWinnerOverTimeCategory()
      << std::endl << "  notificationUrl: " << notification_info->url
      << std::endl << "  notificationText: " << notification_info->text
      << std::endl << "  advertiser: " << notification_info->advertiser
      << std::endl << "  uuid: " << notification_info->uuid;

  ads_client_->ShowNotification(std::move(notification_info));

  client_->AppendCurrentTimeToAdsShownHistory();
  client_->AppendCurrentTimeToCreativeSetHistory(ad_info.creative_set_id);
  client_->AppendCurrentTimeToCampaignHistory(ad_info.campaign_id);

  return true;
}

bool AdsImpl::HistoryRespectsRollingTimeConstraint(
    const std::deque<uint64_t> history,
    const uint64_t seconds_window,
    const uint64_t allowable_ad_count) const {
  uint64_t recent_count = 0;

  auto now_in_seconds = helper::Time::NowInSeconds();

  for (const auto& timestamp_in_seconds : history) {
    if (now_in_seconds - timestamp_in_seconds < seconds_window) {
      recent_count++;
    }
  }

  if (recent_count <= allowable_ad_count) {
    return true;
  }

  return false;
}

bool AdsImpl::IsAllowedToShowAds() {
  auto ads_shown_history = client_->GetAdsShownHistory();

  auto hour_window = base::Time::kSecondsPerHour;
  auto hour_allowed = ads_client_->GetAdsPerHour();
  auto respects_hour_limit = HistoryRespectsRollingTimeConstraint(
      ads_shown_history, hour_window, hour_allowed);

  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;
  auto day_allowed = ads_client_->GetAdsPerDay();
  auto respects_day_limit = HistoryRespectsRollingTimeConstraint(
      ads_shown_history, day_window, day_allowed);

  auto minimum_wait_time = hour_window / hour_allowed;
  bool respects_minimum_wait_time = HistoryRespectsRollingTimeConstraint(
      ads_shown_history, minimum_wait_time, 0);

  return respects_hour_limit && respects_day_limit &&
      respects_minimum_wait_time;
}

void AdsImpl::StartCollectingActivity(const uint64_t start_timer_in) {
  StopCollectingActivity();

  collect_activity_timer_id_ = ads_client_->SetTimer(start_timer_in);
  if (collect_activity_timer_id_ == 0) {
    BLOG(ERROR) <<
        "Failed to start collecting activity due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start collecting activity in " << start_timer_in << " seconds";
}

void AdsImpl::CollectActivity() {
  if (!IsInitialized()) {
    return;
  }

  BLOG(INFO) << "Collect activity";

  ads_serve_->DownloadCatalog();
}

void AdsImpl::StopCollectingActivity() {
  if (!IsCollectingActivity()) {
    return;
  }

  BLOG(INFO) << "Stopped collecting activity";

  ads_client_->KillTimer(collect_activity_timer_id_);
  collect_activity_timer_id_ = 0;
}

bool AdsImpl::IsCollectingActivity() const {
  if (collect_activity_timer_id_ == 0) {
    return false;
  }

  return true;
}

void AdsImpl::StartDeliveringNotifications(const uint64_t start_timer_in) {
  StopDeliveringNotifications();

  delivering_notifications_timer_id_ = ads_client_->SetTimer(start_timer_in);
  if (delivering_notifications_timer_id_ == 0) {
    BLOG(ERROR) <<
        "Failed to start delivering notifications due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start delivering notifications in "
      << start_timer_in << " seconds";
}

void AdsImpl::DeliverNotification() {
  NotificationAllowedCheck(true);

  if (IsMobile()) {
    StartDeliveringNotifications(kDeliverNotificationsAfterSeconds);
  }
}

void AdsImpl::StopDeliveringNotifications() {
  if (!IsDeliveringNotifications()) {
    return;
  }

  BLOG(INFO) << "Stopped delivering notifications";

  ads_client_->KillTimer(delivering_notifications_timer_id_);
  delivering_notifications_timer_id_ = 0;
}

bool AdsImpl::IsDeliveringNotifications() const {
  if (delivering_notifications_timer_id_ == 0) {
    return false;
  }

  return true;
}

bool AdsImpl::IsCatalogOlderThanOneDay() {
  auto catalog_last_updated_timestamp_in_seconds =
    bundle_->GetCatalogLastUpdatedTimestampInSeconds();

  auto now_in_seconds = helper::Time::NowInSeconds();

  if (catalog_last_updated_timestamp_in_seconds != 0 &&
      now_in_seconds > catalog_last_updated_timestamp_in_seconds
          + (base::Time::kSecondsPerHour * base::Time::kHoursPerDay)) {
    return true;
  }

  return false;
}

void AdsImpl::BundleUpdated() {
  ads_serve_->UpdateNextCatalogCheck();
}

void AdsImpl::NotificationAllowedCheck(const bool serve) {
  auto ok = ads_client_->IsNotificationsAvailable();

  // TODO(Terry Mancey): Implement Log (#44)
  // appConstants.APP_ON_NATIVE_NOTIFICATION_AVAILABLE_CHECK, {err, result}

  auto previous = client_->GetAvailable();

  if (ok != previous) {
    client_->SetAvailable(ok);
  }

  if (!serve || ok != previous) {
    GenerateAdReportingSettingsEvent();
  }

  if (!serve) {
    return;
  }

  if (!ok) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Ad not served', { reason: 'notifications not presently allowed' }

    BLOG(INFO) << "Ad not served: Notifications not presently allowed";

    return;
  }

  if (!ads_client_->IsNetworkConnectionAvailable()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Ad not served', { reason: 'network connection not availaable' }

    BLOG(INFO) << "Ad not served: Network connection not available";

    return;
  }

  if (IsCatalogOlderThanOneDay()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Ad not served', { reason: 'catalog older than one day' }

    BLOG(INFO) << "Ad not served: Catalog older than one day";

    return;
  }

  CheckReadyAdServe(false);
}

void AdsImpl::StartSustainingAdInteraction(const uint64_t start_timer_in) {
  StopSustainingAdInteraction();

  sustained_ad_interaction_timer_id_ = ads_client_->SetTimer(start_timer_in);
  if (sustained_ad_interaction_timer_id_ == 0) {
    BLOG(ERROR) <<
        "Failed to start sustaining ad interaction due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start sustaining ad interaction in "
      << start_timer_in << " seconds";
}

void AdsImpl::SustainAdInteractionIfNeeded() {
  if (!IsStillViewingAd()) {
    BLOG(INFO) << "Failed to Sustain ad interaction";
    return;
  }

  BLOG(INFO) << "Sustained ad interaction";

  ConfirmAd(last_shown_notification_info_, ConfirmationType::LANDED);
}

void AdsImpl::StopSustainingAdInteraction() {
  if (!IsSustainingAdInteraction()) {
    return;
  }

  BLOG(INFO) << "Stopped sustaining ad interaction";

  ads_client_->KillTimer(sustained_ad_interaction_timer_id_);
  sustained_ad_interaction_timer_id_ = 0;
}

bool AdsImpl::IsSustainingAdInteraction() const {
  if (sustained_ad_interaction_timer_id_ == 0) {
    return false;
  }

  return true;
}

bool AdsImpl::IsStillViewingAd() const {
  if (!UrlHostsMatch(last_shown_tab_url_, last_shown_notification_info_.url)) {
    BLOG(INFO) << "IsStillViewingAd last_shown_notification_info_url: "
        << last_shown_notification_info_.url
        << " does not match last_shown_tab_url: " << last_shown_tab_url_;

    return false;
  }

  return true;
}

void AdsImpl::ConfirmAd(
    const NotificationInfo& info,
    const ConfirmationType type) {
  if (IsNotificationFromSampleCatalog(info)) {
    BLOG(INFO) << "Confirmation not made: Sample Ad";

    return;
  }

  auto notification_info = std::make_unique<NotificationInfo>(info);

  notification_info->type = type;

  GenerateAdReportingConfirmationEvent(*notification_info);

  ads_client_->ConfirmAd(std::move(notification_info));
}

void AdsImpl::OnTimer(const uint32_t timer_id) {
  BLOG(INFO) << "OnTimer: " << std::endl
      << "  timer_id: " << std::to_string(timer_id) << std::endl
      << "  collect_activity_timer_id_: "
      << std::to_string(collect_activity_timer_id_) << std::endl
      << "  deliverying_notifications_timer_id_: "
      << std::to_string(delivering_notifications_timer_id_) << std::endl
      << "  sustained_ad_interaction_timer_id_: "
      << std::to_string(sustained_ad_interaction_timer_id_);
  if (timer_id == collect_activity_timer_id_) {
    CollectActivity();
  } else if (timer_id == delivering_notifications_timer_id_) {
    DeliverNotification();
  } else if (timer_id == sustained_ad_interaction_timer_id_) {
    SustainAdInteractionIfNeeded();
  } else {
    BLOG(WARNING) << "Unexpected OnTimer: " << std::to_string(timer_id);
  }
}

void AdsImpl::GenerateAdReportingNotificationShownEvent(
    const NotificationInfo& info) {
  if (is_first_run_) {
    is_first_run_ = false;

    GenerateAdReportingRestartEvent();
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("notify");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.String("notificationType");
  writer.String("generated");

  writer.String("notificationClassification");
  writer.StartArray();
  std::vector<std::string> classifications = base::SplitString(info.category,
      "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  for (const auto& classification : classifications) {
    writer.String(classification.c_str());
  }
  writer.EndArray();

  writer.String("notificationCatalog");
  if (IsNotificationFromSampleCatalog(info)) {
    writer.String("sample-catalog");
  } else {
    writer.String(info.creative_set_id.c_str());
  }

  writer.String("notificationUrl");
  writer.String(info.url.c_str());

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);

  ConfirmAd(info, ConfirmationType::VIEW);
}

void AdsImpl::GenerateAdReportingNotificationResultEvent(
    const NotificationInfo& info,
    const NotificationResultInfoResultType type) {
  if (is_first_run_) {
    is_first_run_ = false;

    GenerateAdReportingRestartEvent();
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("notify");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.String("notificationType");
  switch (type) {
    case NotificationResultInfoResultType::CLICKED: {
      writer.String("clicked");
      client_->UpdateAdsUUIDSeen(info.uuid, 1);
      StartSustainingAdInteraction(kSustainAdInteractionAfterSeconds);

      break;
    }

    case NotificationResultInfoResultType::DISMISSED: {
      writer.String("dismissed");
      client_->UpdateAdsUUIDSeen(info.uuid, 1);

      break;
    }

    case NotificationResultInfoResultType::TIMEOUT: {
      writer.String("timeout");

      break;
    }
  }

  writer.String("notificationClassification");
  writer.StartArray();
  std::vector<std::string> classifications = base::SplitString(info.category,
      "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  for (const auto& classification : classifications) {
    writer.String(classification.c_str());
  }
  writer.EndArray();

  writer.String("notificationCatalog");
  if (IsNotificationFromSampleCatalog(info)) {
    writer.String("sample-catalog");
  } else {
    writer.String(info.creative_set_id.c_str());
  }

  writer.String("notificationUrl");
  writer.String(info.url.c_str());

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);

  switch (type) {
    case NotificationResultInfoResultType::CLICKED: {
      ConfirmAd(info, ConfirmationType::CLICK);
      break;
    }

    case NotificationResultInfoResultType::DISMISSED: {
      ConfirmAd(info, ConfirmationType::DISMISS);
      break;
    }

    case NotificationResultInfoResultType::TIMEOUT: {
      break;
    }
  }
}

void AdsImpl::GenerateAdReportingConfirmationEvent(
    const NotificationInfo& info) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("confirmation");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.String("notificationId");
  writer.String(info.uuid.c_str());

  writer.String("notificationType");
  auto type = std::string(info.type);
  writer.String(type.c_str());

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingLoadEvent(
    const LoadInfo& info) {
  if (!IsSupportedUrl(info.tab_url)) {
    return;
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("load");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.String("tabId");
  writer.Int(info.tab_id);

  writer.String("tabType");
  if (client_->GetSearchState()) {
    writer.String("search");
  } else {
    writer.String("click");
  }

  writer.String("tabUrl");
  writer.String(info.tab_url.c_str());

  writer.String("tabClassification");
  writer.StartArray();
  std::vector<std::string> classifications = base::SplitString(
      info.tab_classification, "-", base::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
  for (const auto& classification : classifications) {
    writer.String(classification.c_str());
  }
  writer.EndArray();

  auto cached_page_score = page_score_cache_.find(info.tab_url);
  if (cached_page_score != page_score_cache_.end()) {
    writer.String("pageScore");
    writer.StartArray();
    for (const auto& page_score : cached_page_score->second) {
      writer.Double(page_score);
    }
    writer.EndArray();
  }

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingBackgroundEvent() {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("background");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingForegroundEvent() {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("foreground");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingBlurEvent(
    const BlurInfo& info) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("blur");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.String("tabId");
  writer.Int(info.tab_id);

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingDestroyEvent(
    const DestroyInfo& info) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("destroy");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.String("tabId");
  writer.Int(info.tab_id);

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingFocusEvent(
    const FocusInfo& info) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("focus");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.String("tabId");
  writer.Int(info.tab_id);

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingRestartEvent() {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("restart");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingSettingsEvent() {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("settings");

  writer.String("stamp");
  auto time_stamp = helper::Time::TimeStamp();
  writer.String(time_stamp.c_str());

  writer.String("settings");
  writer.StartObject();

  writer.String("notifications");
  writer.StartObject();

  writer.String("available");
  auto configured = ads_client_->IsNotificationsAvailable();
  writer.Bool(configured);

  writer.EndObject();

  writer.String("locale");
  auto locale = client_->GetLocale();
  writer.String(locale.c_str());

  writer.String("adsPerDay");
  auto ads_per_day = ads_client_->GetAdsPerDay();
  writer.Uint64(ads_per_day);

  writer.String("adsPerHour");
  auto ads_per_hour = ads_client_->GetAdsPerHour();
  writer.Uint64(ads_per_hour);

  writer.EndObject();

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

bool AdsImpl::IsNotificationFromSampleCatalog(
    const NotificationInfo& info) const {
  return info.creative_set_id.empty();
}

bool AdsImpl::IsSupportedUrl(const std::string& url) const {
  DCHECK(!url.empty()) << "Invalid URL";

  return GURL(url).SchemeIsHTTPOrHTTPS();
}

bool AdsImpl::UrlHostsMatch(
    const std::string& url_1,
    const std::string& url_2) const {
  return GURL(url_1).DomainIs(GURL(url_2).host_piece());
}

}  // namespace ads
