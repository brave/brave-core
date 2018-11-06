/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "ads_impl.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/notification_info.h"
#include "logging.h"
#include "search_providers.h"
#include "math_helper.h"
#include "string_helper.h"
#include "time_helper.h"
#include "bat/ads/url_components.h"
#include "static_values.h"

using namespace std::placeholders;

namespace ads {

AdsImpl::AdsImpl(AdsClient* ads_client) :
    initialized_(false),
    boot_(false),
    app_focused_(false),
    last_page_classification_(""),
    collect_activity_timer_id_(0),
    media_playing_({}),
    next_easter_egg_(0),
    ads_client_(ads_client),
    settings_(std::make_unique<Settings>()),
    client_(std::make_unique<Client>(this, ads_client_)),
    bundle_(std::make_unique<Bundle>()),
    ads_serve_(std::make_unique<AdsServe>(this, ads_client_, bundle_.get())) {
}

AdsImpl::~AdsImpl() = default;

void AdsImpl::GenerateAdReportingNotificationShownEvent(
    const NotificationShownInfo& info) {
  if (!boot_) {
    boot_ = true;

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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("notificationType");
  writer.String("generated");

  writer.String("notificationClassification");
  writer.StartArray();
  std::vector<std::string> classifications;
  helper::String::Split(info.classification,
    '-', classifications);
  for (const auto& classification : classifications) {
    writer.String(classification.c_str());
  }
  writer.EndArray();

  writer.String("notificationCatalog");
  if (info.catalog.empty()) {
    writer.String("sample-catalog");
  } else {
    writer.String(info.catalog.c_str());
  }

  writer.String("notificationUrl");
  writer.String(info.url.c_str());

  writer.EndObject();

  auto json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingNotificationResultEvent(
    const NotificationResultInfo& info) {
  if (!boot_) {
    boot_ = true;

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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("notificationType");
  switch (info.result_type) {
    case NotificationResultInfoResultType::CLICKED: {
      writer.String("clicked");
      client_->UpdateAdsUUIDSeen(info.id, 1);
      break;
    }

    case NotificationResultInfoResultType::DISMISSED: {
      writer.String("dismissed");
      client_->UpdateAdsUUIDSeen(info.id, 1);
      break;
    }

    case NotificationResultInfoResultType::TIMEOUT: {
      writer.String("timeout");
      break;
    }
  }

  writer.String("notificationClassification");
  writer.StartArray();
  std::vector<std::string> classifications;
  helper::String::Split(info.classification,
    '-', classifications);
  for (const auto& classification : classifications) {
    writer.String(classification.c_str());
  }
  writer.EndArray();

  writer.String("notificationCatalog");
  if (info.catalog.empty()) {
    writer.String("sample-catalog");
  } else {
    writer.String(info.catalog.c_str());
  }

  writer.String("notificationUrl");
  writer.String(info.url.c_str());

  writer.EndObject();

  auto json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::GenerateAdReportingSustainEvent(
    const SustainInfo& info) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("sustain");

  writer.String("stamp");
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("notificationId");
  writer.String(info.notification_id.c_str());

  writer.String("notificationType");
  writer.String("viewed");

  writer.EndObject();

  auto json = buffer.GetString();
  ads_client_->EventLog(json);
}

void AdsImpl::Initialize() {
  if (initialized_) {
    LOG(ads_client_, LogLevel::WARNING) << "Already initialized";
    return;
  }

  ads_client_->Load("settings.json",
      std::bind(&AdsImpl::OnSettingsLoaded, this, _1, _2));
}

void AdsImpl::InitializeUserModel(const std::string& json) {
  user_model_.reset(usermodel::UserModel::CreateInstance());
  user_model_->initializePageClassifier(json);
}

void AdsImpl::AppFocused(const bool is_focused) {
  app_focused_ = is_focused;

  if (app_focused_) {
    GenerateAdReportingForegroundEvent();
  } else {
    GenerateAdReportingBackgroundEvent();
  }
}

void AdsImpl::TabUpdated(
    const std::string& tab_id,
      const std::string& url,
    const bool is_active,
    const bool is_incognito) {
  if (is_incognito) {
    return;
  }

  client_->UpdateLastUserActivity();

  LoadInfo load_info;
  load_info.tab_id = tab_id;
  load_info.tab_url = url;
  GenerateAdReportingLoadEvent(load_info);

  if (!is_active) {
    BlurInfo blur_info;
    blur_info.tab_id = tab_id;
    GenerateAdReportingBlurEvent(blur_info);
  }
}

void AdsImpl::TabSwitched(
    const std::string& tab_id,
    const std::string& url,
    const bool is_incognito) {
  if (is_incognito) {
    return;
  }

  TabUpdated(tab_id, url, true, is_incognito);
  TestShoppingData(url);
  TestSearchState(url);

  FocusInfo focus_info;
  focus_info.tab_id = tab_id;
  GenerateAdReportingFocusEvent(focus_info);
}

void AdsImpl::TabClosed(const std::string& tab_id) {
  DestroyInfo destroy_info;
  destroy_info.tab_id = tab_id;
  GenerateAdReportingDestroyEvent(destroy_info);
}

void AdsImpl::RecordUnIdle() {
  client_->UpdateLastUserIdleStopTime();
}

void AdsImpl::RemoveAllHistory() {
  auto locales = client_->GetLocales();

  client_->RemoveAllHistory();

  ProcessLocales(locales);

  ConfirmAdUUIDIfAdEnabled();
}

void AdsImpl::SaveCachedInfo() {
  if (!settings_->IsAdsEnabled()) {
    client_->RemoveAllHistory();
  }

  // TODO(Brian Johnson): The following should be abstracted back to the client_
  // class
  ads_client_->Save("client.json", client_->ToJson(),
    std::bind(&AdsImpl::OnClientSaved, this, _1));

  // TODO(Brian Johnson): The following change breaks the code as the bundle
  // should not be saved as json and should be passed as a data structure and
  // stored in a database so that fetching of results is optimized for both
  // memory consumption and performance
  ads_client_->Save("bundle.json", bundle_->ToJson(),
    std::bind(&AdsImpl::OnBundleSaved, this, _1));
}

void AdsImpl::RecordMediaPlaying(
    const std::string& tab_id,
    const bool is_playing) {
  auto tab = media_playing_.find(tab_id);

  if (is_playing) {
    if (tab == media_playing_.end()) {
      media_playing_.insert({tab_id, is_playing});
    }
  } else {
    if (tab != media_playing_.end()) {
      media_playing_.erase(tab_id);
    }
  }
}

void AdsImpl::ClassifyPage(const std::string& url, const std::string& html) {
  if (!IsInitialized()) {
    return;
  }

  TestShoppingData(url);
  TestSearchState(url);

  auto page_score = user_model_->classifyPage(html);
  client_->AppendPageScoreToPageScoreHistory(page_score);

  last_page_classification_ = GetWinningCategory(page_score);

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Site visited', { url, immediateWinner, winnerOverTime }
}

void AdsImpl::ChangeLocale(const std::string& locale) {
  if (!IsInitialized()) {
    return;
  }

  auto closest_match_for_locale = ads_client_->SetLocale(locale);
  client_->SetLocale(closest_match_for_locale);

  LoadUserModel();
}

void AdsImpl::CheckReadyAdServe(const bool forced) {
  if (!IsInitialized()) {
    return;
  }

  if (!forced) {
    if (!app_focused_) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'not in foreground' }
      return;
    }

    if (IsMediaPlaying()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'media playing in browser' }
      return;
    }

    if (!IsAllowedToShowAds()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'not allowed based on history' }
      return;
    }
  }

  auto category = GetWinnerOverTimeCategory();
  ServeAdFromCategory(category);
}

void AdsImpl::ServeSampleAd() {
  if (!IsInitialized()) {
    return;
  }

  ads_client_->GetSampleCategory(this);
}

void AdsImpl::SetNotificationsAvailable(const bool available) {
  client_->SetAvailable(available);
}

void AdsImpl::SetNotificationsAllowed(const bool allowed) {
  client_->SetAllowed(allowed);
}

void AdsImpl::SetNotificationsConfigured(const bool configured) {
  client_->SetConfigured(configured);
}

void AdsImpl::SetNotificationsExpired(const bool expired) {
  client_->SetExpired(expired);
}

void AdsImpl::StartCollectingActivity(const uint64_t start_timer_in) {
  StopCollectingActivity();

  collect_activity_timer_id_ = ads_client_->SetTimer(start_timer_in);

  if (collect_activity_timer_id_ == 0) {
    LOG(ads_client_, LogLevel::ERROR) <<
      "Failed to start collect_activity_timer_id_ timer";
  }
}

void AdsImpl::OnTimer(const uint32_t timer_id) {
  if (timer_id == collect_activity_timer_id_) {
    CollectActivity();
  }
}

void AdsImpl::OnSettingsLoaded(
    const Result result,
    const std::string& json) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to load settings: " << json;
    return;
  }

  if (!settings_->FromJson(json)) {
    LOG(ads_client_, LogLevel::WARNING) <<
        "Failed to parse settings JSON: " << json;
    return;
  }

  GenerateAdReportingSettingsEvent();

  if (!settings_->IsAdsEnabled()) {
    Deinitialize();
    return;
  }

  if (initialized_) {
    return;
  }

  ads_client_->Load("client.json",
      std::bind(&AdsImpl::OnClientLoaded, this, _1, _2));
}

void AdsImpl::OnClientSaved(const Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to save client state";
  }
}

void AdsImpl::OnClientLoaded(
    const Result result,
    const std::string& json) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to load client state";
    return;
  }

  if (!client_->FromJson(json)) {
    LOG(ads_client_, LogLevel::WARNING) <<
      "Failed to parse client JSON: " << json;
    return;
  }

  ProcessLocales(ads_client_->GetLocales());

  LoadUserModel();
}

void AdsImpl::OnUserModelLoaded(const Result result, const std::string& json) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to load user model";
    return;
  }

  InitializeUserModel(json);

  if (!initialized_) {
    initialized_ = true;

    RetrieveSSID();

    ConfirmAdUUIDIfAdEnabled();

    ads_serve_->DownloadCatalog();
  }
}

void AdsImpl::OnBundleSaved(const Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to save bundle";
  }
}

void AdsImpl::OnBundleReset(const Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to reset bundle";
  }
}

void AdsImpl::OnBundleLoaded(
    const Result result,
    const std::string& json) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to load bundle: " << json;
  }
}

//////////////////////////////////////////////////////////////////////////////

bool AdsImpl::IsInitialized() {
  if (!initialized_ ||
      !settings_->IsAdsEnabled() ||
      !user_model_->IsInitialized()) {
    return false;
  }

  return true;
}

void AdsImpl::Deinitialize() {
  if (!initialized_) {
    LOG(ads_client_, LogLevel::WARNING) << "Not initialized";
    return;
  }

  StopCollectingActivity();

  ads_serve_->Reset();

  RemoveAllHistory();

  last_page_classification_ = "";

  // TODO(Brian Johnson): ads_client_->ResetCatalog(); was moved from here to
  // ads_serve::Reset however ads serve is only responsible for downloading
  // catalogs, so this should be re-implemented please

  bundle_->Reset();
  // TODO(Brian Johnson): ads_client_->Reset call below should be moved to
  // bundle::Reset (i.e. inside the above bundle_->Reset() function call)
  ads_client_->Reset("bundle.json",
      std::bind(&AdsImpl::OnBundleSaved, this, _1));

  user_model_.reset();

  app_focused_ = false;

  boot_ = false;

  initialized_ = false;
}

void AdsImpl::LoadUserModel() {
  std::stringstream path;
  path << "locales/" << client_->GetLocale() << "/user_model.json";

  ads_client_->Load(path.str(),
    std::bind(&AdsImpl::OnUserModelLoaded, this, _1, _2));
}

std::string AdsImpl::GetWinningCategory(const std::vector<double>& page_score) {
  auto category = user_model_->winningCategory(page_score);
  return category;
}

std::string AdsImpl::GetWinnerOverTimeCategory() {
  auto page_score_history = client_->GetPageScoreHistory();
  if (page_score_history.size() == 0) {
    return "";
  }

  uint64_t count = page_score_history.front().size();

  std::vector<double> winner_over_time_page_scores(count);
  std::fill(winner_over_time_page_scores.begin(),
    winner_over_time_page_scores.end(), 0);

  for (const auto& page_scores : page_score_history) {
    if (page_scores.size() != count) {
      return "";
    }

    for (size_t i = 0; i < page_scores.size(); i++) {
      winner_over_time_page_scores[i] += page_scores[i];
    }
  }

  auto category = user_model_->winningCategory(winner_over_time_page_scores);

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Site visited', { url, immediateWinner, winnerOverTime }
  return category;
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

void AdsImpl::CollectActivity() {
  if (!IsInitialized()) {
    return;
  }

  ads_serve_->DownloadCatalog();
}

bool AdsImpl::IsCollectingActivity() const {
  return collect_activity_timer_id_ != 0 ? true : false;
}

void AdsImpl::StopCollectingActivity() {
  if (!IsCollectingActivity()) {
    return;
  }

  ads_client_->KillTimer(collect_activity_timer_id_);
  collect_activity_timer_id_ = 0;
}

void AdsImpl::ConfirmAdUUIDIfAdEnabled() {
  if (!settings_->IsAdsEnabled()) {
    StopCollectingActivity();
    return;
  }

  client_->UpdateAdUUID();

  StartCollectingActivity(kOneHourInSeconds);
}

void AdsImpl::RetrieveSSID() {
  std::string ssid = ads_client_->GetSSID();
  if (ssid.empty()) {
    ssid = "Unknown";
  }
  client_->SetCurrentSSID(ssid);
}

void AdsImpl::TestShoppingData(const std::string& url) {
  if (!IsInitialized()) {
    return;
  }

  UrlComponents components;
  if (ads_client_->GetUrlComponents(url, &components) &&
      components.hostname == "www.amazon.com") {
      client_->FlagShoppingState(url, 1.0);
  } else {
    client_->UnflagShoppingState();
  }
}

void AdsImpl::TestSearchState(const std::string& url) {
  if (!IsInitialized()) {
    return;
  }

  UrlComponents components;
  if (ads_client_->GetUrlComponents(url, &components) &&
      SearchProviders::IsSearchEngine(components)) {
      client_->FlagSearchState(url, 1.0);
  } else {
    client_->UnflagSearchState(url);
  }
}

bool AdsImpl::IsMediaPlaying() const {
  return media_playing_.empty() ? false : true;
}

void AdsImpl::ProcessLocales(const std::vector<std::string>& locales) {
  if (locales.empty()) {
    return;
  }

  client_->SetLocales(locales);

  auto locale = settings_->GetAdsLocale();
  ads_client_->SetLocale(locale);
}

void AdsImpl::ServeAdFromCategory(const std::string& category) {
  std::string catalog_id = bundle_->GetCatalogId();
  if (catalog_id.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ad catalog' }
    return;
  }

  if (category.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ad (or permitted ad) for
    // winnerOverTime', category, winnerOverTime, arbitraryKey }
    return;
  }

  ads_client_->GetCategory(category, this);
}

void AdsImpl::OnGetSampleCategory(
    const Result result,
    const std::string& category) {
  if (result == Result::FAILED || category.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ads for category', category }
    return;
  }

  ServeAdFromCategory(category);
}

void AdsImpl::OnGetCategory(
    const Result result,
    const std::string& category,
    const std::vector<AdInfo>& ads) {
  if (result == Result::FAILED || ads.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ads for category', category }
    return;
  }

  auto ads_unseen = GetUnseenAds(ads);
  if (ads_unseen.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Ad round-robin', { category, adsSeen, adsNotSeen }

    client_->ResetAdsUUIDSeen(ads);

    ads_unseen = GetUnseenAds(ads);
    if (ads_unseen.empty()) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Notification not made', { reason: 'no ads for category', category }
      return;
    }
  }

  auto rand = helper::Math::Random() % ads_unseen.size();
  auto ad = ads_unseen[rand];

  if (ad.advertiser.empty() ||
      ad.notification_text.empty() ||
      ad.notification_url.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'incomplete ad information',
    // category, winnerOverTime, arbitraryKey, notificationUrl,
    // notificationText, advertiser
    return;
  }

  auto notification_info = std::make_unique<NotificationInfo>();
  notification_info->advertiser = ad.advertiser;
  notification_info->category = category;
  notification_info->text = ad.notification_text;
  notification_info->url = ad.notification_url;
  notification_info->uuid = ad.uuid;

  ads_client_->ShowNotification(std::move(notification_info));

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Notification shown', {category, winnerOverTime, arbitraryKey,
  // notificationUrl, notificationText, advertiser, uuid, hierarchy}

  client_->AppendCurrentTimeToAdsShownHistory();
}

std::vector<AdInfo> AdsImpl::GetUnseenAds(
    const std::vector<AdInfo>& ads) {
  auto ads_unseen = ads;
  auto ads_seen = client_->GetAdsUUIDSeen();

  auto iterator = std::remove_if(
    ads_unseen.begin(),
    ads_unseen.end(),
    [&](AdInfo &info) {
      return ads_seen.find(info.uuid) != ads_seen.end();
    });

  ads_unseen.erase(iterator, ads_unseen.end());

  return ads_unseen;
}

bool AdsImpl::IsAllowedToShowAds() {
  std::deque<std::time_t> ads_shown_history = client_->GetAdsShownHistory();

  auto hour_window = kOneHourInSeconds;
  auto hour_allowed = settings_->GetAdsPerHour();
  auto respects_hour_limit = AdsShownHistoryRespectsRollingTimeConstraint(
    ads_shown_history, hour_window, hour_allowed);

  auto day_window = kOneHourInSeconds;
  auto day_allowed = settings_->GetAdsPerDay();
  auto respects_day_limit = AdsShownHistoryRespectsRollingTimeConstraint(
    ads_shown_history, day_window, day_allowed);

  auto minimum_wait_time = hour_window / hour_allowed;
  bool respects_minimum_wait_time =
    AdsShownHistoryRespectsRollingTimeConstraint(
    ads_shown_history, minimum_wait_time, 0);

  return respects_hour_limit &&
    respects_day_limit &&
    respects_minimum_wait_time;
}

bool AdsImpl::AdsShownHistoryRespectsRollingTimeConstraint(
    const std::deque<time_t> history,
    const uint64_t seconds_window,
    const uint64_t allowable_ad_count) const {
  uint64_t recent_count = 0;
  auto now = static_cast<uint64_t>(std::time(nullptr));

  for (auto i : history) {
    auto time_of_ad = i;

    if (now - time_of_ad < seconds_window) {
      recent_count++;
    }
  }

  if (recent_count <= allowable_ad_count) {
    return true;
  }

  return false;
}

void AdsImpl::GenerateAdReportingLoadEvent(
    const LoadInfo info) {
  UrlComponents components;

  if (ads_client_->GetUrlComponents(info.tab_url, &components) ||
      (components.scheme != "http" && components.scheme != "https")) {
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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("tabId");
  writer.String(info.tab_id.c_str());

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
  std::vector<std::string> classifications;
  helper::String::Split(last_page_classification_, '-', classifications);
  for (const auto& classification : classifications) {
    writer.String(classification.c_str());
  }
  writer.EndArray();

  // TODO(Terry Mancey): cachedValue.url from JavaScript
  auto cached_page_score = page_score_cache_.find(info.tab_url);
  if (cached_page_score != page_score_cache_.end()) {
    writer.String("pageScore");
    writer.StartArray();
    for (const auto& score : cached_page_score->second) {
      writer.Double(score);
    }
    writer.EndArray();
  }

  writer.EndObject();

  auto json = buffer.GetString();
  ads_client_->EventLog(json);

  auto now = static_cast<uint64_t>(std::time(nullptr));
  if (_is_testing && info.tab_url == "https://www.iab.com/"
      && next_easter_egg_ < now) {
    next_easter_egg_ = now + (30 * 1000);

    CheckReadyAdServe(true);
  }
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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("place");
  auto place = client_->GetCurrentPlace();
  writer.String(place.c_str());

  writer.EndObject();

  auto json = buffer.GetString();
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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("place");
  auto place = client_->GetCurrentPlace();
  writer.String(place.c_str());

  writer.EndObject();

  auto json = buffer.GetString();
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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("tabId");
  writer.String(info.tab_id.c_str());

  writer.EndObject();

  auto json = buffer.GetString();
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
  writer.String("focus");

  writer.String("stamp");
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("tabId");
  writer.String(info.tab_id.c_str());

  writer.EndObject();

  auto json = buffer.GetString();
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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("tabId");
  writer.String(info.tab_id.c_str());

  writer.EndObject();

  auto json = buffer.GetString();
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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("place");
  auto place = client_->GetCurrentPlace();
  writer.String(place.c_str());

  writer.EndObject();

  auto json = buffer.GetString();
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
  std::string time_stamp;
  Time::TimeStamp(time_stamp);
  writer.String(time_stamp.c_str());

  writer.String("settings");
  writer.StartObject();

  writer.String("notifications");
  writer.StartObject();

  writer.String("configured");
  auto configured = client_->GetConfigured();
  writer.Bool(configured);

  writer.String("allowed");
  auto allowed = client_->GetAllowed();
  writer.Bool(allowed);

  writer.EndObject();

  // TODO(Terry Mancey): Confirm logic as GetCurrentPlace() does not seem right
  writer.String("place");
  auto place = client_->GetCurrentPlace();
  writer.String(place.c_str());

  writer.String("locale");
  auto locale = settings_->GetAdsLocale();
  writer.String(locale.c_str());

  writer.String("adsPerDay");
  auto ads_per_day = settings_->GetAdsPerDay();
  writer.Uint64(ads_per_day);

  writer.String("adsPerHour");
  auto ads_per_hour = settings_->GetAdsPerHour();
  writer.Uint64(ads_per_hour);

  writer.EndObject();

  writer.EndObject();

  auto json = buffer.GetString();
  ads_client_->EventLog(json);
}

}  // namespace ads
