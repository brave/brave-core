/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "client.h"
#include "static_values.h"

namespace ads {

Client::Client(ads::AdsImpl* ads, ads::AdsClient* ads_client) :
    ads_(ads),
    ads_client_(ads_client),
    client_state_(new CLIENT_STATE()) {
}

Client::~Client() = default;

bool Client::LoadJson(const std::string& json) {
  CLIENT_STATE state;
  if (!LoadFromJson(state, json.c_str())) {
    ads_client_->Log(ads::LogLevel::ERROR, "Failed to parse client json");
    return false;
  }

  client_state_.reset(new CLIENT_STATE(state));

  return true;
}

void Client::SaveJson() {
  std::string json;
  SaveToJson(*client_state_, json);
  ads_client_->SaveClient(json, this);
}

void Client::AppendCurrentTimeToAdsShownHistory() {
  auto now = std::time(nullptr);
  client_state_->ads_shown_history.push_front(now);
  if (client_state_->ads_shown_history.size() >
      kMaximumEntriesInAdsShownHistory) {
    client_state_->ads_shown_history.pop_back();
  }
}

std::deque<std::time_t> Client::GetAdsShownHistory() {
  return client_state_->ads_shown_history;
}

void Client::UpdateAdUUID() {
  if (!client_state_->ad_uuid.empty()) {
    return;
  }

  ads_client_->GenerateAdUUID(client_state_->ad_uuid);
}

void Client::UpdateAdsUUIDSeen(
    const std::string& uuid,
    const uint64_t value) {
  client_state_->ads_uuid_seen.insert({uuid, value});
}

std::map<std::string, uint64_t> Client::GetAdsUUIDSeen() {
  return client_state_->ads_uuid_seen;
}

void Client::ResetAdsUUIDSeenForAds(
    const std::vector<bundle::CategoryInfo>& categories) {
  for (const auto& category : categories) {
    auto ad_uuid_seen = client_state_->ads_uuid_seen.find(category.uuid);
    if (ad_uuid_seen != client_state_->ads_uuid_seen.end()) {
      client_state_->ads_uuid_seen.erase(ad_uuid_seen);
    }
  }
}

void Client::SetAvailable(const bool available) {
  client_state_->available = available;
}

void Client::SetAllowed(const bool allowed) {
  client_state_->allowed = allowed;
}

bool Client::GetAllowed() {
  return client_state_->allowed;
}

void Client::SetConfigured(const bool configured) {
  client_state_->configured = configured;
}

bool Client::GetConfigured() {
  return client_state_->configured;
}

void Client::SetCurrentSSID(const std::string& ssid) {
  client_state_->current_ssid = ssid;
}

void Client::SetExpired(const bool expired) {
  client_state_->expired = expired;
}

void Client::FlagShoppingState(
    const std::string& url,
    const uint64_t score) {
  client_state_->shop_activity = true;
  client_state_->shop_url = url;
  client_state_->score = score;

  auto now = std::time(nullptr);
  client_state_->last_shop_time = now;
}

void Client::UnflagShoppingState() {
  client_state_->shop_activity = false;
}

bool Client::GetShoppingState() {
  return client_state_->shop_activity;
}

void Client::FlagSearchState(
    const std::string& url,
    const uint64_t score) {
  client_state_->search_activity = true;
  client_state_->search_url = url;
  client_state_->score = score;

  auto now = std::time(nullptr);
  client_state_->last_search_time = now;
}

void Client::UnflagSearchState(const std::string &url) {
  if (client_state_->search_url == url) {
    return;
  }

  client_state_->search_activity = false;

  auto now = std::time(nullptr);
  client_state_->last_search_time = now;
}

bool Client::GetSearchState() {
  return client_state_->search_activity;
}

void Client::UpdateLastUserActivity() {
  auto now = std::time(nullptr);
  client_state_->last_user_activity = now;
}

void Client::UpdateLastUserIdleStopTime() {
  auto now = std::time(nullptr);
  client_state_->last_user_idle_stop_time = now;
}

void Client::SetLocale(const std::string& locale) {
  client_state_->locale = locale;
}

std::string Client::GetLocale() {
  return client_state_->locale;
}

void Client::SetLocales(const std::vector<std::string>& locales) {
  client_state_->locales = locales;
}

std::vector<std::string> Client::GetLocales() {
  return client_state_->locales;
}

void Client::AppendPageScoreToPageScoreHistory(
    const std::vector<double>& page_scores) {
  client_state_->page_score_history.push_front(page_scores);
  if (client_state_->page_score_history.size() >
      kMaximumEntriesInPageScoreHistory) {
    client_state_->page_score_history.pop_back();
  }
}

std::deque<std::vector<double>> Client::GetPageScoreHistory() {
  return client_state_->page_score_history;
}

std::string Client::GetCurrentPlace() {
  auto place = client_state_->places.find(client_state_->current_ssid);
  if (place != client_state_->places.end()) {
    return place->second;
  } else {
    return "UNDISCLOSED";
  }
}

void Client::RemoveAllHistory() {
  client_state_.reset(new CLIENT_STATE());
}

//////////////////////////////////////////////////////////////////////////////

void Client::OnClientSaved(const ads::Result result) {
  if (result == ads::Result::FAILED) {
    ads_client_->Log(ads::LogLevel::WARNING, "Failed to save client state");
  }
}

}  // namespace ads
