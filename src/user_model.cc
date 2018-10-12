/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ctime>

#include <uriparser/Uri.h>

#include "../include/user_model.h"
#include "../include/ads_impl.h"
#include "../include/user_model_state.h"
#include "../include/settings_state.h"
#include "../include/json_helper.h"
#include "../include/ad_info.h"
#include "../include/ads_client.h"
#include "../include/static_values.h"

namespace state {

UserModel::UserModel(
    rewards_ads::AdsImpl* ads,
    ads::AdsClient* ads_client,
    std::shared_ptr<Settings> settings) :
      ads_(ads),
      ads_client_(ads_client),
      settings_(settings),
      user_model_state_(new USER_MODEL_STATE()) {
}

UserModel::~UserModel() = default;

void UserModel::TestShoppingData(const std::string& url) {
  std::string hostname = GetHostName(url);
  if (hostname.empty()) {
    UnflagShoppingState();
    return;
  }

  if (hostname.compare("www.amazon.com") == 0) {
    FlagShoppingState(url, 1.0);
  } else {
    UnflagShoppingState();
  }
}

void UserModel::FlagShoppingState(
    const std::string& url,
    const uint64_t score) {
  user_model_state_->shop_activity = true;
  user_model_state_->shop_url = url;
  user_model_state_->score = score;

  auto now = std::time(nullptr);
  user_model_state_->last_shop_time = now;
}

void UserModel::UnflagShoppingState() {
  user_model_state_->shop_activity = false;
}

void UserModel::TestSearchState(const std::string& url) {
  std::string hostname = GetHostName(url);
  if (hostname.empty()) {
    UnflagSearchState(url);
    return;
  }

  bool is_a_search = false;

  // TODO(Terry Mancey): Decouple search providers into SearchProviders class
  for (auto& search_provider : search_providers_) {
    std::string search_provider_hostname =
      GetHostName(search_provider.base.c_str());

    if (search_provider_hostname.empty()) {
      LOG(WARNING) "Search provider hostname not found" << std::endl;
      continue;
    }

    bool is_search_engine = search_provider.any_visit_to_base_domain_is_search;

    if (is_search_engine && hostname.compare(search_provider_hostname) == 0) {
      is_a_search = true;
      break;
    }

    size_t index = search_provider.search.find('{');
    std::string substring = search_provider.search.substr(0, index);
    size_t href_index = url.find(substring);

    if (index != std::string::npos && href_index != std::string::npos) {
      is_a_search = true;
      break;
    }
  }

  if (is_a_search) {
    FlagSearchState(url, 1.0);
  } else {
    UnflagSearchState(url);
  }
}

void UserModel::FlagSearchState(
    const std::string& url,
    const uint64_t score) {
  user_model_state_->search_activity = true;
  user_model_state_->search_url = url;
  user_model_state_->score = score;

  auto now = std::time(nullptr);
  user_model_state_->last_search_time = now;
}

void UserModel::UnflagSearchState(const std::string &url) {
  if (user_model_state_->search_url.compare(url) == 0) {
    return;
  }

  user_model_state_->search_activity = false;

  auto now = std::time(nullptr);
  user_model_state_->last_search_time = now;
}

std::string UserModel::GetHostName(const std::string& url) {
  // TODO(Terry Mancey): Decouple into URI class

  UriParserStateA parser_state;

  UriUriA uri;
  parser_state.uri = &uri;
  if (uriParseUriA(&parser_state, url.c_str()) != URI_SUCCESS) {
    uriFreeUriMembersA(&uri);
    return "";
  }

  std::string hostname(uri.hostText.first, uri.hostText.afterLast);

  uriFreeUriMembersA(&uri);

  return hostname;
}

void UserModel::UpdateLastUserActivity() {
  auto now = std::time(nullptr);
  user_model_state_->last_user_activity = now;
}

void UserModel::UpdateLastUserIdleStopTime() {
  auto now = std::time(nullptr);
  user_model_state_->last_user_idle_stop_time = now;
}

void UserModel::SetCurrentSSID(const std::string& ssid) {
  user_model_state_->current_ssid = ssid;
}

void UserModel::SetLocale(const std::string& locale) {
  user_model_state_->locale = locale;
}

void UserModel::SetAvailable(const bool available) {
  user_model_state_->available = available;
}

void UserModel::SetAllowed(const bool allowed) {
  user_model_state_->allowed = allowed;
}

void UserModel::UpdateAdUUID() {
  if (!user_model_state_->ad_uuid.empty()) {
    return;
  }

  ads_client_->GenerateAdUUID(user_model_state_->ad_uuid);
}

void UserModel::UpdateAdsUUIDSeen(
    const std::string& uuid,
    const uint64_t value) {
  user_model_state_->ads_uuid_seen.insert({uuid, value});
}

bool UserModel::IsAllowedToShowAds() {
  std::vector<time_t> ads_shown_history = user_model_state_->ads_shown_history;

  uint64_t hour_window = 60 * 60;
  uint64_t hour_allowed = settings_->GetAdsAmountPerHour();
  bool respects_hour_limit = HistoryRespectsRollingTimeConstraint(
    ads_shown_history, hour_window, hour_allowed);

  uint64_t day_window = 24 * hour_window;
  uint64_t day_allowed = settings_->GetAdsAmountPerDay();
  bool respects_day_limit = HistoryRespectsRollingTimeConstraint(
    ads_shown_history, day_window, day_allowed);

  uint64_t minimum_wait_time = hour_window / hour_allowed;
  bool respects_minimum_wait_time = HistoryRespectsRollingTimeConstraint(
    ads_shown_history, minimum_wait_time, 0);

  return respects_hour_limit &&
    respects_day_limit &&
    respects_minimum_wait_time;
}

bool UserModel::HistoryRespectsRollingTimeConstraint(
    const std::vector<time_t> history,
    const uint64_t seconds_window,
    const uint64_t allowable_ad_count) {
  uint64_t recent_count = 0;
  auto now = std::time(nullptr);

  for (auto i : history) {
    uint64_t time_of_ad = i;

    if (now - time_of_ad < seconds_window) {
      recent_count++;
    }
  }

  return recent_count <= allowable_ad_count ? true : false;
}

std::unique_ptr<ads::AdInfo> UserModel::ServeAd() {
  // TODO(Terry Mancey): Implement ServerAdFromCategory

  auto ad_info = std::make_unique<ads::AdInfo>();

  ad_info->advertiser = "Brave";
  ad_info->category = "Technology & Computing";
  ad_info->notification_text = "On a mission to fix the web";
  ad_info->notification_url = "https://brave.com";
  ad_info->uuid = "8c513c79-fb74-47f0-9f82-5e62c45bf999";

  return ad_info;
}

std::unique_ptr<ads::AdInfo> UserModel::ServeSampleAd() {
  // TODO(Terry Mancey): Implement ServerAdFromCategory

  auto ad_info = std::make_unique<ads::AdInfo>();

  ad_info->advertiser = "Brave";
  ad_info->category = "Technology & Computing";
  ad_info->notification_text = "On a mission to fix the web";
  ad_info->notification_url = "https://brave.com";
  ad_info->uuid = "8c513c79-fb74-47f0-9f82-5e62c45bf999";

  return ad_info;
}

void UserModel::RemoveAllHistory() {
  user_model_state_.reset(new USER_MODEL_STATE());
}

std::vector<std::string> UserModel::GetLocales() {
  return user_model_state_->locales;
}

void UserModel::SetLocales(const std::vector<std::string>& locales) {
  user_model_state_->locales = locales;
}

bool UserModel::SetLocaleSync(const std::string& locale) {
  // TODO(Terry Mancey): Implement

  return false;
}

std::vector<std::string> UserModel::GetLocalesSync() {
  // TODO(Terry Mancey): Implement

  return {};
}

bool UserModel::LoadState(const std::string& json) {
  USER_MODEL_STATE state;
  if (!LoadFromJson(state, json.c_str())) {
    return false;
  }

  user_model_state_.reset(new USER_MODEL_STATE(state));

  return true;
}

void UserModel::SaveState() {
  std::string json;
  SaveToJsonString(*user_model_state_, json);
  ads_client_->SaveUserModelState(json, this);
}

void UserModel::OnUserModelStateSaved(const ads::Result result) {
}

}  // namespace state
