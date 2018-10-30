/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>

#include "ads_impl.h"
#include "ads_client.h"
#include "bundle_category_info.h"
#include "ad_info.h"
#include "search_providers.h"
#include "math_helper.h"
#include "url_components.h"
#include "static_values.h"

namespace rewards_ads {

AdsImpl::AdsImpl(ads::AdsClient* ads_client) :
    initialized_(false),
    app_focused_(false),
    collect_activity_timer_id_(0),
    media_playing_({}),
    ads_client_(ads_client),
    settings_(std::make_unique<state::Settings>(ads_client_)),
    client_(std::make_unique<state::Client>(this, ads_client_)),
    bundle_(std::make_shared<state::Bundle>(ads_client_)),
    catalog_ads_serve_(std::make_unique<catalog::AdsServe>
      (this, ads_client_, bundle_)) {
}

AdsImpl::~AdsImpl() = default;

void AdsImpl::Initialize() {
  if (initialized_) {
    ads_client_->Log(ads::LogLevel::WARNING, "Already initialized");
    return;
  }

  ads_client_->LoadSettings(this);
}

void AdsImpl::InitializeUserModel(const std::string& json) {
  user_model_->initializePageClassifier(json);
}

void AdsImpl::AppFocused(const bool focused) {
  app_focused_ = focused;
}

void AdsImpl::TabUpdate() {
  client_->UpdateLastUserActivity();
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

  client_->SaveJson();
}

void AdsImpl::ConfirmAdUUIDIfAdEnabled() {
  if (!settings_->IsAdsEnabled()) {
    StopCollectingActivity();
    return;
  }

  client_->UpdateAdUUID();

  StartCollectingActivity(rewards_ads::_one_hour_in_seconds);
}

void AdsImpl::TestShoppingData(const std::string& url) {
  if (!IsInitialized()) {
    return;
  }

  ads::UrlComponents components;
  ads_client_->GetUrlComponents(url, components);
  if (components.hostname == "www.amazon.com") {
    client_->FlagShoppingState(url, 1.0);
  } else {
    client_->UnflagShoppingState();
  }
}

void AdsImpl::TestSearchState(const std::string& url) {
  if (!IsInitialized()) {
    return;
  }

  ads::UrlComponents components;
  ads_client_->GetUrlComponents(url, components);
  if (ads::SearchProviders::IsSearchEngine(components)) {
    client_->FlagSearchState(url, 1.0);
  } else {
    client_->UnflagSearchState(url);
  }
}

void AdsImpl::RecordMediaPlaying(const std::string& tabId, const bool active) {
  auto tab = media_playing_.find(tabId);

  if (active) {
    if (tab == media_playing_.end()) {
      media_playing_.insert({tabId, active});
    }
  } else {
    if (tab != media_playing_.end()) {
      media_playing_.erase(tabId);
    }
  }
}

void AdsImpl::ClassifyPage(const std::string& html) {
  if (!IsInitialized()) {
    return;
  }

  auto page_scores = user_model_->classifyPage(html);
  client_->AppendPageScoreToPageScoreHistory(page_scores);
}

void AdsImpl::ChangeLocale(const std::string& locale) {
  if (!IsInitialized()) {
    return;
  }

  auto closest_match_for_locale = ads_client_->SetLocale(locale);
  client_->SetLocale(closest_match_for_locale);

  LoadUserModel();
}

void AdsImpl::CollectActivity() {
  if (!IsInitialized()) {
    return;
  }

  catalog_ads_serve_->DownloadCatalog();
}

void AdsImpl::ApplyCatalog() {
  if (!IsInitialized()) {
    return;
  }

  client_->SaveJson();
  bundle_->Save();
}

void AdsImpl::RetrieveSSID() {
  std::string ssid;
  ads_client_->GetSSID(ssid);

  client_->SetCurrentSSID(ssid);
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

  auto category = GetWinningCategory();
  ServeAdFromCategory(category, this);
}

void AdsImpl::ServeSampleAd() {
  if (!IsInitialized()) {
    return;
  }

  auto category = ads_client_->GetSampleCategory(this);
  if (!category.empty()) {
    ServeAdFromCategory(category, this);
  }
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

  ads_client_->SetTimer(start_timer_in, collect_activity_timer_id_);

  if (collect_activity_timer_id_ == 0) {
    ads_client_->Log(ads::LogLevel::ERROR,
      "Failed to start collect_activity_timer_id_ timer");
  }
}

void AdsImpl::OnTimer(const uint32_t timer_id) {
  if (timer_id == collect_activity_timer_id_) {
    CollectActivity();
  }
}

void AdsImpl::OnSettingsLoaded(
    const ads::Result result,
    const std::string& json) {
  if (result == ads::Result::FAILED) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to load settings state");
    return;
  }

  if (!settings_->LoadJson(json)) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to load settings state");
    return;
  }

  if (!settings_->IsAdsEnabled()) {
    Deinitialize();
    return;
  }

  if (initialized_) {
    return;
  }

  ads_client_->LoadClient(this);
}

void AdsImpl::OnClientSaved(const ads::Result result) {
  if (result == ads::Result::FAILED) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to save client state");
  }
}

void AdsImpl::OnClientLoaded(
    const ads::Result result,
    const std::string& json) {
  if (result == ads::Result::FAILED) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to load client state");
    return;
  }

  if (!client_->LoadJson(json)) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to load client state");
    return;
  }

  std::vector<std::string> locales;
  ads_client_->GetLocales(locales);
  ProcessLocales(locales);

  LoadUserModel();
}

void AdsImpl::OnUserModelLoaded(const ads::Result result) {
  if (result == ads::Result::FAILED) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to load user model");
    return;
  }

  if (!initialized_) {
    initialized_ = true;

    RetrieveSSID();

    ConfirmAdUUIDIfAdEnabled();

    catalog_ads_serve_->DownloadCatalog();
  }
}

void AdsImpl::OnBundleSaved(const ads::Result result) {
  if (result == ads::Result::FAILED) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to save bundle");
  }
}

void AdsImpl::OnBundleLoaded(
    const ads::Result result,
    const std::string& json) {
  if (result == ads::Result::FAILED) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to load bundle");
  }
}

//////////////////////////////////////////////////////////////////////////////

void AdsImpl::Deinitialize() {
  if (!initialized_) {
    ads_client_->Log(ads::LogLevel::WARNING, "Not initialized");
    return;
  }

  StopCollectingActivity();

  catalog_ads_serve_->ResetNextCatalogCheck();

  RemoveAllHistory();

  ads_client_->ResetCatalog();
  bundle_->Reset();

  user_model_.reset();

  initialized_ = false;
}

bool AdsImpl::IsInitialized() {
  if (!initialized_ ||
      !settings_->IsAdsEnabled() ||
      !user_model_->IsInitialized()) {
    return false;
  }

  return true;
}

void AdsImpl::LoadUserModel() {
  user_model_ = std::make_unique<usermodel::UserModel>();
  ads_client_->LoadUserModel(this);
}

std::string AdsImpl::GetWinningCategory() {
  auto page_score_history = client_->GetPageScoreHistory();
  if (page_score_history.size() == 0) {
    return "";
  }

  uint64_t count = page_score_history.front().size();

  std::vector<double> winner_over_time_page_scores(count);
  std::fill(winner_over_time_page_scores.begin(),
    winner_over_time_page_scores.end(), 0);

  for (auto const& page_scores : page_score_history) {
    if (page_scores.size() != count) {
      return "";
    }

    for (int i = 0; i < page_scores.size(); i++) {
      winner_over_time_page_scores[i] += page_scores[i];
    }
  }

  auto category = usermodel::UserModel::winningCategory(
    winner_over_time_page_scores, user_model_->page_classifier.Classes());

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Site visited', { url, immediateWinner, winnerOverTime }
  return category;
}

bool AdsImpl::IsCollectingActivity() const {
  return collect_activity_timer_id_ != 0 ? true : false;
}

void AdsImpl::StopCollectingActivity() {
  if (!IsCollectingActivity()) {
    return;
  }

  ads_client_->StopTimer(collect_activity_timer_id_);
  collect_activity_timer_id_ = 0;
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

void AdsImpl::ServeAdFromCategory(
    const std::string& category,
    CallbackHandler* callback_handler) {
  // TODO(Terry Mancey): Get catalog id from ads_client
  // if (catalog_->GetCatalogId().empty()) {
  //   // TODO(Terry Mancey): Implement Log (#44)
  //   // 'Notification not made', { reason: 'no ad catalog' }
  //   return;
  // }

  if (category.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ad (or permitted ad) for
    // winnerOverTime', category, winnerOverTime, arbitraryKey }
    return;
  }

  ads_client_->GetAds(category, this);
}

void AdsImpl::OnGetAds(
    const ads::Result result,
    const std::string& category,
    const std::vector<bundle::CategoryInfo>& ads) {
  if (result == ads::Result::FAILED || ads.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Notification not made', { reason: 'no ads for category', category }
    return;
  }

  auto ads_unseen = GetUnseenAds(ads);
  if (ads_unseen.empty()) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Ad round-robin', { category, adsSeen, adsNotSeen }

    client_->ResetAdsUUIDSeenForAds(ads);

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

  // TODO(Terry Mancey): UpdatedAdsUUIDSeen should only be called from
  // GenerateAdReportingEvent once implemented (#37)
  client_->UpdateAdsUUIDSeen(ad.uuid, 1);

  auto ad_info = std::make_unique<ads::AdInfo>();
  ad_info->advertiser = ad.advertiser;
  ad_info->category = category;
  ad_info->notification_text = ad.notification_text;
  ad_info->notification_url = ad.notification_url;
  ad_info->uuid = ad.uuid;

  ads_client_->ShowAd(std::move(ad_info));

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Notification shown', {category, winnerOverTime, arbitraryKey,
  // notificationUrl, notificationText, advertiser, uuid, hierarchy}

  client_->AppendCurrentTimeToAdsShownHistory();
}

std::vector<bundle::CategoryInfo> AdsImpl::GetUnseenAds(
    const std::vector<bundle::CategoryInfo>& categories) {
  auto ads_unseen = categories;
  auto ads_seen = client_->GetAdsUUIDSeen();

  auto iterator = std::remove_if(
    ads_unseen.begin(),
    ads_unseen.end(),
    [&](bundle::CategoryInfo &info) {
      return ads_seen.find(info.uuid) != ads_seen.end();
    });

  ads_unseen.erase(iterator, ads_unseen.end());

  return ads_unseen;
}

bool AdsImpl::IsAllowedToShowAds() {
  std::deque<std::time_t> ads_shown_history = client_->GetAdsShownHistory();

  auto hour_window = rewards_ads::_one_hour_in_seconds;
  auto hour_allowed = settings_->GetAdsAmountPerHour();
  auto respects_hour_limit = AdsShownHistoryRespectsRollingTimeConstraint(
    ads_shown_history, hour_window, hour_allowed);

  auto day_window = rewards_ads::_one_hour_in_seconds;
  auto day_allowed = settings_->GetAdsAmountPerDay();
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
  auto recent_count = 0;
  auto now = std::time(nullptr);

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

}  // namespace rewards_ads
