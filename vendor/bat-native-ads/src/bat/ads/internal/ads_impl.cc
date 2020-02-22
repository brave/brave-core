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
#include "bat/ads/notification_info.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/classification_helper.h"
#include "bat/ads/internal/locale_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/search_providers.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/uri_helper.h"
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
    last_sustained_ad_domain_(""),
    next_easter_egg_timestamp_in_seconds_(0),
    client_(std::make_unique<Client>(this, ads_client)),
    bundle_(std::make_unique<Bundle>(this, ads_client)),
    ads_serve_(std::make_unique<AdsServe>(this, ads_client, bundle_.get())),
    frequency_capping_(std::make_unique<FrequencyCapping>(client_.get())),
    notifications_(std::make_unique<Notifications>(this, ads_client)),
    ad_conversions_(std::make_unique<AdConversionTracking>(
        this, ads_client, client_.get())),
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

  NotificationAllowedCheck(false);

#if defined(OS_ANDROID)
    RemoveAllNotificationsAfterReboot();
    RemoveAllNotificationsAfterUpdate();
#endif

  client_->UpdateAdUUID();

  if (IsMobile()) {
    if (client_->GetNextCheckServeAdTimestampInSeconds() == 0) {
      StartDeliveringNotificationsAfterSeconds(
          2 * base::Time::kSecondsPerMinute);
    } else {
      StartDeliveringNotifications();
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
  if (last_version_code != current_version_code) {
    client_->SetVersionCode(current_version_code);
    // ads notifications don't sustain app update, so remove them
    notifications_->RemoveAll(false);
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

bool AdsImpl::GetNotificationForId(
    const std::string& id,
    ads::NotificationInfo* notification) {
  return notifications_->Get(id, notification);
}

void AdsImpl::OnForeground() {
  is_foreground_ = true;
  GenerateAdReportingForegroundEvent();

  if (IsMobile() && !ads_client_->CanShowBackgroundNotifications()) {
    StartDeliveringNotifications();
  }
}

void AdsImpl::OnBackground() {
  is_foreground_ = false;
  GenerateAdReportingBackgroundEvent();

  if (IsMobile() && !ads_client_->CanShowBackgroundNotifications()) {
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

  if (!IsInitialized()) {
    BLOG(WARNING) << "OnUnIdle failed as not initialized";
    return;
  }

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
    const std::string& html) {
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

  if (DomainsMatch(url, last_shown_notification_info_.url)) {
    BLOG(INFO) << "Site visited " << url
        << ", domain matches the last shown ad notification for "
            << last_shown_notification_info_.url;

    const std::string domain = GetDomain(url);
    if (last_sustained_ad_domain_ != domain) {
      last_sustained_ad_domain_ = domain;

      StartSustainingAdInteraction(kSustainAdInteractionAfterSeconds);
    } else {
      BLOG(INFO) << "Already sustaining ad interaction for " << url;
    }

    return;
  }

  if (!last_shown_notification_info_.url.empty()) {
    BLOG(INFO) << "Site visited " << url
      << ", domain does not match the last shown ad notification for "
          << last_shown_notification_info_.url;
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
    const std::vector<AdConversionTrackingInfo>& ad_conversions) {
  for (const auto& ad_conversion : ad_conversions) {
    if (!helper::Uri::MatchWildcard(url, ad_conversion.url_pattern)) {
      continue;
    }

    ConfirmationType confirmation_type;
    if (ad_conversion.type == "postview") {
      confirmation_type = ConfirmationType::VIEW;
    } else if (ad_conversion.type == "postclick") {
      confirmation_type = ConfirmationType::CLICK;
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

      ad_conversions_->Add(ad.ad_content.creative_set_id, ad.ad_content.uuid);
    }
  }
}

void AdsImpl::MaybeClassifyPage(
    const std::string& url,
    const std::string& html) {
  if (!ShouldClassifyPagesIfTargeted()) {
    MaybeGenerateAdReportingLoadEvent(url, kUntargetedPageClassification);
    return;
  }

  auto classification = ClassifyPage(url, html);
  MaybeGenerateAdReportingLoadEvent(url, classification);
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

void AdsImpl::TestShoppingData(
    const std::string& url) {
  if (!IsInitialized()) {
    BLOG(WARNING) << "TestShoppingData failed as not initialized";
    return;
  }

  if (DomainsMatch(url, kShoppingStateUrl)) {
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
  ShowAd(ad);
}

void AdsImpl::CheckEasterEgg(
    const std::string& url) {
  if (!_is_testing) {
    return;
  }

  auto now_in_seconds = Time::NowInSeconds();

  if (DomainsMatch(url, kEasterEggUrl) &&
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
  if (!IsInitialized()) {
    FailedToServeAd("Not initialized");
    return;
  }

  if (!bundle_->IsReady()) {
    FailedToServeAd("Bundle not ready");
    return;
  }

  if (!forced) {
    if (!is_confirmations_ready_) {
      FailedToServeAd("Confirmations not ready");
      return;
    }

    if (!IsAndroid() && !IsForeground()) {
      FailedToServeAd("Not in foreground");
      return;
    }

    if (IsMediaPlaying()) {
      FailedToServeAd("Media playing in browser");
      return;
    }

    if (ShouldNotDisturb()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'do not disturb while not in
      // foreground' }

      FailedToServeAd("Should not disturb");
      return;
    }

    if (!IsAllowedToServeAds()) {
      FailedToServeAd("Not allowed based on history");
      return;
    }
  }

  auto categories = GetWinningCategories();
  ServeAdFromCategories(categories);
}

void AdsImpl::ServeAdFromCategories(
    const std::vector<std::string>& categories) {
  std::string catalog_id = bundle_->GetCatalogId();
  if (catalog_id.empty()) {
    FailedToServeAd("No ad catalog");
    return;
  }

  if (categories.empty()) {
    BLOG(INFO) << "No categories";
    ServeUntargetedAd();
    return;
  }

  BLOG(INFO) << "Serving ad from categories:";
  for (const auto& category : categories) {
    BLOG(INFO) << "  " << category;
  }

  auto callback =
      std::bind(&AdsImpl::OnServeAdFromCategories, this, _1, _2, _3);
  ads_client_->GetAds(categories, callback);
}

void AdsImpl::OnServeAdFromCategories(
    const Result result,
    const std::vector<std::string>& categories,
    const std::vector<AdInfo>& ads) {
  auto eligible_ads = GetEligibleAds(ads);
  if (!eligible_ads.empty()) {
    BLOG(INFO) << "Found " << eligible_ads.size() << " eligible ads";
    ServeAd(eligible_ads);
    return;
  }

  BLOG(INFO) << "No eligible ads found in categories:";
  for (const auto& category : categories) {
    BLOG(INFO) << "  " << category;
  }

  if (ServeAdFromParentCategories(categories)) {
    return;
  }

  ServeUntargetedAd();
}

bool AdsImpl::ServeAdFromParentCategories(
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

  auto callback =
      std::bind(&AdsImpl::OnServeAdFromCategories, this, _1, _2, _3);
  ads_client_->GetAds(parent_categories, callback);

  return true;
}

void AdsImpl::ServeUntargetedAd() {
  BLOG(INFO) << "Serving ad from untargeted category";

  std::vector<std::string> categories = {
    kUntargetedPageClassification
  };

  auto callback = std::bind(&AdsImpl::OnServeUntargetedAd, this, _1, _2, _3);
  ads_client_->GetAds(categories, callback);
}

void AdsImpl::OnServeUntargetedAd(
    const Result result,
    const std::vector<std::string>& categories,
    const std::vector<AdInfo>& ads) {
  auto eligible_ads = GetEligibleAds(ads);
  if (eligible_ads.empty()) {
    FailedToServeAd("No eligible ads found");
    return;
  }

  BLOG(INFO) << "Found " << eligible_ads.size() << " eligible ads";
  ServeAd(eligible_ads);
}

void AdsImpl::ServeAd(
    const std::vector<AdInfo>& ads) {
  auto rand = base::RandInt(0, ads.size() - 1);
  auto ad = ads.at(rand);
  ShowAd(ad);

  SuccessfullyServedAd();
}

void AdsImpl::SuccessfullyServedAd() {
  if (IsMobile()) {
    StartDeliveringNotificationsAfterSeconds(
        base::Time::kSecondsPerHour / ads_client_->GetAdsPerHour());
  }
}

void AdsImpl::FailedToServeAd(
    const std::string& reason) {
  BLOG(INFO) << "Notification not made: " << reason;

  if (IsMobile()) {
    StartDeliveringNotificationsAfterSeconds(
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

std::vector<AdInfo> AdsImpl::GetEligibleAds(
    const std::vector<AdInfo>& ads) {
  std::vector<AdInfo> eligible_ads = {};

  auto unseen_ads = GetUnseenAdsAndRoundRobinIfNeeded(ads);

  std::vector<std::unique_ptr<ExclusionRule>> exclusion_rules =
      CreateExclusionRules();

  for (const auto& ad : unseen_ads) {
    bool should_exclude = false;
    for (std::unique_ptr<ExclusionRule>& exclusion_rule : exclusion_rules) {
      if (exclusion_rule->ShouldExclude(ad)) {
        BLOG(INFO) << exclusion_rule->GetLastMessage();
        should_exclude = true;
      }
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

std::vector<AdInfo> AdsImpl::GetUnseenAdsAndRoundRobinIfNeeded(
    const std::vector<AdInfo>& ads) const {
  if (ads.empty()) {
    return ads;
  }

  std::vector<AdInfo> ads_for_unseen_advertisers =
      GetAdsForUnseenAdvertisers(ads);
  if (ads_for_unseen_advertisers.empty()) {
    BLOG(INFO) << "All advertisers have been shown, so round robin";

    const bool should_not_show_last_advertiser =
        client_->GetAdvertisersUUIDSeen().size() > 1 ? true : false;

    client_->ResetAdvertisersUUIDSeen(ads);

    ads_for_unseen_advertisers = GetAdsForUnseenAdvertisers(ads);

    if (should_not_show_last_advertiser) {
      const auto it = std::remove_if(ads_for_unseen_advertisers.begin(),
          ads_for_unseen_advertisers.end(), [&](AdInfo& ad) {
        return ad.advertiser_id == last_shown_ad_info_.advertiser_id;
      });

      ads_for_unseen_advertisers.erase(it, ads_for_unseen_advertisers.end());
    }
  }

  std::vector<AdInfo> unseen_ads = GetUnseenAds(ads_for_unseen_advertisers);
  if (unseen_ads.empty()) {
    BLOG(INFO) << "All ads have been shown, so round robin";

    const bool should_not_show_last_ad =
        client_->GetAdsUUIDSeen().size() > 1 ? true : false;

    client_->ResetAdsUUIDSeen(ads);

    unseen_ads = GetUnseenAds(ads);

    if (should_not_show_last_ad) {
      const auto it = std::remove_if(ads_for_unseen_advertisers.begin(),
          ads_for_unseen_advertisers.end(), [&](AdInfo& ad) {
        return ad.uuid == last_shown_ad_info_.uuid;
      });

      ads_for_unseen_advertisers.erase(it, ads_for_unseen_advertisers.end());
    }
  }

  return unseen_ads;
}

std::vector<AdInfo> AdsImpl::GetUnseenAds(
    const std::vector<AdInfo>& ads) const {
  auto unseen_ads = ads;
  const auto seen_ads = client_->GetAdsUUIDSeen();
  const auto seen_advertisers = client_->GetAdvertisersUUIDSeen();

  const auto it = std::remove_if(unseen_ads.begin(), unseen_ads.end(),
      [&](AdInfo& ad) {
    return seen_ads.find(ad.uuid) != seen_ads.end() &&
        seen_ads.find(ad.advertiser_id) != seen_advertisers.end();
  });

  unseen_ads.erase(it, unseen_ads.end());

  return unseen_ads;
}

std::vector<AdInfo> AdsImpl::GetAdsForUnseenAdvertisers(
    const std::vector<AdInfo>& ads) const {
  auto unseen_ads = ads;
  const auto seen_ads = client_->GetAdvertisersUUIDSeen();

  const auto it = std::remove_if(unseen_ads.begin(), unseen_ads.end(),
      [&](AdInfo& ad) {
    return seen_ads.find(ad.advertiser_id) != seen_ads.end();
  });

  unseen_ads.erase(it, unseen_ads.end());

  return unseen_ads;
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
    const AdInfo& ad) {
  if (!IsAdValid(ad)) {
    return false;
  }

  auto now_in_seconds = Time::NowInSeconds();

  client_->AppendTimestampToCreativeSetHistoryForUuid(ad.creative_set_id,
      now_in_seconds);
  client_->AppendTimestampToCampaignHistoryForUuid(ad.campaign_id,
      now_in_seconds);

  client_->UpdateAdsUUIDSeen(ad.uuid, 1);
  client_->UpdateAdvertisersUUIDSeen(ad.advertiser_id, 1);

  last_shown_ad_info_ = ad;

  auto notification_info = std::make_unique<NotificationInfo>();
  notification_info->id = base::GenerateGUID();
  notification_info->parent_id = base::GenerateGUID();
  notification_info->advertiser = ad.advertiser;
  notification_info->category = ad.category;
  notification_info->text = ad.notification_text;
  notification_info->url = helper::Uri::GetUri(ad.notification_url);
  notification_info->creative_set_id = ad.creative_set_id;
  notification_info->uuid = ad.uuid;

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Notification shown', {category, winnerOverTime, arbitraryKey,
  // notificationUrl, notificationText, advertiser, uuid, hierarchy}

  BLOG(INFO) << "Ad notification shown:"
      << std::endl << "  id: " << notification_info->id
      << std::endl << "  campaign_id: " << ad.campaign_id
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

bool AdsImpl::IsAllowedToServeAds() {
  std::vector<std::unique_ptr<PermissionRule>> permission_rules =
      CreatePermissionRules();

  bool is_allowed = true;

  for (std::unique_ptr<PermissionRule>& permission_rule : permission_rules) {
    if (!permission_rule->IsAllowed()) {
      BLOG(INFO) << permission_rule->GetLastMessage();
      is_allowed = false;
    }
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

void AdsImpl::StartDeliveringNotifications() {
  StopDeliveringNotifications();

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

void AdsImpl::StartDeliveringNotificationsAfterSeconds(
    const uint64_t seconds) {
  auto timestamp_in_seconds = Time::NowInSeconds() + seconds;
  client_->SetNextCheckServeAdTimestampInSeconds(timestamp_in_seconds);

  StartDeliveringNotifications();
}

void AdsImpl::DeliverNotification() {
  NotificationAllowedCheck(true);
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

    FailedToServeAd("Notifications not allowed");
    return;
  }

  if (!ads_client_->IsNetworkConnectionAvailable()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'network connection not available' }

    FailedToServeAd("Network connection not available");
    return;
  }

  if (IsCatalogOlderThanOneDay()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'catalog older than one day' }

    FailedToServeAd("Catalog older than one day");
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
    BLOG(INFO) << "Failed to sustain ad interaction, domain for the focused "
        << "tab does not match the last shown ad notification for "
            << last_shown_notification_info_.url;
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
  return DomainsMatch(active_tab_url_, last_shown_notification_info_.url);
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
  } else if (ad_conversions_->OnTimer(timer_id)) {
    return;
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
  auto ad_history = std::make_unique<AdHistory>();
  ad_history->timestamp_in_seconds = Time::NowInSeconds();
  ad_history->uuid = base::GenerateGUID();
  ad_history->parent_uuid = notification_info.parent_id;
  ad_history->ad_content.uuid = notification_info.uuid;
  ad_history->ad_content.creative_set_id = notification_info.creative_set_id;
  ad_history->ad_content.brand = notification_info.advertiser;
  ad_history->ad_content.brand_info = notification_info.text;
  ad_history->ad_content.brand_display_url =
      GetDisplayUrl(notification_info.url);
  ad_history->ad_content.brand_url = notification_info.url;
  ad_history->ad_content.ad_action = confirmation_type;
  ad_history->category_content.category = notification_info.category;

  client_->AppendAdHistoryToAdsShownHistory(*ad_history);
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

bool AdsImpl::DomainsMatch(
    const std::string& url_1,
    const std::string& url_2) const {
  return GURL(url_1).DomainIs(GURL(url_2).host_piece());
}

std::string AdsImpl::GetDomain(
    const std::string& url) const {
  auto host_piece = GURL(url).host_piece();

  std::string domain;
  host_piece.CopyToString(&domain);

  return domain;
}

}  // namespace ads
