/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/ads_impl.h"
#include "../include/ads_client.h"
#include "../include/user_model.h"
#include "../include/settings.h"
#include "../include/catalog.h"
#include "../include/callback_handler.h"
#include "../include/ad_info.h"
#include "../include/platform_helper.h"
#include "../include/static_values.h"

// TODO(Terry Mancey): Implement URL validation
// TODO(Terry Mancey): Profile performance
// TODO(Terry Mancey): Profile memory consumption

namespace rewards_ads {

AdsImpl::AdsImpl(ads::AdsClient* ads_client) :
    ads_client_(ads_client),
    settings_(std::make_shared<state::Settings>(this, ads_client_)),
    user_model_(std::make_unique<state::UserModel>
      (this, ads_client_, settings_)),
    catalog_(std::make_shared<state::Catalog>(this, ads_client_)),
    catalog_ads_serve_(std::make_unique<catalog::AdsServe>
      (this, ads_client_, catalog_)),
    initialized_(false),
    app_focused_(false),
    media_playing_({}),
    collect_activity_timer_id_(0) {
  ads_client_->LoadSettingsState(this);
  ads_client_->LoadUserModelState(this);
}

AdsImpl::~AdsImpl() = default;

void AdsImpl::Initialize() {
  if (!settings_->IsAdsEnabled()) {
    Deinitialize();
    return;
  }

  if (initialized_) {
    return;
  }

  initialized_ = true;

  RetrieveSSID();

  auto locales = user_model_->GetLocalesSync();
  ProcessLocales(locales);

  ConfirmAdUUIDIfAdEnabled();

  catalog_ads_serve_->DownloadCatalog();
}

void AdsImpl::AppFocused(const bool focused) {
  app_focused_ = focused;
}

void AdsImpl::TabUpdate() {
  user_model_->UpdateLastUserActivity();
}

void AdsImpl::RecordUnIdle() {
  user_model_->UpdateLastUserIdleStopTime();
}

void AdsImpl::RemoveAllHistory() {
  auto locales = user_model_->GetLocales();

  user_model_->RemoveAllHistory();

  ProcessLocales(locales);

  ConfirmAdUUIDIfAdEnabled();
}

void AdsImpl::SaveCachedInfo() {
  if (!settings_->IsAdsEnabled()) {
    user_model_->RemoveAllHistory();
  }

  user_model_->SaveState();
}

void AdsImpl::ConfirmAdUUIDIfAdEnabled() {
  if (!settings_->IsAdsEnabled()) {
    StopCollectingActivity();
    return;
  }

  user_model_->UpdateAdUUID();

  StartCollectingActivity(START_TIMER_IN_ONE_HOUR);
}

void AdsImpl::TestShoppingData(const std::string& url) {
  if (!initialized_ || !settings_->IsAdsEnabled()) {
    return;
  }

  user_model_->TestShoppingData(url);
}

void AdsImpl::TestSearchState(const std::string& url) {
  if (!initialized_ || !settings_->IsAdsEnabled()) {
    return;
  }

  user_model_->TestSearchState(url);
}

void AdsImpl::RecordMediaPlaying(const std::string& tabId, const bool active) {
  std::map<std::string, bool>::iterator tab;
  tab = media_playing_.find(tabId);

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

void AdsImpl::ChangeNotificationsAvailable(const bool available) {
  user_model_->SetAvailable(available);
}

void AdsImpl::ChangeNotificationsAllowed(const bool allowed) {
  user_model_->SetAllowed(allowed);
}

void AdsImpl::ClassifyPage(const std::string& page) {
  // TODO(Terry Mancey): Implement ClassifyPage (#22)

  if (!initialized_ || !settings_->IsAdsEnabled()) {
    return;
  }
}

void AdsImpl::ChangeLocale(const std::string& locale) {
  if (!initialized_ || !settings_->IsAdsEnabled()) {
    return;
  }

  if (!user_model_->SetLocaleSync(locale)) {
    return;
  }

  user_model_->SetLocale(locale);
}

void AdsImpl::CollectActivity() {
  // TODO(Terry Mancey): Implement CollectActivity (#24)

  if (!initialized_ || !settings_->IsAdsEnabled()) {
    return;
  }
}

void AdsImpl::ApplyCatalog() {
  if (!initialized_ || !settings_->IsAdsEnabled()) {
    return;
  }

  catalog_->SaveState();
  user_model_->SaveState();
}

void AdsImpl::RetrieveSSID() {
  std::string ssid;
  ads_client_->GetSSID(ssid);

  user_model_->SetCurrentSSID(ssid);
}

void AdsImpl::CheckReadyAdServe(const bool forced) {
  if (!initialized_ || !settings_->IsAdsEnabled()) {
    return;
  }

  if (!forced) {
    if (!app_focused_) {
      // TODO(Terry Mancey): Implement User Model Log (#44)
      return;
    }

    if (IsMediaPlaying()) {
      // TODO(Terry Mancey): Implement User Model Log (#44)
      return;
    }

    if (!user_model_->IsAllowedToShowAds()) {
      // TODO(Terry Mancey): Implement User Model Log (#44)
      return;
    }
  }

  std::unique_ptr<ads::AdInfo> ad_info = user_model_->ServeAd();
  ads_client_->ShowAd(std::move(ad_info));
}

void AdsImpl::ServeSampleAd() {
  if (!initialized_ || !settings_->IsAdsEnabled()) {
    return;
  }

  std::unique_ptr<ads::AdInfo> ad_info = user_model_->ServeSampleAd();
  ads_client_->ShowAd(std::move(ad_info));
}

//////////////////////////////////////////////////////////////////////////////

void AdsImpl::OnTimer(const uint32_t timer_id) {
  if (timer_id == collect_activity_timer_id_) {
    CollectActivity();
  }
}

//////////////////////////////////////////////////////////////////////////////

void AdsImpl::OnSettingsStateLoaded(
    const ads::Result result,
    const std::string& json) {
  if (result != ads::Result::ADS_OK) {
    return;
  }

  settings_->LoadState(json);
}

void AdsImpl::OnUserModelStateSaved(const ads::Result result) {
}

void AdsImpl::OnUserModelStateLoaded(
    const ads::Result result,
    const std::string& json) {
  if (result != ads::Result::ADS_OK) {
    return;
  }

  user_model_->LoadState(json);
}

void AdsImpl::OnCatalogStateSaved(const ads::Result result) {
}

void AdsImpl::OnCatalogStateLoaded(
    const ads::Result result,
    const std::string& json) {
  if (result != ads::Result::ADS_OK) {
    StartCollectingActivity(START_TIMER_IN_ONE_HOUR);
    return;
  }

  ApplyCatalog();

  uint64_t start_time_in = catalog_ads_serve_->NextCatalogCheck();
  StartCollectingActivity(start_time_in);
}

//////////////////////////////////////////////////////////////////////////////

void AdsImpl::Deinitialize() {
  StopCollectingActivity();

  RemoveAllHistory();

  catalog_ads_serve_->ResetNextCatalogCheck();
  catalog_->Reset();

  initialized_ = false;
}

//////////////////////////////////////////////////////////////////////////////

void AdsImpl::StartCollectingActivity(const uint64_t start_timer_in) {
  if (IsCollectingActivity()) {
    return;
  }

  ads_client_->SetTimer(start_timer_in, collect_activity_timer_id_);

  if (collect_activity_timer_id_ == 0) {
    LOG(ERROR) << "Failed to start collect_activity_timer_id_ timer"
      << std::endl;
  }
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

//////////////////////////////////////////////////////////////////////////////

bool AdsImpl::IsMediaPlaying() const {
  return media_playing_.empty() ? false : true;
}

//////////////////////////////////////////////////////////////////////////////

void AdsImpl::ProcessLocales(const std::vector<std::string>& locales) {
  if (locales.size() == 0) {
    return;
  }

  user_model_->SetLocales(locales);

  std::string locale = settings_->GetAdsLocale();
  if (std::find(locales.begin(), locales.end(), locale) == locales.end()) {
    locale = locales.front();
  }

  user_model_->SetLocaleSync(locale);
}

}  // namespace rewards_ads
