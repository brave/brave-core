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
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/static_values.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "base/rand_util.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "base/guid.h"

#if defined(OS_ANDROID)
#include "base/system/sys_info.h"
#include "base/android/build_info.h"
#endif

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
    last_sustaining_ad_url_(""),
    next_easter_egg_timestamp_in_seconds_(0),
    client_(std::make_unique<Client>(this, ads_client)),
    bundle_(std::make_unique<Bundle>(this, ads_client)),
    ads_serve_(std::make_unique<AdsServe>(this, ads_client, bundle_.get())),
    notifications_(std::make_unique<Notifications>(this, ads_client)),
    user_model_(nullptr),
    is_initialized_(false),
    is_confirmations_ready_(false),
    ads_client_(ads_client) {
}

AdsImpl::~AdsImpl() {
  StopCollectingActivity();
  StopDeliveringNotifications();
  StopSustainingAdInteraction();
}

void AdsImpl::Initialize(InitializeCallback callback) {
  BLOG(INFO) << "Initializing ads";

  initialize_callback_ = callback;

  if (IsInitialized()) {
    BLOG(INFO) << "Already initialized ads";

    initialize_callback_(FAILED);
    return;
  }

  auto initialize_step_2_callback =
      std::bind(&AdsImpl::InitializeStep2, this, _1);
  client_->Initialize(initialize_step_2_callback);
}

void AdsImpl::InitializeStep2(const Result result) {
  if (result != SUCCESS) {
    initialize_callback_(FAILED);
    return;
  }

  auto callback = std::bind(&AdsImpl::InitializeStep3, this, _1);
  notifications_->Initialize(callback);
}

void AdsImpl::InitializeStep3(const Result result) {
  if (result != SUCCESS) {
    initialize_callback_(FAILED);
    return;
  }

  client_->SetLocales(ads_client_->GetLocales());

  auto locale = ads_client_->GetAdsLocale();
  ChangeLocale(locale);
}

void AdsImpl::InitializeStep4(const Result result) {
  if (result != SUCCESS) {
    initialize_callback_(FAILED);
    return;
  }

  is_initialized_ = true;

  BLOG(INFO) << "Successfully initialized ads";

  initialize_callback_(SUCCESS);

  is_foreground_ = ads_client_->IsForeground();

  ads_client_->SetIdleThreshold(kIdleThresholdInSeconds);

  NotificationAllowedCheck(false);

#if defined(OS_ANDROID)
    RemoveAllNotificationsAfterReboot();
    RemoveAllNotificationsAfterUpdate();
#endif

  client_->UpdateAdUUID();

  if (_is_debug) {
    StartCollectingActivity(kDebugOneHourInSeconds);
  } else {
    StartCollectingActivity(base::Time::kSecondsPerHour);
  }

  ads_serve_->DownloadCatalog();
}

#if defined(OS_ANDROID)
void AdsImpl::RemoveAllNotificationsAfterReboot() {
  //ads notifications don't sustain reboot, so remove all
  auto ads_shown_history = client_->GetAdsShownHistory();
  if (!ads_shown_history.empty()) {
    uint64_t ad_shown_timestamp = ads_shown_history.front();
    uint64_t boot_timestamp = Time::NowInSeconds() -
        static_cast<uint64_t>(base::SysInfo::Uptime().InSeconds());
    if (ad_shown_timestamp <= boot_timestamp) {
      notifications_->RemoveAll(false);
    }
  }
}

void AdsImpl::RemoveAllNotificationsAfterUpdate() {
  std::string current_version_code (base::android::BuildInfo::GetInstance()->package_version_code());
  std::string last_version_code = client_->GetVersionCode();
  if (last_version_code.empty()) {
    //initial update of version_code
    client_->SetVersionCode(current_version_code);
  }
  else if (last_version_code != current_version_code){
    //ads notifications don't sustain app update, so remove them
    notifications_->RemoveAll(false);
  }
}
#endif

bool AdsImpl::IsInitialized() {
  if (!is_initialized_ ||
      !ads_client_->IsAdsEnabled() ||
      !user_model_->IsInitialized()) {
    return false;
  }

  return true;
}

void AdsImpl::Shutdown(ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(WARNING) << "Failed to shutdown ads as not initialized";

    callback(FAILED);
    return;
  }

  notifications_->RemoveAll(true);

  callback(SUCCESS);
}

void AdsImpl::LoadUserModel() {
  auto locale = client_->GetLocale();

  auto callback = std::bind(&AdsImpl::OnUserModelLoaded, this, _1, _2);
  ads_client_->LoadUserModelForLocale(locale, callback);
}

void AdsImpl::OnUserModelLoaded(const Result result, const std::string& json) {
  auto locale = client_->GetLocale();

  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load user model for " << locale << " locale";
    return;
  }

  BLOG(INFO) << "Successfully loaded user model for " << locale << " locale";

  InitializeUserModel(json, locale);

  if (!IsInitialized()) {
    InitializeStep4(SUCCESS);
  }
}

void AdsImpl::InitializeUserModel(
    const std::string& json,
    const std::string& locale) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  BLOG(INFO) << "Initializing \"" << locale << "\" user model";

  user_model_.reset(usermodel::UserModel::CreateInstance());
  user_model_->InitializePageClassifier(json);

  BLOG(INFO) << "Initialized \"" << locale << "\" user model";
}

bool AdsImpl::IsMobile() const {
  ClientInfo client_info;
  ads_client_->GetClientInfo(&client_info);

  if (client_info.platform != ANDROID_OS && client_info.platform != IOS) {
    return false;
  }

  return true;
}

bool AdsImpl::GetNotificationForId(
    const std::string& id,
    ads::NotificationInfo* notification) {
  return notifications_->Get(id, notification);
}

void AdsImpl::OnForeground() {
  is_foreground_ = true;
  GenerateAdReportingForegroundEvent();

  if (IsMobile()) {
    StartDeliveringNotifications();
  }
}

void AdsImpl::OnBackground() {
  is_foreground_ = false;
  GenerateAdReportingBackgroundEvent();

  if (IsMobile()) {
    StopDeliveringNotifications();
  }
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
  auto tab = media_playing_.find(last_shown_tab_id_);
  if (tab == media_playing_.end()) {
    // Media is not playing in the last shown tab
    return false;
  }

  return true;
}

void AdsImpl::OnNotificationEvent(
    const std::string& id,
    const ads::NotificationEventType type) {
  NotificationInfo notification;
  if (!notifications_->Get(id, &notification)) {
    return;
  }

  switch (type) {
    case ads::NotificationEventType::VIEWED: {
      NotificationEventViewed(id, notification);
      break;
    }

    case ads::NotificationEventType::CLICKED: {
      NotificationEventClicked(id, notification);
      break;
    }

    case ads::NotificationEventType::DISMISSED: {
      NotificationEventDismissed(id, notification);
      break;
    }

    case ads::NotificationEventType::TIMEOUT: {
      NotificationEventTimedOut(id, notification);
      break;
    }
  }
}

void AdsImpl::NotificationEventViewed(
    const std::string& id,
    const NotificationInfo& notification) {
  GenerateAdReportingNotificationShownEvent(notification);

  ConfirmAd(notification, ConfirmationType::VIEW);
}

void AdsImpl::NotificationEventClicked(
    const std::string& id,
    const NotificationInfo& notification) {
  notifications_->Remove(id, true);

  GenerateAdReportingNotificationResultEvent(notification,
      NotificationResultInfoResultType::CLICKED);

  ConfirmAd(notification, ConfirmationType::CLICK);
}

void AdsImpl::NotificationEventDismissed(
    const std::string& id,
    const NotificationInfo& notification) {
  notifications_->Remove(id, false);

  GenerateAdReportingNotificationResultEvent(notification,
      NotificationResultInfoResultType::DISMISSED);

  ConfirmAd(notification, ConfirmationType::DISMISS);
}

void AdsImpl::NotificationEventTimedOut(
    const std::string& id,
    const NotificationInfo& notification) {
  notifications_->Remove(id, false);

  GenerateAdReportingNotificationResultEvent(notification,
      NotificationResultInfoResultType::TIMEOUT);
}

void AdsImpl::OnTabUpdated(
    const int32_t tab_id,
    const std::string& url,
    const bool is_active,
    const bool is_incognito) {
  if (is_incognito) {
    return;
  }

  client_->UpdateLastUserActivity();

  if (is_active) {
    BLOG(INFO) << "OnTabUpdated.IsFocused for tab id: " << tab_id
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
    BLOG(INFO) << "OnTabUpdated.IsBlurred for tab id: " << tab_id
        << " and url: " << url;

    BlurInfo blur_info;
    blur_info.tab_id = tab_id;
    GenerateAdReportingBlurEvent(blur_info);
  }
}

void AdsImpl::OnTabClosed(const int32_t tab_id) {
  BLOG(INFO) << "OnTabClosed for tab id: " << tab_id;

  OnMediaStopped(tab_id);

  DestroyInfo destroy_info;
  destroy_info.tab_id = tab_id;
  GenerateAdReportingDestroyEvent(destroy_info);
}

void AdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  client_->RemoveAllHistory();

  callback(SUCCESS);
}

void AdsImpl::SetConfirmationsIsReady(const bool is_ready) {
  is_confirmations_ready_ = is_ready;
}

void AdsImpl::ChangeLocale(const std::string& locale) {
  auto language_code = helper::Locale::GetLanguageCode(locale);

  auto locales = ads_client_->GetLocales();
  if (std::find(locales.begin(), locales.end(), language_code)
      != locales.end()) {
    BLOG(INFO) << "Changed locale to " << language_code;
    client_->SetLocale(language_code);
  } else {
    BLOG(INFO) << language_code << " locale not found, so changed locale to "
        << kDefaultLanguageCode;

    client_->SetLocale(kDefaultLanguageCode);
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

    if (last_sustaining_ad_url_ != url) {
      last_sustaining_ad_url_ = url;

      StartSustainingAdInteraction(kSustainAdInteractionAfterSeconds);
    } else {
      BLOG(INFO) << "Already sustaining Ad interaction for " << url;
    }

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
    BLOG(WARNING) << "Failed to test shopping data as not initialized";
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
    BLOG(WARNING) << "Failed to test search state as not initialized";
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
    BLOG(WARNING) << "Failed to serve sample Ad as not initialized";
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

  auto now_in_seconds = Time::NowInSeconds();

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

    if (!IsForeground()) {
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

  auto available_ads = GetAvailableAds(ads);

  BLOG(INFO) << "Found " << available_ads.size() << " out of " << ads.size()
      << " availables ads for \"" << category << "\" category";

  if (available_ads.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ad (or permitted ad) for
    // winnerOverTime', category, winnerOverTime, arbitraryKey }

    BLOG(INFO) << "Notification not made: No ad (or permitted ad) for \""
        << category << "\" category";

    return;
  }

  auto rand = base::RandInt(0, available_ads.size() - 1);
  auto ad = available_ads.at(rand);
  ShowAd(ad, category);
}

std::vector<AdInfo> AdsImpl::GetAvailableAds(
    const std::vector<AdInfo>& ads) {
  std::vector<AdInfo> available_ads = {};

  for (const auto& ad : ads) {
    if (!AdRespectsTotalMaxFrequencyCapping(ad)) {
      BLOG(INFO) << "creativeSetId " << ad.creative_set_id
          << " has exceeded the totalMax";

      continue;
    }

    if (!AdRespectsPerDayFrequencyCapping(ad)) {
      BLOG(INFO) << "creativeSetId " << ad.creative_set_id
          << " has exceeded the perDay";

      continue;
    }

    if (!AdRespectsDailyCapFrequencyCapping(ad)) {
      BLOG(INFO) << "creativeSetId " << ad.creative_set_id
          << " has exceeded the dailyCap";

      continue;
    }

    available_ads.push_back(ad);
  }

  return available_ads;
}

bool AdsImpl::AdRespectsTotalMaxFrequencyCapping(const AdInfo& ad) {
  auto creative_set = GetCreativeSetForId(ad.creative_set_id);
  if (creative_set.size() >= ad.total_max) {
    return false;
  }

  return true;
}

bool AdsImpl::AdRespectsPerDayFrequencyCapping(const AdInfo& ad) {
  auto creative_set = GetCreativeSetForId(ad.creative_set_id);
  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  return HistoryRespectsRollingTimeConstraint(
      creative_set, day_window, ad.per_day);
}

bool AdsImpl::AdRespectsDailyCapFrequencyCapping(const AdInfo& ad) {
  auto campaign = GetCampaignForId(ad.campaign_id);
  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  return HistoryRespectsRollingTimeConstraint(
      campaign, day_window, ad.daily_cap);
}

std::deque<uint64_t> AdsImpl::GetCreativeSetForId(const std::string& id) {
  std::deque<uint64_t> creative_set = {};

  auto creative_set_history = client_->GetCreativeSetHistory();
  if (creative_set_history.find(id) != creative_set_history.end()) {
    creative_set = creative_set_history.at(id);
  }

  return creative_set;
}

std::deque<uint64_t> AdsImpl::GetCampaignForId(const std::string& id) {
  std::deque<uint64_t> campaign = {};

  auto campaign_history = client_->GetCampaignHistory();
  if (campaign_history.find(id) != campaign_history.end()) {
    campaign = campaign_history.at(id);
  }

  return campaign;
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
  notification_info->id = base::GenerateGUID();
  notification_info->advertiser = ad_info.advertiser;
  notification_info->category = category;
  notification_info->text = ad_info.notification_text;
  notification_info->url = helper::Uri::GetUri(ad_info.notification_url);
  notification_info->creative_set_id = ad_info.creative_set_id;
  notification_info->uuid = ad_info.uuid;

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Notification shown', {category, winnerOverTime, arbitraryKey,
  // notificationUrl, notificationText, advertiser, uuid, hierarchy}

  BLOG(INFO) << "Notification shown:"
      << std::endl << "  id: " << notification_info->id
      << std::endl << "  campaign_id: " << ad_info.campaign_id
      << std::endl << "  winnerOverTime: " << GetWinnerOverTimeCategory()
      << std::endl << "  advertiser: " << notification_info->advertiser
      << std::endl << "  category: " << notification_info->category
      << std::endl << "  text: " << notification_info->text
      << std::endl << "  url: " << notification_info->url
      << std::endl << "  uuid: " << notification_info->uuid;

  notifications_->PushBack(*notification_info);
  if (notifications_->Count() > kMaximumAdNotifications) {
    notifications_->PopFront(true);
  }


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

  auto now_in_seconds = Time::NowInSeconds();

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
  auto does_history_respect_ads_per_day_limit =
      DoesHistoryRespectAdsPerDayLimit();

  bool does_history_respect_minimum_wait_time;
  if (!IsMobile()) {
    does_history_respect_minimum_wait_time =
        DoesHistoryRespectMinimumWaitTimeToShowAds();
  } else {
    does_history_respect_minimum_wait_time = true;
  }

  BLOG(INFO) << "IsAllowedToShowAds:";
  BLOG(INFO) << "    does_history_respect_minimum_wait_time: "
      << does_history_respect_minimum_wait_time;
  BLOG(INFO) << "    does_history_respect_ads_per_day_limit: "
      << does_history_respect_ads_per_day_limit;

  return does_history_respect_minimum_wait_time &&
      does_history_respect_ads_per_day_limit;
}

bool AdsImpl::DoesHistoryRespectMinimumWaitTimeToShowAds() {
  auto ads_shown_history = client_->GetAdsShownHistory();

  auto hour_window = base::Time::kSecondsPerHour;
  auto hour_allowed = ads_client_->GetAdsPerHour();
  auto respects_hour_limit = HistoryRespectsRollingTimeConstraint(
      ads_shown_history, hour_window, hour_allowed);

  auto minimum_wait_time = hour_window / hour_allowed;
  auto respects_minimum_wait_time = HistoryRespectsRollingTimeConstraint(
      ads_shown_history, minimum_wait_time, 0);

  BLOG(INFO) << "DoesHistoryRespectMinimumWaitTimeToShowAds:";
  BLOG(INFO) << "    respects_hour_limit: "
      << respects_hour_limit;
  BLOG(INFO) << "    respects_minimum_wait_time: "
      << respects_minimum_wait_time;

  return respects_hour_limit && respects_minimum_wait_time;
}

bool AdsImpl::DoesHistoryRespectAdsPerDayLimit() {
  auto ads_shown_history = client_->GetAdsShownHistory();

  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;
  auto day_allowed = ads_client_->GetAdsPerDay();

  auto respects_day_limit = HistoryRespectsRollingTimeConstraint(
      ads_shown_history, day_window, day_allowed);

  BLOG(INFO) << "DoesHistoryRespectAdsPerDayLimit:";
  BLOG(INFO) << "    respects_day_limit: "
      << respects_day_limit;

  return respects_day_limit;
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
    BLOG(WARNING) << "Failed to collect activity as not initialized";
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

void AdsImpl::StartDeliveringNotifications() {
  StopDeliveringNotifications();

  if (client_->GetNextCheckServeAdTimestampInSeconds() == 0) {
    client_->UpdateNextCheckServeAdTimestampInSeconds();
  }

  auto now_in_seconds = Time::NowInSeconds();
  auto next_check_serve_ad_timestamp_in_seconds =
      client_->GetNextCheckServeAdTimestampInSeconds();

  uint64_t start_timer_in;
  if (now_in_seconds >= next_check_serve_ad_timestamp_in_seconds) {
    // Browser was launched after the next check to serve an ad
    start_timer_in = 1 * base::Time::kSecondsPerMinute;
  } else {
    start_timer_in = next_check_serve_ad_timestamp_in_seconds - now_in_seconds;
  }

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

  client_->UpdateNextCheckServeAdTimestampInSeconds();

  StartDeliveringNotifications();
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

  auto now_in_seconds = Time::NowInSeconds();

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
    // 'Notification not made', { reason: 'notifications not presently allowed' }

    BLOG(INFO) << "Notification not made: Notifications not presently allowed";

    return;
  }

  if (!ads_client_->IsNetworkConnectionAvailable()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'network connection not available' }

    BLOG(INFO) << "Notification not made: Network connection not available";

    return;
  }

  if (IsCatalogOlderThanOneDay()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'catalog older than one day' }

    BLOG(INFO) << "Notification not made: Catalog older than one day";

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
  auto time_stamp = Time::Timestamp();
  writer.String(time_stamp.c_str());

  writer.String("notificationType");
  writer.String("generated");

  writer.String("notificationClassification");
  writer.StartArray();
  std::vector<std::string> classifications = base::SplitString(
      info.category, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
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
  auto time_stamp = Time::Timestamp();
  writer.String(time_stamp.c_str());

  writer.String("notificationType");
  switch (type) {
    case NotificationResultInfoResultType::CLICKED: {
      writer.String("clicked");
      client_->UpdateAdsUUIDSeen(info.uuid, 1);

      last_shown_notification_info_ = NotificationInfo(info);

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
  std::vector<std::string> classifications = base::SplitString(
      info.category, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
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
  auto time_stamp = Time::Timestamp();
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
  auto time_stamp = Time::Timestamp();
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
  auto time_stamp = Time::Timestamp();
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
  auto time_stamp = Time::Timestamp();
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
  auto time_stamp = Time::Timestamp();
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
  auto time_stamp = Time::Timestamp();
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
  auto time_stamp = Time::Timestamp();
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
  auto time_stamp = Time::Timestamp();
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
  auto time_stamp = Time::Timestamp();
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
