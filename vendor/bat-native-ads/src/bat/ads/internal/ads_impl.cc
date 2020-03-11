/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <fstream>
#include <functional>
#include <utility>
#include <vector>

#include "bat/ads/ad_history.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/ads_history.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/ad_notification_info.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/classification_helper.h"
#include "bat/ads/internal/locale_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/search_providers.h"
#include "bat/ads/internal/reports.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/uri_helper.h"
#include "bat/ads/internal/ad_events/ad_notification_event_factory.h"
#include "bat/ads/internal/event_type_blur_info.h"
#include "bat/ads/internal/event_type_destroy_info.h"
#include "bat/ads/internal/event_type_focus_info.h"
#include "bat/ads/internal/event_type_load_info.h"
#include "bat/ads/internal/filters/ads_history_filter_factory.h"
#include "bat/ads/internal/filters/ads_history_date_range_filter.h"
#include "bat/ads/internal/frequency_capping/exclusion_rule.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/daily_cap_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/total_max_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/minimum_wait_time_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/sorts/ads_history_sort_factory.h"

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

const char kCategoryDelimiter = '-';

std::string GetDisplayUrl(const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid())
    return std::string();

  return gurl.host();
}

}  // namespace

namespace ads {

AdsImpl::AdsImpl(AdsClient* ads_client) :
    is_foreground_(false),
    active_tab_id_(0),
    collect_activity_timer_id_(0),
    delivering_ad_notifications_timer_id_(0),
    sustained_ad_notification_interaction_timer_id_(0),
    next_easter_egg_timestamp_in_seconds_(0),
    client_(std::make_unique<Client>(this, ads_client)),
    bundle_(std::make_unique<Bundle>(this, ads_client)),
    ads_serve_(std::make_unique<AdsServe>(this, ads_client, bundle_.get())),
    frequency_capping_(std::make_unique<FrequencyCapping>(client_.get())),
    ad_notifications_(std::make_unique<AdNotifications>(this, ads_client)),
    ad_conversions_(std::make_unique<AdConversions>(
        this, ads_client, client_.get())),
    user_model_(nullptr),
    is_initialized_(false),
    is_confirmations_ready_(false),
    ads_client_(ads_client) {
}

AdsImpl::~AdsImpl() {
  StopCollectingActivity();
  StopDeliveringAdNotifications();
  StopSustainingAdNotificationInteraction();
}

AdsClient* AdsImpl::get_ads_client() const {
  return ads_client_;
}

Client* AdsImpl::get_client() const {
  return client_.get();
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
  ad_notifications_->Initialize(callback);
}

void AdsImpl::InitializeStep3(
    const Result result) {
  if (result != SUCCESS) {
    initialize_callback_(FAILED);
    return;
  }

  auto callback = std::bind(&AdsImpl::InitializeStep4, this, _1);
  ad_conversions_->Initialize(callback);
}

void AdsImpl::InitializeStep4(
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

void AdsImpl::InitializeStep5(
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

  ad_conversions_->ProcessQueue();

  MaybeServeAdNotification(false);

#if defined(OS_ANDROID)
    // Ad notifications do not sustain a reboot or update, so we should remove
    // orphaned ad notifications
    RemoveAllAdNotificationsAfterReboot();
    RemoveAllAdNotificationsAfterUpdate();
#endif

  client_->UpdateAdUUID();

  if (IsMobile()) {
    if (client_->GetNextCheckServeAdNotificationTimestampInSeconds() == 0) {
      StartDeliveringAdNotificationsAfterSeconds(
          2 * base::Time::kSecondsPerMinute);
    } else {
      StartDeliveringAdNotifications();
    }
  }

  if (_is_debug) {
    StartCollectingActivity(kDebugOneHourInSeconds);
  } else {
    StartCollectingActivity(base::Time::kSecondsPerHour);
  }

  ads_serve_->DownloadCatalog();
}

#if defined(OS_ANDROID)
void AdsImpl::RemoveAllAdNotificationsAfterReboot() {
  auto ads_shown_history = client_->GetAdsShownHistory();
  if (!ads_shown_history.empty()) {
    uint64_t ad_shown_timestamp =
        ads_shown_history.front().timestamp_in_seconds;
    uint64_t boot_timestamp = Time::NowInSeconds() -
        static_cast<uint64_t>(base::SysInfo::Uptime().InSeconds());
    if (ad_shown_timestamp <= boot_timestamp) {
      ad_notifications_->RemoveAll(false);
    }
  }
}

void AdsImpl::RemoveAllAdNotificationsAfterUpdate() {
  std::string current_version_code(
      base::android::BuildInfo::GetInstance()->package_version_code());
  std::string last_version_code = client_->GetVersionCode();
  if (last_version_code != current_version_code) {
    client_->SetVersionCode(current_version_code);
    ad_notifications_->RemoveAll(false);
  }
}
#endif

bool AdsImpl::IsInitialized() {
  if (!is_initialized_ || !ads_client_->IsEnabled()) {
    return false;
  }

  if (ShouldClassifyPagesIfTargeted() && !user_model_->IsInitialized()) {
    return false;
  }

  return true;
}

void AdsImpl::Shutdown(
    ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(WARNING) << "Shutdown failed as not initialized";

    callback(FAILED);
    return;
  }

  ad_notifications_->RemoveAll(true);

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
    InitializeStep5(SUCCESS);
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

bool AdsImpl::GetAdNotification(
    const std::string& uuid,
    AdNotificationInfo* notification) {
  return ad_notifications_->Get(uuid, notification);
}

void AdsImpl::OnForeground() {
  is_foreground_ = true;

  const Reports reports(this);
  const std::string report = reports.GenerateForegroundEventReport();
  ads_client_->EventLog(report);

  if (IsMobile() && !ads_client_->CanShowBackgroundNotifications()) {
    StartDeliveringAdNotifications();
  }
}

void AdsImpl::OnBackground() {
  is_foreground_ = false;

  const Reports reports(this);
  const std::string report = reports.GenerateBackgroundEventReport();
  ads_client_->EventLog(report);

  if (IsMobile() && !ads_client_->CanShowBackgroundNotifications()) {
    StopDeliveringAdNotifications();
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

  if (!IsInitialized()) {
    BLOG(WARNING) << "OnUnIdle failed as not initialized";
    return;
  }

  BLOG(INFO) << "Browser state changed to unidle";

  client_->UpdateLastUserIdleStopTime();

  if (IsMobile()) {
    return;
  }

  MaybeServeAdNotification(true);
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

void AdsImpl::OnAdNotificationEvent(
    const std::string& uuid,
    const AdNotificationEventType event_type) {
  AdNotificationInfo info;
  if (!ad_notifications_->Get(uuid, &info)) {
    NOTREACHED();
    return;
  }

  switch (event_type) {
    case AdNotificationEventType::kViewed: {
      last_shown_ad_notification_info_ = info;
      break;
    }

    case AdNotificationEventType::kClicked: {
      ad_notifications_->Remove(uuid, true);
      break;
    }

    case AdNotificationEventType::kDismissed: {
      ad_notifications_->Remove(uuid, false);
      break;
    }

    case AdNotificationEventType::kTimedOut: {
      ad_notifications_->Remove(uuid, false);
      break;
    }
  }

  const auto ad_event = AdEventFactory::Build(this, event_type);
  DCHECK(ad_event);
  if (!ad_event) {
    return;
  }

  ad_event->Trigger(info);
}

bool AdsImpl::ShouldNotDisturb() const {
  if (!IsAndroid()) {
    return false;
  }

  if (IsForeground()) {
    return false;
  }

  auto now = base::Time::Now();
  base::Time::Exploded now_exploded;
  now.LocalExplode(&now_exploded);

  if (now_exploded.hour >= kDoNotDisturbToHour &&
      now_exploded.hour <= kDoNotDisturbFromHour) {
    return false;
  }

  return true;
}

bool AdsImpl::IsAndroid() const {
  ClientInfo client_info;
  ads_client_->GetClientInfo(&client_info);

  if (client_info.platform != ANDROID_OS) {
    return false;
  }

  return true;
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

    const Reports reports(this);
    FocusInfo focus_info;
    focus_info.tab_id = tab_id;
    const std::string report = reports.GenerateFocusEventReport(focus_info);
    ads_client_->EventLog(report);
  } else {
    BLOG(INFO) << "OnTabUpdated.IsBlurred for tab id: " << tab_id
        << " and url: " << url;

    const Reports reports(this);
    BlurInfo blur_info;
    blur_info.tab_id = tab_id;
    const std::string report = reports.GenerateBlurEventReport(blur_info);
    ads_client_->EventLog(report);
  }
}

void AdsImpl::OnTabClosed(
    const int32_t tab_id) {
  BLOG(INFO) << "OnTabClosed for tab id: " << tab_id;

  OnMediaStopped(tab_id);

  const Reports reports(this);
  DestroyInfo destroy_info;
  destroy_info.tab_id = tab_id;
  const std::string report = reports.GenerateDestroyEventReport(destroy_info);
  ads_client_->EventLog(report);
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

AdsHistory AdsImpl::GetAdsHistory(
    const AdsHistory::FilterType filter_type,
    const AdsHistory::SortType sort_type,
    const uint64_t from_timestamp,
    const uint64_t to_timestamp) {
  auto history = client_->GetAdsShownHistory();

  const auto date_range_filter = std::make_unique<AdsHistoryDateRangeFilter>();
  DCHECK(date_range_filter);
  if (date_range_filter) {
    history = date_range_filter->Apply(history, from_timestamp, to_timestamp);
  }

  const auto filter = AdsHistoryFilterFactory::Build(filter_type);
  DCHECK(filter);
  if (filter) {
    history = filter->Apply(history);
  }

  const auto sort = AdsHistorySortFactory::Build(sort_type);
  DCHECK(sort);
  if (sort) {
    history = sort->Apply(history);
  }

  AdsHistory ads_history;
  for (const auto& entry : history) {
    ads_history.entries.push_back(entry);
  }

  return ads_history;
}

AdContent::LikeAction AdsImpl::ToggleAdThumbUp(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContent::LikeAction& action) {
  auto like_action =
      client_->ToggleAdThumbUp(creative_instance_id, creative_set_id, action);
  if (like_action == AdContent::LikeAction::kThumbsUp) {
    ConfirmAction(creative_instance_id, creative_set_id,
        ConfirmationType::kUpvoted);
  }

  return like_action;
}

AdContent::LikeAction AdsImpl::ToggleAdThumbDown(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContent::LikeAction& action) {
  auto like_action =
      client_->ToggleAdThumbDown(creative_instance_id, creative_set_id, action);
  if (like_action == AdContent::LikeAction::kThumbsDown) {
    ConfirmAction(creative_instance_id, creative_set_id,
        ConfirmationType::kDownvoted);
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
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool saved) {
  return client_->ToggleSaveAd(creative_instance_id, creative_set_id, saved);
}

bool AdsImpl::ToggleFlagAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool flagged) {
  auto flag_ad =
      client_->ToggleFlagAd(creative_instance_id, creative_set_id, flagged);
  if (flag_ad) {
    ConfirmAction(creative_instance_id, creative_set_id,
        ConfirmationType::kFlagged);
  }

  return flag_ad;
}

void AdsImpl::ChangeLocale(
    const std::string& locale) {
  auto language = helper::Locale::GetLanguageCode(locale);

  if (!ShouldClassifyPagesIfTargeted()) {
    client_->SetUserModelLanguage(language);

    InitializeStep5(SUCCESS);
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
    const std::string& content) {
  DCHECK(!url.empty());

  if (!IsInitialized()) {
    BLOG(WARNING) << "OnPageLoaded failed as not initialized";
    return;
  }

  if (url.empty()) {
    BLOG(INFO) << "Site visited, empty URL";
    return;
  }

  CheckAdConversion(url);

  if (helper::Uri::MatchesDomainOrHost(url,
      last_shown_ad_notification_info_.target_url)) {
    BLOG(INFO) << "Site visited " << url
        << ", domain matches the last shown ad notification for "
            << last_shown_ad_notification_info_.target_url;

    const std::string domain = GURL(url).host();
    if (last_sustained_ad_notification_domain_ != domain) {
      last_sustained_ad_notification_domain_ = domain;

      StartSustainingAdNotificationInteraction(
          kSustainAdNotificationInteractionAfterSeconds);
    } else {
      BLOG(INFO) << "Already sustaining ad notification interaction for "
          << url;
    }

    return;
  }

  if (!last_shown_ad_notification_info_.target_url.empty()) {
    BLOG(INFO) << "Site visited " << url
      << ", domain does not match the last shown ad notification for "
          << last_shown_ad_notification_info_.target_url;
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

  MaybeClassifyPage(url, content);

  CheckEasterEgg(url);

  BLOG(INFO) << "Site visited " << url << ", previous tab url was "
      << previous_tab_url_;
}

void AdsImpl::CheckAdConversion(
    const std::string& url) {
  DCHECK(!url.empty());
  if (url.empty()) {
    return;
  }

  if (!ads_client_->ShouldAllowAdConversionTracking()) {
    return;
  }

  auto callback = std::bind(&AdsImpl::OnGetAdConversions, this, _1, _2, _3);
  ads_client_->GetAdConversions(url, callback);
}

void AdsImpl::OnGetAdConversions(
    const Result result,
    const std::string& url,
    const AdConversionList& ad_conversions) {
  for (const auto& ad_conversion : ad_conversions) {
    if (!helper::Uri::MatchesWildcard(url, ad_conversion.url_pattern)) {
      continue;
    }

    ConfirmationType confirmation_type;
    if (ad_conversion.type == "postview") {
      confirmation_type = ConfirmationType::kViewed;
    } else if (ad_conversion.type == "postclick") {
      confirmation_type = ConfirmationType::kClicked;
    } else {
      BLOG(WARNING) << "Unsupported ad conversion type: " << ad_conversion.type;
      continue;
    }

    auto ads_history  = client_->GetAdsShownHistory();
    const auto sort =
        AdsHistorySortFactory::Build(AdsHistory::SortType::kDescendingOrder);
    DCHECK(sort);
    if (sort) {
      ads_history = sort->Apply(ads_history);
    }

    for (const auto& ad : ads_history) {
      auto ad_conversion_history = client_->GetAdConversionHistory();
      if (ad_conversion_history.find(ad.ad_content.creative_set_id) !=
          ad_conversion_history.end()) {
        continue;
      }

      if (ad_conversion.creative_set_id != ad.ad_content.creative_set_id) {
        continue;
      }

      if (confirmation_type != ad.ad_content.ad_action) {
        continue;
      }

      const base::Time observation_window = base::Time::Now() -
          base::TimeDelta::FromDays(ad_conversion.observation_window);
      const base::Time time = Time::FromDoubleT(ad.timestamp_in_seconds);
      if (observation_window > time) {
        continue;
      }

      ad_conversions_->AddToQueue(ad.ad_content.creative_instance_id,
          ad.ad_content.creative_set_id);
    }
  }
}

void AdsImpl::MaybeClassifyPage(
    const std::string& url,
    const std::string& content) {
  LoadInfo load_info;
  load_info.tab_id = active_tab_id_;
  load_info.tab_url = active_tab_url_;

  if (ShouldClassifyPagesIfTargeted()) {
    load_info.tab_classification = ClassifyPage(url, content);
  } else {
    load_info.tab_classification = kUntargetedPageClassification;
  }

  const Reports reports(this);
  const std::string report = reports.GenerateLoadEventReport(load_info);
  ads_client_->EventLog(report);
}

bool AdsImpl::ShouldClassifyPagesIfTargeted() const {
  const std::string locale = ads_client_->GetLocale();
  const std::string region = helper::Locale::GetRegionCode(locale);

  auto targeted = false;

  for (const auto& schema : kSupportedRegionsSchemas) {
    const std::map<std::string, bool> regions = schema.second;
    const auto it = regions.find(region);
    if (it != regions.end()) {
      targeted = it->second;
      break;
    }
  }

  return targeted;
}

std::string AdsImpl::ClassifyPage(
    const std::string& url,
    const std::string& content) {
  auto page_score = user_model_->ClassifyPage(content);

  auto winning_category = GetWinningCategory(page_score);
  if (winning_category.empty()) {
    BLOG(INFO) << "Failed to classify page at " << url
        << " as not enough content";
    return "";
  }

  client_->SetLastPageClassification(winning_category);

  client_->AppendPageScoreToPageScoreHistory(page_score);

  CachePageScore(active_tab_url_, page_score);

  const auto winning_categories = GetWinningCategories();

  BLOG(INFO) << "Successfully classified page at " << url << " as "
      << winning_category << ". Winning category over time is "
      << winning_categories.front();

  return winning_category;
}

std::vector<std::string> AdsImpl::GetWinningCategories() {
  auto page_score_history = client_->GetPageScoreHistory();
  if (page_score_history.size() == 0) {
    return {};
  }

  uint64_t count = page_score_history.front().size();

  std::vector<double> winning_category_page_scores(count);
  std::fill(winning_category_page_scores.begin(),
      winning_category_page_scores.end(), 0.0);

  for (const auto& page_score : page_score_history) {
    DCHECK(page_score.size() == count);

    for (size_t i = 0; i < page_score.size(); i++) {
      auto taxonomy = user_model_->GetTaxonomyAtIndex(i);
      if (client_->IsFilteredCategory(taxonomy)) {
        BLOG(INFO) << taxonomy
                   << " taxonomy has been excluded from the winner over time";

        continue;
      }

      winning_category_page_scores[i] += page_score[i];
    }
  }

  auto sorted_winning_category_page_scores = winning_category_page_scores;
  std::sort(sorted_winning_category_page_scores.begin(),
      sorted_winning_category_page_scores.end(), std::greater<double>());

  std::vector<std::string> winning_categories;
  for (const auto& page_score : sorted_winning_category_page_scores) {
    if (page_score == 0.0) {
      continue;
    }

    auto it = std::find(winning_category_page_scores.begin(),
        winning_category_page_scores.end(), page_score);
    const int index = std::distance(winning_category_page_scores.begin(), it);
    const std::string category = user_model_->GetTaxonomyAtIndex(index);
    if (category.empty()) {
      continue;
    }

    if (std::find(winning_categories.begin(), winning_categories.end(),
        category) != winning_categories.end()) {
      continue;
    }

    winning_categories.push_back(category);

    if (winning_categories.size() == kWinningCategoryCountForServingAds) {
      break;
    }
  }

  return winning_categories;
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

const std::map<std::string, std::vector<double>>&
AdsImpl::GetPageScoreCache() const {
  return page_score_cache_;
}

void AdsImpl::TestShoppingData(
    const std::string& url) {
  if (!IsInitialized()) {
    BLOG(WARNING) << "TestShoppingData failed as not initialized";
    return;
  }

  if (helper::Uri::MatchesDomainOrHost(url, kShoppingStateUrl)) {
    client_->FlagShoppingState(url, 1.0);
  } else {
    client_->UnflagShoppingState();
  }
}

bool AdsImpl::TestSearchState(
    const std::string& url) {
  if (!IsInitialized()) {
    BLOG(WARNING) << "TestSearchState failed as not initialized";
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
    BLOG(WARNING) << "ServeSampleAd failed as not initialized";
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

  auto categories = state.creative_ad_notifications.begin();
  auto categories_count = state.creative_ad_notifications.size();
  if (categories_count == 0) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no categories' }

    BLOG(INFO) << "Notification not made: No sample bundle ad notification "
        "categories";

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
  ShowAdNotification(ad);
}

void AdsImpl::CheckEasterEgg(
    const std::string& url) {
  if (!_is_testing) {
    return;
  }

  auto now_in_seconds = Time::NowInSeconds();

  if (helper::Uri::MatchesDomainOrHost(url, kEasterEggUrl) &&
      next_easter_egg_timestamp_in_seconds_ < now_in_seconds) {
    BLOG(INFO) << "Collect easter egg";

    ServeAdNotificationIfReady(true);

    next_easter_egg_timestamp_in_seconds_ =
        now_in_seconds + kNextEasterEggStartsInSeconds;

    BLOG(INFO) << "Next easter egg available in "
        << next_easter_egg_timestamp_in_seconds_ << " seconds";
  }
}

void AdsImpl::ServeAdNotificationIfReady(
    const bool should_force) {
  if (!IsInitialized()) {
    FailedToServeAdNotification("Not initialized");
    return;
  }

  if (!bundle_->IsReady()) {
    FailedToServeAdNotification("Bundle not ready");
    return;
  }

  if (!should_force) {
    if (!is_confirmations_ready_) {
      FailedToServeAdNotification("Confirmations not ready");
      return;
    }

    if (!IsAndroid() && !IsForeground()) {
      FailedToServeAdNotification("Not in foreground");
      return;
    }

    if (IsMediaPlaying()) {
      FailedToServeAdNotification("Media playing in browser");
      return;
    }

    if (ShouldNotDisturb()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'do not disturb while not in
      // foreground' }

      FailedToServeAdNotification("Should not disturb");
      return;
    }

    if (!IsAllowedToServeAdNotifications()) {
      FailedToServeAdNotification("Not allowed based on history");
      return;
    }
  }

  auto categories = GetWinningCategories();
  ServeAdNotificationFromCategories(categories);
}

void AdsImpl::ServeAdNotificationFromCategories(
    const std::vector<std::string>& categories) {
  std::string catalog_id = bundle_->GetCatalogId();
  if (catalog_id.empty()) {
    FailedToServeAdNotification("No ad catalog");
    return;
  }

  if (categories.empty()) {
    BLOG(INFO) << "No categories";
    ServeUntargetedAdNotification();
    return;
  }

  BLOG(INFO) << "Serving ad from categories:";
  for (const auto& category : categories) {
    BLOG(INFO) << "  " << category;
  }

  auto callback = std::bind(&AdsImpl::OnServeAdNotificationFromCategories,
      this, _1, _2, _3);
  ads_client_->GetCreativeAdNotifications(categories, callback);
}

void AdsImpl::OnServeAdNotificationFromCategories(
    const Result result,
    const std::vector<std::string>& categories,
    const CreativeAdNotificationList& ads) {
  auto eligible_ads = GetEligibleAds(ads);
  if (!eligible_ads.empty()) {
    BLOG(INFO) << "Found " << eligible_ads.size() << " eligible ads";
    ServeAdNotification(eligible_ads);
    return;
  }

  BLOG(INFO) << "No eligible ads found in categories:";
  for (const auto& category : categories) {
    BLOG(INFO) << "  " << category;
  }

  if (ServeAdNotificationFromParentCategories(categories)) {
    return;
  }

  ServeUntargetedAdNotification();
}

bool AdsImpl::ServeAdNotificationFromParentCategories(
    const std::vector<std::string>& categories) {
  std::vector<std::string> parent_categories;
  for (const auto& category : categories) {
    auto pos = category.find_last_of(kCategoryDelimiter);
    if (pos == std::string::npos) {
      return false;
    }

    std::string parent_category = category.substr(0, pos);

    if (std::find(parent_categories.begin(), parent_categories.end(),
        parent_category) != parent_categories.end()) {
      continue;
    }

    parent_categories.push_back(parent_category);
  }

  BLOG(INFO) << "Serving ad from parent categories:";
  for (const auto& parent_category : parent_categories) {
    BLOG(INFO) << "  " << parent_category;
  }

  auto callback = std::bind(&AdsImpl::OnServeAdNotificationFromCategories,
      this, _1, _2, _3);
  ads_client_->GetCreativeAdNotifications(parent_categories, callback);

  return true;
}

void AdsImpl::ServeUntargetedAdNotification() {
  BLOG(INFO) << "Serving ad notification from untargeted category";

  std::vector<std::string> categories = {
    kUntargetedPageClassification
  };

  auto callback = std::bind(&AdsImpl::OnServeUntargetedAdNotification,
      this, _1, _2, _3);
  ads_client_->GetCreativeAdNotifications(categories, callback);
}

void AdsImpl::OnServeUntargetedAdNotification(
    const Result result,
    const std::vector<std::string>& categories,
    const CreativeAdNotificationList& ads) {
  auto eligible_ads = GetEligibleAds(ads);
  if (eligible_ads.empty()) {
    FailedToServeAdNotification("No eligible ads found");
    return;
  }

  BLOG(INFO) << "Found " << eligible_ads.size() << " eligible ads";
  ServeAdNotification(eligible_ads);
}

void AdsImpl::ServeAdNotification(
    const CreativeAdNotificationList& ads) {
  auto rand = base::RandInt(0, ads.size() - 1);
  auto ad = ads.at(rand);
  ShowAdNotification(ad);

  SuccessfullyServedAd();
}

void AdsImpl::SuccessfullyServedAd() {
  if (IsMobile()) {
    StartDeliveringAdNotificationsAfterSeconds(
        base::Time::kSecondsPerHour / ads_client_->GetAdsPerHour());
  }
}

void AdsImpl::FailedToServeAdNotification(
    const std::string& reason) {
  BLOG(INFO) << "Notification not made: " << reason;

  if (IsMobile()) {
    StartDeliveringAdNotificationsAfterSeconds(
        2 * base::Time::kSecondsPerMinute);
  }
}

std::vector<std::unique_ptr<ExclusionRule>>
    AdsImpl::CreateExclusionRules() const {
  std::vector<std::unique_ptr<ExclusionRule>> exclusion_rules;

  std::unique_ptr<ExclusionRule> daily_cap_frequency_cap =
      std::make_unique<DailyCapFrequencyCap>(frequency_capping_.get());
  exclusion_rules.push_back(std::move(daily_cap_frequency_cap));

  std::unique_ptr<ExclusionRule> per_day_frequency_cap =
      std::make_unique<PerDayFrequencyCap>(frequency_capping_.get());
  exclusion_rules.push_back(std::move(per_day_frequency_cap));

  std::unique_ptr<ExclusionRule> per_hour_frequency_cap =
      std::make_unique<PerHourFrequencyCap>(frequency_capping_.get());
  exclusion_rules.push_back(std::move(per_hour_frequency_cap));

  std::unique_ptr<ExclusionRule> total_max_frequency_cap =
      std::make_unique<TotalMaxFrequencyCap>(frequency_capping_.get());
  exclusion_rules.push_back(std::move(total_max_frequency_cap));

  return exclusion_rules;
}

CreativeAdNotificationList AdsImpl::GetEligibleAds(
    const CreativeAdNotificationList& ads) {
  CreativeAdNotificationList eligible_ads;

  const auto exclusion_rules = CreateExclusionRules();

  auto unseen_ads = GetUnseenAdsAndRoundRobinIfNeeded(ads);
  for (const auto& ad : unseen_ads) {
    bool should_exclude = false;

    for (const auto& exclusion_rule : exclusion_rules) {
      if (!exclusion_rule->ShouldExclude(ad)) {
        continue;
      }

      BLOG(INFO) << exclusion_rule->GetLastMessage();
      should_exclude = true;
    }

    if (should_exclude) {
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

CreativeAdNotificationList AdsImpl::GetUnseenAdsAndRoundRobinIfNeeded(
    const CreativeAdNotificationList& ads) const {
  if (ads.empty()) {
    return ads;
  }

  CreativeAdNotificationList ads_for_unseen_advertisers =
      GetAdsForUnseenAdvertisers(ads);
  if (ads_for_unseen_advertisers.empty()) {
    BLOG(INFO) << "All advertisers have been shown, so round robin";

    const bool should_not_show_last_advertiser =
        client_->GetSeenAdvertisers().size() > 1 ? true : false;

    client_->ResetSeenAdvertisers(ads);

    ads_for_unseen_advertisers = GetAdsForUnseenAdvertisers(ads);

    if (should_not_show_last_advertiser) {
      const auto it = std::remove_if(ads_for_unseen_advertisers.begin(),
          ads_for_unseen_advertisers.end(),
              [&](CreativeAdNotificationInfo& info) {
        return info.advertiser_id ==
            last_shown_creative_ad_notification_info_.advertiser_id;
      });

      ads_for_unseen_advertisers.erase(it, ads_for_unseen_advertisers.end());
    }
  }

  CreativeAdNotificationList unseen_ads =
      GetUnseenAds(ads_for_unseen_advertisers);
  if (unseen_ads.empty()) {
    BLOG(INFO) << "All ads have been shown, so round robin";

    const bool should_not_show_last_ad =
        client_->GetSeenAdNotifications().size() > 1 ? true : false;

    client_->ResetSeenAdNotifications(ads);

    unseen_ads = GetUnseenAds(ads);

    if (should_not_show_last_ad) {
      const auto it = std::remove_if(ads_for_unseen_advertisers.begin(),
          ads_for_unseen_advertisers.end(),
              [&](CreativeAdNotificationInfo& info) {
        return info.creative_instance_id ==
            last_shown_creative_ad_notification_info_.creative_instance_id;
      });

      ads_for_unseen_advertisers.erase(it, ads_for_unseen_advertisers.end());
    }
  }

  return unseen_ads;
}

CreativeAdNotificationList AdsImpl::GetUnseenAds(
    const CreativeAdNotificationList& ads) const {
  auto unseen_ads = ads;
  const auto seen_ads = client_->GetSeenAdNotifications();
  const auto seen_advertisers = client_->GetSeenAdvertisers();

  const auto it = std::remove_if(unseen_ads.begin(), unseen_ads.end(),
      [&](CreativeAdNotificationInfo& info) {
    return seen_ads.find(info.creative_instance_id) != seen_ads.end() &&
        seen_ads.find(info.advertiser_id) != seen_advertisers.end();
  });

  unseen_ads.erase(it, unseen_ads.end());

  return unseen_ads;
}

CreativeAdNotificationList AdsImpl::GetAdsForUnseenAdvertisers(
    const CreativeAdNotificationList& ads) const {
  auto unseen_ads = ads;
  const auto seen_ads = client_->GetSeenAdvertisers();

  const auto it = std::remove_if(unseen_ads.begin(), unseen_ads.end(),
      [&seen_ads](CreativeAdNotificationInfo& info) {
    return seen_ads.find(info.advertiser_id) != seen_ads.end();
  });

  unseen_ads.erase(it, unseen_ads.end());

  return unseen_ads;
}

bool AdsImpl::IsAdNotificationValid(
    const CreativeAdNotificationInfo& info) {
  if (info.title.empty() ||
      info.body.empty() ||
      info.target_url.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'incomplete ad information',
    // category, winnerOverTime, arbitraryKey, notificationUrl,
    // notificationText, advertiser

    BLOG(INFO) << "Ad notification not made: Incomplete ad information"
        << std::endl << "  creativeInstanceId: " << info.creative_instance_id
        << std::endl << "  creativeSetId: " << info.creative_set_id
        << std::endl << "  campaignId: " << info.campaign_id
        << std::endl << "  title: " << info.title
        << std::endl << "  body: " << info.body
        << std::endl << "  targetUrl: " << info.target_url;

    return false;
  }

  return true;
}

bool AdsImpl::ShowAdNotification(
    const CreativeAdNotificationInfo& info) {
  if (!IsAdNotificationValid(info)) {
    return false;
  }

  auto now_in_seconds = Time::NowInSeconds();

  client_->AppendTimestampToCreativeSetHistory(
      info.creative_set_id, now_in_seconds);
  client_->AppendTimestampToCampaignHistory(
      info.campaign_id, now_in_seconds);

  client_->UpdateSeenAdNotification(info.creative_instance_id, 1);
  client_->UpdateSeenAdvertiser(info.advertiser_id, 1);

  last_shown_creative_ad_notification_info_ = info;

  auto ad_notification = std::make_unique<AdNotificationInfo>();
  ad_notification->uuid = base::GenerateGUID();
  ad_notification->parent_uuid = base::GenerateGUID();
  ad_notification->creative_instance_id = info.creative_instance_id;
  ad_notification->creative_set_id = info.creative_set_id;
  ad_notification->category = info.category;
  ad_notification->title = info.title;
  ad_notification->body = info.body;
  ad_notification->target_url = helper::Uri::GetUri(info.target_url);

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Notification shown', {category, winnerOverTime, arbitraryKey,
  // notificationUrl, notificationText, advertiser, uuid, hierarchy}

  BLOG(INFO) << "Ad notification shown:"
      << std::endl << "  uuid: " << ad_notification->uuid
      << std::endl << "  parentUuid: " << ad_notification->parent_uuid
      << std::endl << "  creativeInstanceId: "
          << ad_notification->creative_instance_id
      << std::endl << "  creativeSetId: " << ad_notification->creative_set_id
      << std::endl << "  category: " << ad_notification->category
      << std::endl << "  title: " << ad_notification->title
      << std::endl << "  body: " << ad_notification->body
      << std::endl << "  targetUrl: " << ad_notification->target_url;

  ad_notifications_->PushBack(*ad_notification);

  if (kMaximumAdNotifications > 0 &&
      ad_notifications_->Count() > kMaximumAdNotifications) {
    ad_notifications_->PopFront(true);
  }

  return true;
}

std::vector<std::unique_ptr<PermissionRule>>
    AdsImpl::CreatePermissionRules() const {
  std::vector<std::unique_ptr<PermissionRule>> permission_rules;

  std::unique_ptr<PermissionRule> ads_per_hour_frequency_cap =
      std::make_unique<AdsPerHourFrequencyCap>(this, ads_client_,
          frequency_capping_.get());
  permission_rules.push_back(std::move(ads_per_hour_frequency_cap));

  std::unique_ptr<PermissionRule> minimum_wait_time_frequency_cap =
      std::make_unique<MinimumWaitTimeFrequencyCap>(this, ads_client_,
          frequency_capping_.get());
  permission_rules.push_back(std::move(minimum_wait_time_frequency_cap));

  std::unique_ptr<PermissionRule> ads_per_day_frequency_cap =
      std::make_unique<AdsPerDayFrequencyCap>(ads_client_,
          frequency_capping_.get());
  permission_rules.push_back(std::move(ads_per_day_frequency_cap));

  return permission_rules;
}

bool AdsImpl::IsAllowedToServeAdNotifications() {
  const auto permission_rules = CreatePermissionRules();

  bool is_allowed = true;

  for (const auto& permission_rule : permission_rules) {
    if (permission_rule->IsAllowed()) {
      continue;
    }

    BLOG(INFO) << permission_rule->GetLastMessage();
    is_allowed = false;
  }

  return is_allowed;
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
    BLOG(WARNING) << "CollectActivity failed as not initialized";
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

void AdsImpl::StartDeliveringAdNotifications() {
  StopDeliveringAdNotifications();

  auto now_in_seconds = Time::NowInSeconds();
  auto next_check_serve_ad_timestamp_in_seconds =
      client_->GetNextCheckServeAdNotificationTimestampInSeconds();

  uint64_t start_timer_in;
  if (now_in_seconds >= next_check_serve_ad_timestamp_in_seconds) {
    // Browser was launched after the next check to serve an ad
    start_timer_in = 1 * base::Time::kSecondsPerMinute;
  } else {
    start_timer_in = next_check_serve_ad_timestamp_in_seconds - now_in_seconds;
  }

  delivering_ad_notifications_timer_id_ = ads_client_->SetTimer(start_timer_in);
  if (delivering_ad_notifications_timer_id_ == 0) {
    BLOG(ERROR) <<
        "Failed to start delivering ad notifications due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start delivering ad notifications in "
      << start_timer_in << " seconds";
}

void AdsImpl::StartDeliveringAdNotificationsAfterSeconds(
    const uint64_t seconds) {
  auto timestamp_in_seconds = Time::NowInSeconds() + seconds;
  client_->SetNextCheckServeAdNotificationTimestampInSeconds(
      timestamp_in_seconds);

  StartDeliveringAdNotifications();
}

void AdsImpl::DeliverAdNotification() {
  MaybeServeAdNotification(true);
}

void AdsImpl::StopDeliveringAdNotifications() {
  if (!IsDeliveringAdNotifications()) {
    return;
  }

  BLOG(INFO) << "Stopped delivering ad notifications";

  ads_client_->KillTimer(delivering_ad_notifications_timer_id_);
  delivering_ad_notifications_timer_id_ = 0;
}

bool AdsImpl::IsDeliveringAdNotifications() const {
  if (delivering_ad_notifications_timer_id_ == 0) {
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

void AdsImpl::MaybeServeAdNotification(
    const bool should_serve) {
  auto ok = ads_client_->ShouldShowNotifications();

  // TODO(Terry Mancey): Implement Log (#44)
  // appConstants.APP_ON_NATIVE_NOTIFICATION_AVAILABLE_CHECK, {err, result}

  auto previous = client_->GetAvailable();

  if (ok != previous) {
    client_->SetAvailable(ok);
  }

  if (!should_serve || ok != previous) {
    const Reports reports(this);
    const std::string report = reports.GenerateSettingsEventReport();
    ads_client_->EventLog(report);
  }

  if (!should_serve) {
    return;
  }

  if (!ok) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'notifications not presently allowed'
    // }

    FailedToServeAdNotification("Notifications not allowed");
    return;
  }

  if (!ads_client_->IsNetworkConnectionAvailable()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'network connection not available' }

    FailedToServeAdNotification("Network connection not available");
    return;
  }

  if (IsCatalogOlderThanOneDay()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'catalog older than one day' }

    FailedToServeAdNotification("Catalog older than one day");
    return;
  }

  ServeAdNotificationIfReady(false);
}

void AdsImpl::StartSustainingAdNotificationInteraction(
    const uint64_t start_timer_in) {
  StopSustainingAdNotificationInteraction();

  sustained_ad_notification_interaction_timer_id_ =
      ads_client_->SetTimer(start_timer_in);
  if (sustained_ad_notification_interaction_timer_id_ == 0) {
    BLOG(ERROR) <<
        "Failed to start sustaining ad notification interaction due to an "
            "invalid timer";

    return;
  }

  BLOG(INFO) << "Start sustaining ad notification interaction in "
      << start_timer_in << " seconds";
}

void AdsImpl::SustainAdNotificationInteractionIfNeeded() {
  if (!IsStillViewingAdNotification()) {
    BLOG(INFO) << "Failed to sustain ad notification interaction, domain for "
        "the focused tab does not match the last shown ad notification for "
            << last_shown_ad_notification_info_.target_url;
    return;
  }

  BLOG(INFO) << "Sustained ad notification interaction";

  ConfirmAdNotification(last_shown_ad_notification_info_,
      ConfirmationType::kLanded);
}

void AdsImpl::StopSustainingAdNotificationInteraction() {
  if (!IsSustainingAdNotificationInteraction()) {
    return;
  }

  BLOG(INFO) << "Stopped sustaining ad notification interaction";

  ads_client_->KillTimer(sustained_ad_notification_interaction_timer_id_);
  sustained_ad_notification_interaction_timer_id_ = 0;
}

bool AdsImpl::IsSustainingAdNotificationInteraction() const {
  if (sustained_ad_notification_interaction_timer_id_ == 0) {
    return false;
  }

  return true;
}

bool AdsImpl::IsStillViewingAdNotification() const {
  return helper::Uri::MatchesDomainOrHost(active_tab_url_,
      last_shown_ad_notification_info_.target_url);
}

void AdsImpl::ConfirmAdNotification(
    const AdNotificationInfo& info,
    const ConfirmationType& confirmation_type) {
  if (IsCreativeSetFromSampleCatalog(info.creative_set_id)) {
    BLOG(INFO) << "Confirmation not made: Sample Ad";

    return;
  }

  auto notification_info = std::make_unique<AdNotificationInfo>(info);
  notification_info->confirmation_type = confirmation_type;

  const Reports reports(this);
  const std::string report = reports.GenerateConfirmationEventReport(
      info.creative_instance_id, confirmation_type);
  ads_client_->EventLog(report);

  ads_client_->ConfirmAdNotification(std::move(notification_info));
}

void AdsImpl::ConfirmAction(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const ConfirmationType& confirmation_type) {
  if (IsCreativeSetFromSampleCatalog(creative_set_id)) {
    BLOG(INFO) << "Confirmation not made: Sample Ad";

    return;
  }

  const Reports reports(this);
  const std::string report = reports.GenerateConfirmationEventReport(
      creative_instance_id, confirmation_type);
  ads_client_->EventLog(report);

  ads_client_->ConfirmAction(creative_instance_id, creative_set_id,
      confirmation_type);
}

void AdsImpl::OnTimer(
    const uint32_t timer_id) {
  BLOG(INFO) << "OnTimer: " << std::endl
      << "  timer_id: " << std::to_string(timer_id) << std::endl
      << "  collect_activity_timer_id_: "
      << std::to_string(collect_activity_timer_id_) << std::endl
      << "  delivering_ad_notifications_timer_id_: "
      << std::to_string(delivering_ad_notifications_timer_id_) << std::endl
      << "  sustained_ad_notification_interaction_timer_id_: "
      << std::to_string(sustained_ad_notification_interaction_timer_id_);
  if (timer_id == collect_activity_timer_id_) {
    CollectActivity();
  } else if (timer_id == delivering_ad_notifications_timer_id_) {
    DeliverAdNotification();
  } else if (timer_id == sustained_ad_notification_interaction_timer_id_) {
    SustainAdNotificationInteractionIfNeeded();
  } else if (ad_conversions_->OnTimer(timer_id)) {
    return;
  } else {
    BLOG(WARNING) << "Unexpected OnTimer: " << std::to_string(timer_id);
  }
}

void AdsImpl::AppendAdNotificationToAdsHistory(
    const AdNotificationInfo& info,
    const ConfirmationType& confirmation_type) {
  auto ad_history = std::make_unique<AdHistory>();
  ad_history->timestamp_in_seconds = Time::NowInSeconds();
  ad_history->uuid = base::GenerateGUID();
  ad_history->parent_uuid = info.parent_uuid;
  ad_history->ad_content.creative_instance_id = info.creative_instance_id;
  ad_history->ad_content.creative_set_id = info.creative_set_id;
  ad_history->ad_content.brand = info.title;
  ad_history->ad_content.brand_info = info.body;
  ad_history->ad_content.brand_display_url = GetDisplayUrl(info.target_url);
  ad_history->ad_content.brand_url = info.target_url;
  ad_history->ad_content.ad_action = confirmation_type;
  ad_history->category_content.category = info.category;

  client_->AppendAdHistoryToAdsShownHistory(*ad_history);
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

}  // namespace ads
