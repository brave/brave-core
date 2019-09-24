/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>
#include <vector>
#include <algorithm>
#include <utility>

#include "bat/ads/ad_history_detail.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/ads_history.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/notification_info.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/classification_helper.h"
#include "bat/ads/internal/locale_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/search_providers.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/uri_helper.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "base/guid.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"

#if defined(OS_ANDROID)
#include "base/system/sys_info.h"
#include "base/android/build_info.h"
#endif

#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

const int kDaysOfAdsHistory = 7;

std::string GetDisplayUrl(const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid())
    return std::string();

  return gurl.host();
}

}  // namespace

namespace ads {

AdsImpl::AdsImpl(AdsClient* ads_client) :
    is_first_run_(true),
    is_foreground_(false),
    media_playing_({}),
    active_tab_id_(0),
    active_tab_url_(""),
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

void AdsImpl::Initialize(
    InitializeCallback callback) {
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

void AdsImpl::InitializeStep2(
    const Result result) {
  if (result != SUCCESS) {
    initialize_callback_(FAILED);
    return;
  }

  auto callback = std::bind(&AdsImpl::InitializeStep3, this, _1);
  notifications_->Initialize(callback);
}

void AdsImpl::InitializeStep3(
    const Result result) {
  if (result != SUCCESS) {
    initialize_callback_(FAILED);
    return;
  }

  auto user_model_languages = ads_client_->GetUserModelLanguages();
  client_->SetUserModelLanguages(user_model_languages);

  auto locale = ads_client_->GetLocale();
  ChangeLocale(locale);
}

void AdsImpl::InitializeStep4(
    const Result result) {
  if (result != SUCCESS) {
    initialize_callback_(FAILED);
    return;
  }

  is_initialized_ = true;

  BLOG(INFO) << "Successfully initialized ads";

  is_foreground_ = ads_client_->IsForeground();

  ads_client_->SetIdleThreshold(kIdleThresholdInSeconds);

  initialize_callback_(SUCCESS);

  NotificationAllowedCheck(false);

#if defined(OS_ANDROID)
    RemoveAllNotificationsAfterReboot();
    RemoveAllNotificationsAfterUpdate();
#endif

  client_->UpdateAdUUID();

  if (IsMobile()) {
    StartDeliveringNotifications();
  }

  if (_is_debug) {
    StartCollectingActivity(kDebugOneHourInSeconds);
  } else {
    StartCollectingActivity(base::Time::kSecondsPerHour);
  }

  ads_serve_->DownloadCatalog();
}

#if defined(OS_ANDROID)
void AdsImpl::RemoveAllNotificationsAfterReboot() {
  // ads notifications don't sustain reboot, so remove all
  auto ads_shown_history = client_->GetAdsShownHistory();
  if (!ads_shown_history.empty()) {
    uint64_t ad_shown_timestamp =
        ads_shown_history.front().timestamp_in_seconds;
    uint64_t boot_timestamp = Time::NowInSeconds() -
        static_cast<uint64_t>(base::SysInfo::Uptime().InSeconds());
    if (ad_shown_timestamp <= boot_timestamp) {
      notifications_->RemoveAll(false);
    }
  }
}

void AdsImpl::RemoveAllNotificationsAfterUpdate() {
  std::string current_version_code(
      base::android::BuildInfo::GetInstance()->package_version_code());
  std::string last_version_code = client_->GetVersionCode();
  if (last_version_code.empty()) {
    // initial update of version_code
    client_->SetVersionCode(current_version_code);
  } else if (last_version_code != current_version_code) {
    // ads notifications don't sustain app update, so remove them
    notifications_->RemoveAll(false);
  }
}
#endif

bool AdsImpl::IsInitialized() {
  if (!is_initialized_ || !ads_client_->IsEnabled()) {
    return false;
  }

  if (ShouldClassifyPages() && !user_model_->IsInitialized()) {
    return false;
  }

  return true;
}

void AdsImpl::Shutdown(
    ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(WARNING) << "Failed to shutdown ads as not initialized";

    callback(FAILED);
    return;
  }

  notifications_->RemoveAll(true);

  callback(SUCCESS);
}

void AdsImpl::LoadUserModel() {
  auto language = client_->GetUserModelLanguage();

  auto callback = std::bind(&AdsImpl::OnUserModelLoaded, this, _1, _2);
  ads_client_->LoadUserModelForLanguage(language, callback);
}

void AdsImpl::OnUserModelLoaded(
    const Result result,
    const std::string& json) {
  auto language = client_->GetUserModelLanguage();

  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load user model for " << language << " language";
    return;
  }

  BLOG(INFO) << "Successfully loaded user model for " << language
      << " language";

  InitializeUserModel(json, language);

  if (!IsInitialized()) {
    InitializeStep4(SUCCESS);
  }
}

void AdsImpl::InitializeUserModel(
    const std::string& json,
    const std::string& language) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  BLOG(INFO) << "Initializing user model for \"" << language << "\" language";

  user_model_.reset(usermodel::UserModel::CreateInstance());
  user_model_->InitializePageClassifier(json);

  BLOG(INFO) << "Initialized user model for \"" << language << "\" language";
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
  if (!IsInitialized()) {
    return;
  }

  is_foreground_ = true;
  GenerateAdReportingForegroundEvent();

  if (IsMobile()) {
    StartDeliveringNotifications();
  }
}

void AdsImpl::OnBackground() {
  if (!IsInitialized()) {
    return;
  }

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

void AdsImpl::OnMediaPlaying(
    const int32_t tab_id) {
  auto tab = media_playing_.find(tab_id);
  if (tab != media_playing_.end()) {
    // Media is already playing for this tab
    return;
  }

  BLOG(INFO) << "OnMediaPlaying for tab id: " << tab_id;

  media_playing_.insert({tab_id, true});
}

void AdsImpl::OnMediaStopped(
    const int32_t tab_id) {
  auto tab = media_playing_.find(tab_id);
  if (tab == media_playing_.end()) {
    // Media is not playing for this tab
    return;
  }

  BLOG(INFO) << "OnMediaStopped for tab id: " << tab_id;

  media_playing_.erase(tab_id);
}

bool AdsImpl::IsMediaPlaying() const {
  auto tab = media_playing_.find(active_tab_id_);
  if (tab == media_playing_.end()) {
    // Media is not playing in the active tab
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
  GenerateAdsHistoryEntry(notification, ConfirmationType::VIEW);
}

void AdsImpl::NotificationEventClicked(
    const std::string& id,
    const NotificationInfo& notification) {
  notifications_->Remove(id, true);

  GenerateAdReportingNotificationResultEvent(notification,
      NotificationResultInfoResultType::CLICKED);

  ConfirmAd(notification, ConfirmationType::CLICK);
  GenerateAdsHistoryEntry(notification, ConfirmationType::CLICK);
}

void AdsImpl::NotificationEventDismissed(
    const std::string& id,
    const NotificationInfo& notification) {
  notifications_->Remove(id, false);

  GenerateAdReportingNotificationResultEvent(notification,
      NotificationResultInfoResultType::DISMISSED);

  ConfirmAd(notification, ConfirmationType::DISMISS);
  GenerateAdsHistoryEntry(notification, ConfirmationType::DISMISS);
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

    active_tab_id_ = tab_id;
    previous_tab_url_ = active_tab_url_;
    active_tab_url_ = url;

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

void AdsImpl::OnTabClosed(
    const int32_t tab_id) {
  BLOG(INFO) << "OnTabClosed for tab id: " << tab_id;

  OnMediaStopped(tab_id);

  DestroyInfo destroy_info;
  destroy_info.tab_id = tab_id;
  GenerateAdReportingDestroyEvent(destroy_info);
}

void AdsImpl::RemoveAllHistory(
    RemoveAllHistoryCallback callback) {
  client_->RemoveAllHistory();

  callback(SUCCESS);
}

void AdsImpl::SetConfirmationsIsReady(
    const bool is_ready) {
  is_confirmations_ready_ = is_ready;
}

std::map<uint64_t, std::vector<AdsHistory>> AdsImpl::GetAdsHistory() {
  std::map<uint64_t, std::vector<AdsHistory>> ads_history;
  base::Time now = base::Time::Now().LocalMidnight();

  auto ad_history_details = client_->GetAdsShownHistory();
  for (auto& detail_item : ad_history_details) {
    auto history_item = std::make_unique<AdsHistory>();
    history_item->details.push_back(detail_item);

    base::Time timestamp =
        Time::FromDoubleT(detail_item.timestamp_in_seconds).LocalMidnight();
    base::TimeDelta time_delta = now - timestamp;
    if (time_delta.InDays() >= kDaysOfAdsHistory) {
      break;
    }

    const uint64_t timestamp_in_seconds =
        static_cast<uint64_t>((timestamp - base::Time()).InSeconds());
    ads_history[timestamp_in_seconds].push_back(*history_item);
  }

  return ads_history;
}

AdContent::LikeAction AdsImpl::ToggleAdThumbUp(
    const std::string& id,
    const std::string& creative_set_id,
    const AdContent::LikeAction& action) {
  auto like_action = client_->ToggleAdThumbUp(id, creative_set_id, action);
  if (like_action == AdContent::LIKE_ACTION_THUMBS_UP) {
    ConfirmAction(id, creative_set_id, ConfirmationType::UPVOTE);
  }

  return like_action;
}

AdContent::LikeAction AdsImpl::ToggleAdThumbDown(
    const std::string& id,
    const std::string& creative_set_id,
    const AdContent::LikeAction& action) {
  auto like_action = client_->ToggleAdThumbDown(id, creative_set_id, action);
  if (like_action == AdContent::LIKE_ACTION_THUMBS_DOWN) {
    ConfirmAction(id, creative_set_id, ConfirmationType::DOWNVOTE);
  }

  return like_action;
}

CategoryContent::OptAction AdsImpl::ToggleAdOptInAction(
    const std::string& category,
    const CategoryContent::OptAction& action) {
  return client_->ToggleAdOptInAction(category, action);
}

CategoryContent::OptAction AdsImpl::ToggleAdOptOutAction(
    const std::string& category,
    const CategoryContent::OptAction& action) {
  return client_->ToggleAdOptOutAction(category, action);
}

bool AdsImpl::ToggleSaveAd(
    const std::string& id,
    const std::string& creative_set_id,
    const bool saved) {
  return client_->ToggleSaveAd(id, creative_set_id, saved);
}

bool AdsImpl::ToggleFlagAd(
    const std::string& id,
    const std::string& creative_set_id,
    const bool flagged) {
  auto flag_ad = client_->ToggleFlagAd(id, creative_set_id, flagged);
  if (flag_ad) {
    ConfirmAction(id, creative_set_id, ConfirmationType::FLAG);
  }

  return flag_ad;
}

void AdsImpl::ChangeLocale(
    const std::string& locale) {
  auto language = helper::Locale::GetLanguageCode(locale);

  if (!ShouldClassifyPages()) {
    client_->SetUserModelLanguage(language);

    InitializeStep4(SUCCESS);
    return;
  }

  auto languages = client_->GetUserModelLanguages();
  if (std::find(languages.begin(), languages.end(), language)
      != languages.end()) {
    BLOG(INFO) << "Changed to " << language << " user model";

    client_->SetUserModelLanguage(language);
  } else {
    BLOG(INFO) << language << " user model not found, defaulting to "
        << kDefaultUserModelLanguage << " user model";

    client_->SetUserModelLanguage(kDefaultUserModelLanguage);
  }

  LoadUserModel();
}

void AdsImpl::OnPageLoaded(
    const std::string& url,
    const std::string& html) {
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

  MaybeClassifyPage(url, html);

  CheckEasterEgg(url);

  BLOG(INFO) << "Site visited " << url << ", previous tab url was "
      << previous_tab_url_;
}

void AdsImpl::MaybeClassifyPage(
    const std::string& url,
    const std::string& html) {
  if (!ShouldClassifyPages()) {
    MaybeGenerateAdReportingLoadEvent(url, kUntargetedPageClassification);
    return;
  }

  auto classification = ClassifyPage(url, html);
  MaybeGenerateAdReportingLoadEvent(url, classification);
}

bool AdsImpl::ShouldClassifyPages() const {
  auto locale = ads_client_->GetLocale();
  auto region = helper::Locale::GetRegionCode(locale);

  auto it = kSupportedRegions.find(region);
  if (it == kSupportedRegions.end()) {
    return false;
  }

  return it->second;
}

std::string AdsImpl::ClassifyPage(
    const std::string& url,
    const std::string& html) {
  auto page_score = user_model_->ClassifyPage(html);

  auto winning_category = GetWinningCategory(page_score);
  if (winning_category.empty()) {
    BLOG(INFO) << "Failed to classify page at " << url
        << " as not enough content";
    return "";
  }

  client_->SetLastPageClassification(winning_category);

  client_->AppendPageScoreToPageScoreHistory(page_score);

  CachePageScore(active_tab_url_, page_score);

  BLOG(INFO) << "Successfully classified page at " << url << " as "
      << winning_category << ". Winning category over time is "
      << GetWinnerOverTimeCategory();

  return winning_category;
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
    DCHECK(page_score.size() == count);

    for (size_t i = 0; i < page_score.size(); i++) {
      auto taxonomy = user_model_->GetTaxonomyAtIndex(i);
      if (client_->IsFilteredCategory(taxonomy)) {
        BLOG(INFO) << taxonomy
                   << " taxonomy has been excluded from the winner over time";

        continue;
      }

      winner_over_time_page_score[i] += page_score[i];
    }
  }

  return GetWinningCategory(winner_over_time_page_score);
}

std::string AdsImpl::GetWinningCategory(
    const std::vector<double>& page_score) {
  return user_model_->GetWinningCategory(page_score);
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

void AdsImpl::TestShoppingData(
    const std::string& url) {
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

bool AdsImpl::TestSearchState(
    const std::string& url) {
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
  std::string json_schema =
      ads_client_->LoadJsonSchema(_bundle_schema_resource_name);
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

void AdsImpl::CheckEasterEgg(
    const std::string& url) {
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

void AdsImpl::CheckReadyAdServe(
    const bool forced) {
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
      BLOG(INFO) << "Notification not made: Not in foreground";
      return;
    }

    if (IsMediaPlaying()) {
      BLOG(INFO) << "Notification not made: Media playing in browser";
      return;
    }

    if (!IsAllowedToServeAds()) {
      BLOG(INFO) << "Notification not made: Not allowed based on history";
      return;
    }
  }

  auto category = GetWinnerOverTimeCategory();
  ServeAdFromCategory(category);
}

void AdsImpl::ServeAdFromCategory(
    const std::string& category) {
  BLOG(INFO) << "Notification for category " << category;

  std::string catalog_id = bundle_->GetCatalogId();
  if (catalog_id.empty()) {
    BLOG(INFO) << "Notification not made: No ad catalog";
    return;
  }

  if (!category.empty()) {
    auto callback =
        std::bind(&AdsImpl::OnServeAdFromCategory, this, _1, _2, _3);
    ads_client_->GetAds(category, callback);
    return;
  }

  BLOG(INFO) << "Notification not made: Category is empty, trying "
      << "again with untargeted category";

  ServeUntargetedAd();
}

void AdsImpl::OnServeAdFromCategory(
    const Result result,
    const std::string& category,
    const std::vector<AdInfo>& ads) {
  auto eligible_ads = GetEligibleAds(ads);
  if (!eligible_ads.empty()) {
    ServeAd(category, eligible_ads);
    return;
  }

  if (ServeAdFromParentCategory(category, eligible_ads)) {
    return;
  }

  BLOG(INFO) << "Notification not made: No ads found in \"" << category
      << "\" category, trying again with untargeted category";

  ServeUntargetedAd();
}

bool AdsImpl::ServeAdFromParentCategory(
    const std::string& category,
    const std::vector<AdInfo>& ads) {
  auto pos = category.find_last_of('-');
  if (pos == std::string::npos) {
    return false;
  }

  std::string parent_category = category.substr(0, pos);

  BLOG(INFO) << "Notification not made: No ads found in \"" << category
      << "\" category, trying again with \"" << parent_category
      << "\" category";

  auto callback =
      std::bind(&AdsImpl::OnServeAdFromCategory, this, _1, _2, _3);
  ads_client_->GetAds(parent_category, callback);

  return true;
}

void AdsImpl::ServeUntargetedAd() {
  auto callback = std::bind(&AdsImpl::OnServeUntargetedAd, this, _1, _2, _3);
  ads_client_->GetAds(kUntargetedPageClassification, callback);
}

void AdsImpl::OnServeUntargetedAd(
    const Result result,
    const std::string& category,
    const std::vector<AdInfo>& ads) {
  auto eligible_ads = GetEligibleAds(ads);
  if (!eligible_ads.empty()) {
    ServeAd(category, eligible_ads);
    return;
  }

  BLOG(INFO) << "Notification not made: No ad (or eligible ad) for \""
      << category << "\" category";
}

void AdsImpl::ServeAd(
    const std::string& category,
    const std::vector<AdInfo>& ads) {
  BLOG(INFO) << "Found " << ads.size() << " eligible ads for \"" << category
      << "\" category";

  auto rand = base::RandInt(0, ads.size() - 1);
  auto ad = ads.at(rand);
  ShowAd(ad, category);
}

std::vector<AdInfo> AdsImpl::GetEligibleAds(
    const std::vector<AdInfo>& ads) {
  std::vector<AdInfo> eligible_ads = {};

  auto unseen_ads = GetUnseenAdsAndRoundRobinIfNeeded(ads);

  for (const auto& ad : unseen_ads) {
    if (!AdRespectsTotalMaxFrequencyCapping(ad)) {
      BLOG(WARNING) << "creativeSetId " << ad.creative_set_id
          << " has exceeded the frequency capping for totalMax";

      continue;
    }

    if (!AdRespectsPerHourFrequencyCapping(ad)) {
      BLOG(WARNING) << "adUUID " << ad.uuid
          << " has exceeded the frequency capping for perHour";

      continue;
    }

    if (!AdRespectsPerDayFrequencyCapping(ad)) {
      BLOG(WARNING) << "creativeSetId " << ad.creative_set_id
          << " has exceeded the frequency capping for perDay";

      continue;
    }

    if (!AdRespectsDailyCapFrequencyCapping(ad)) {
      BLOG(WARNING) << "campaignId " << ad.campaign_id
          << " has exceeded the frequency capping for dailyCap";

      continue;
    }

    if (client_->IsFilteredAd(ad.creative_set_id)) {
      BLOG(WARNING) << "creativeSetId " << ad.creative_set_id
          << " appears in the filtered ads list";

      continue;
    }

    if (client_->IsFlaggedAd(ad.creative_set_id)) {
      BLOG(WARNING) << "creativeSetId " << ad.creative_set_id
          << " appears in the flagged ads list";

      continue;
    }

    eligible_ads.push_back(ad);
  }

  return eligible_ads;
}

std::vector<AdInfo> AdsImpl::GetUnseenAdsAndRoundRobinIfNeeded(
    const std::vector<AdInfo>& ads) const {
  auto unseen_ads = GetUnseenAds(ads);
  if (unseen_ads.empty()) {
    client_->ResetAdsUUIDSeen(ads);

    unseen_ads = GetUnseenAds(ads);
  }

  return unseen_ads;
}

std::vector<AdInfo> AdsImpl::GetUnseenAds(
    const std::vector<AdInfo>& ads) const {
  auto unseen_ads = ads;
  const auto seen_ads = client_->GetAdsUUIDSeen();

  const auto it = std::remove_if(unseen_ads.begin(), unseen_ads.end(),
      [&](AdInfo& ad) {
    return seen_ads.find(ad.uuid) != seen_ads.end();
  });

  unseen_ads.erase(it, unseen_ads.end());

  return unseen_ads;
}

bool AdsImpl::AdRespectsTotalMaxFrequencyCapping(
    const AdInfo& ad) {
  auto creative_set = GetCreativeSetForId(ad.creative_set_id);
  if (creative_set.size() >= ad.total_max) {
    return false;
  }

  return true;
}

bool AdsImpl::AdRespectsPerHourFrequencyCapping(
    const AdInfo& ad) {
  auto ads_shown = GetAdsShownForId(ad.uuid);
  auto hour_window = base::Time::kSecondsPerHour;

  return HistoryRespectsRollingTimeConstraint(
      ads_shown, hour_window, 1);
}

bool AdsImpl::AdRespectsPerDayFrequencyCapping(
    const AdInfo& ad) {
  auto creative_set = GetCreativeSetForId(ad.creative_set_id);
  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  return HistoryRespectsRollingTimeConstraint(
      creative_set, day_window, ad.per_day);
}

bool AdsImpl::AdRespectsDailyCapFrequencyCapping(
    const AdInfo& ad) {
  auto campaign = GetCampaignForId(ad.campaign_id);
  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  return HistoryRespectsRollingTimeConstraint(
      campaign, day_window, ad.daily_cap);
}

std::deque<uint64_t> AdsImpl::GetAdsShownForId(
    const std::string& id) {
  std::deque<uint64_t> ads_shown = {};

  auto ads_shown_history = client_->GetAdsShownHistory();
  for (const auto& ad_shown : ads_shown_history) {
    if (ad_shown.ad_content.uuid == id) {
      ads_shown.push_back(ad_shown.timestamp_in_seconds);
    }
  }

  return ads_shown;
}

std::deque<uint64_t> AdsImpl::GetCreativeSetForId(
    const std::string& id) {
  std::deque<uint64_t> creative_set = {};

  auto creative_set_history = client_->GetCreativeSetHistory();
  if (creative_set_history.find(id) != creative_set_history.end()) {
    creative_set = creative_set_history.at(id);
  }

  return creative_set;
}

std::deque<uint64_t> AdsImpl::GetCampaignForId(
    const std::string& id) {
  std::deque<uint64_t> campaign = {};

  auto campaign_history = client_->GetCampaignHistory();
  if (campaign_history.find(id) != campaign_history.end()) {
    campaign = campaign_history.at(id);
  }

  return campaign;
}

bool AdsImpl::IsAdValid(
    const AdInfo& ad_info) {
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
        << std::endl << "  creativeSetId: " << ad_info.creative_set_id
        << std::endl << "  uuid: " << ad_info.uuid;

    return false;
  }

  return true;
}

bool AdsImpl::ShowAd(
    const AdInfo& ad,
    const std::string& category) {
  if (!IsAdValid(ad)) {
    return false;
  }

  auto notification_info = std::make_unique<NotificationInfo>();
  notification_info->id = base::GenerateGUID();
  notification_info->advertiser = ad.advertiser;
  notification_info->category = category;
  notification_info->text = ad.notification_text;
  notification_info->url = helper::Uri::GetUri(ad.notification_url);
  notification_info->creative_set_id = ad.creative_set_id;
  notification_info->uuid = ad.uuid;

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Notification shown', {category, winnerOverTime, arbitraryKey,
  // notificationUrl, notificationText, advertiser, uuid, hierarchy}

  BLOG(INFO) << "Notification shown:"
      << std::endl << "  id: " << notification_info->id
      << std::endl << "  campaign_id: " << ad.campaign_id
      << std::endl << "  winnerOverTime: " << GetWinnerOverTimeCategory()
      << std::endl << "  advertiser: " << notification_info->advertiser
      << std::endl << "  category: " << notification_info->category
      << std::endl << "  text: " << notification_info->text
      << std::endl << "  url: " << notification_info->url
      << std::endl << "  uuid: " << notification_info->uuid;

  notifications_->PushBack(*notification_info);

#if defined(OS_ANDROID)
  if (notifications_->Count() > kMaximumAdNotifications) {
    notifications_->PopFront(true);
  }
#endif


  client_->AppendCurrentTimeToCreativeSetHistory(ad.creative_set_id);
  client_->AppendCurrentTimeToCampaignHistory(ad.campaign_id);

  client_->UpdateAdsUUIDSeen(ad.uuid, 1);

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

bool AdsImpl::HistoryRespectsRollingTimeConstraint(
    const std::deque<AdHistoryDetail> history,
    const uint64_t seconds_window,
    const uint64_t allowable_ad_count) const {
  uint64_t recent_count = 0;

  auto now_in_seconds = Time::NowInSeconds();

  for (const auto& detail : history) {
    if (now_in_seconds - detail.timestamp_in_seconds < seconds_window) {
      recent_count++;
    }
  }

  if (recent_count <= allowable_ad_count) {
    return true;
  }

  return false;
}

bool AdsImpl::IsAllowedToServeAds() {
  auto does_history_respect_ads_per_day_limit =
      DoesHistoryRespectAdsPerDayLimit();

  bool does_history_respect_minimum_wait_time;
  if (!IsMobile()) {
    does_history_respect_minimum_wait_time =
        DoesHistoryRespectMinimumWaitTimeToServeAds();
  } else {
    does_history_respect_minimum_wait_time = true;
  }

  BLOG(INFO) << "IsAllowedToServeAds:";
  BLOG(INFO) << "    does_history_respect_minimum_wait_time: "
      << does_history_respect_minimum_wait_time;
  BLOG(INFO) << "    does_history_respect_ads_per_day_limit: "
      << does_history_respect_ads_per_day_limit;

  return does_history_respect_minimum_wait_time &&
      does_history_respect_ads_per_day_limit;
}

bool AdsImpl::DoesHistoryRespectMinimumWaitTimeToServeAds() {
  auto ads_shown_history = client_->GetAdsShownHistory();

  auto hour_window = base::Time::kSecondsPerHour;
  auto hour_allowed = ads_client_->GetAdsPerHour();
  auto respects_hour_limit = HistoryRespectsRollingTimeConstraint(
      ads_shown_history, hour_window, hour_allowed);

  auto minimum_wait_time = hour_window / hour_allowed;
  auto respects_minimum_wait_time = HistoryRespectsRollingTimeConstraint(
      ads_shown_history, minimum_wait_time, 0);

  BLOG(INFO) << "DoesHistoryRespectMinimumWaitTimeToServeAds:";
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

void AdsImpl::StartCollectingActivity(
    const uint64_t start_timer_in) {
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

void AdsImpl::NotificationAllowedCheck(
    const bool serve) {
  auto ok = ads_client_->ShouldShowNotifications();

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
    // 'Notification not made', { reason: 'notifications not presently allowed'
    // }

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

void AdsImpl::StartSustainingAdInteraction(
    const uint64_t start_timer_in) {
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
  if (!UrlHostsMatch(active_tab_url_, last_shown_notification_info_.url)) {
    BLOG(INFO) << "IsStillViewingAd last_shown_notification_info_url: "
        << last_shown_notification_info_.url
        << " does not match last_shown_tab_url: " << active_tab_url_;

    return false;
  }

  return true;
}

void AdsImpl::ConfirmAd(
    const NotificationInfo& info,
    const ConfirmationType& type) {
  if (IsNotificationFromSampleCatalog(info)) {
    BLOG(INFO) << "Confirmation not made: Sample Ad";

    return;
  }

  auto notification_info = std::make_unique<NotificationInfo>(info);

  notification_info->type = type;

  GenerateAdReportingConfirmationEvent(*notification_info);

  ads_client_->ConfirmAd(std::move(notification_info));
}

void AdsImpl::ConfirmAction(
    const std::string& uuid,
    const std::string& creative_set_id,
    const ConfirmationType& type) {
  if (IsCreativeSetFromSampleCatalog(creative_set_id)) {
    BLOG(INFO) << "Confirmation not made: Sample Ad";

    return;
  }

  GenerateAdReportingConfirmationEvent(uuid, type);

  ads_client_->ConfirmAction(uuid, creative_set_id, type);
}

void AdsImpl::OnTimer(
    const uint32_t timer_id) {
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
  auto classifications =
      helper::Classification::GetClassifications(info.category);
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

      last_shown_notification_info_ = NotificationInfo(info);

      break;
    }

    case NotificationResultInfoResultType::DISMISSED: {
      writer.String("dismissed");

      break;
    }

    case NotificationResultInfoResultType::TIMEOUT: {
      writer.String("timeout");

      break;
    }
  }

  writer.String("notificationClassification");
  writer.StartArray();
  auto classifications =
      helper::Classification::GetClassifications(info.category);
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
  GenerateAdReportingConfirmationEvent(info.uuid, info.type);
}

void AdsImpl::GenerateAdReportingConfirmationEvent(
  const std::string& uuid,
  const ConfirmationType& type) {
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
  writer.String(uuid.c_str());

  writer.String("notificationType");
  auto confirmation_type = std::string(type);
  writer.String(confirmation_type.c_str());

  writer.EndObject();

  writer.EndObject();

  auto* json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::MaybeGenerateAdReportingLoadEvent(
    const std::string& url,
    const std::string& classification) {
  if (active_tab_url_ != url) {
    return;
  }

  LoadInfo load_info;
  load_info.tab_id = active_tab_id_;
  load_info.tab_url = active_tab_url_;
  load_info.tab_classification = classification;
  GenerateAdReportingLoadEvent(load_info);
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
  auto classifications =
      helper::Classification::GetClassifications(info.tab_classification);
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

  writer.String("locale");
  auto locale = ads_client_->GetLocale();
  writer.String(locale.c_str());

  writer.String("notifications");
  writer.StartObject();

  writer.String("shouldShow");
  auto should_show = ads_client_->ShouldShowNotifications();
  writer.Bool(should_show);

  writer.EndObject();

  writer.String("userModelLanguage");
  auto user_model_language = client_->GetUserModelLanguage();
  writer.String(user_model_language.c_str());

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

void AdsImpl::GenerateAdsHistoryEntry(
    const NotificationInfo& notification_info,
    const ConfirmationType& confirmation_type) {
  auto ad_history_detail = std::make_unique<AdHistoryDetail>();
  ad_history_detail->timestamp_in_seconds = Time::NowInSeconds();
  ad_history_detail->uuid = base::GenerateGUID();
  ad_history_detail->ad_content.uuid = notification_info.uuid;
  ad_history_detail->ad_content.creative_set_id =
      notification_info.creative_set_id;
  ad_history_detail->ad_content.brand = notification_info.advertiser;
  ad_history_detail->ad_content.brand_info = notification_info.text;
  ad_history_detail->ad_content.brand_display_url =
      GetDisplayUrl(notification_info.url);
  ad_history_detail->ad_content.brand_url = notification_info.url;
  ad_history_detail->ad_content.ad_action = confirmation_type;
  ad_history_detail->category_content.category = notification_info.category;

  client_->AppendAdToAdsShownHistory(*ad_history_detail);
}

bool AdsImpl::IsNotificationFromSampleCatalog(
    const NotificationInfo& info) const {
  return info.creative_set_id.empty();
}

bool AdsImpl::IsCreativeSetFromSampleCatalog(
  const std::string& creative_set_id) const {
  return creative_set_id.empty();
}

bool AdsImpl::IsSupportedUrl(
    const std::string& url) const {
  DCHECK(!url.empty()) << "Invalid URL";

  return GURL(url).SchemeIsHTTPOrHTTPS();
}

bool AdsImpl::UrlHostsMatch(
    const std::string& url_1,
    const std::string& url_2) const {
  return GURL(url_1).DomainIs(GURL(url_2).host_piece());
}

}  // namespace ads
