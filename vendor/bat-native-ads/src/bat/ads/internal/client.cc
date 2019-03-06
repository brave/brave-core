/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/time_helper.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/logging.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ads {

Client::Client(AdsImpl* ads, AdsClient* ads_client) :
    is_initialized_(false),
    state_has_loaded_(false),
    ads_(ads),
    ads_client_(ads_client),
    client_state_(new ClientState()) {
}

Client::~Client() = default;

void Client::SaveState() {
  if (!state_has_loaded_) {
    return;
  }

  auto json = client_state_->ToJson();
  auto callback = std::bind(&Client::OnStateSaved, this, _1);
  ads_client_->Save(_client_name, json, callback);
}

void Client::LoadState() {
  auto callback = std::bind(&Client::OnStateLoaded, this, _1, _2);
  ads_client_->Load(_client_name, callback);
}

void Client::AppendCurrentTimeToAdsShownHistory() {
  auto now_in_seconds = helper::Time::NowInSeconds();
  client_state_->ads_shown_history.push_front(now_in_seconds);

  if (client_state_->ads_shown_history.size() >
      kMaximumEntriesInAdsShownHistory) {
    client_state_->ads_shown_history.pop_back();
  }

  SaveState();
}

const std::deque<uint64_t> Client::GetAdsShownHistory() {
  return client_state_->ads_shown_history;
}

void Client::UpdateAdUUID() {
  if (!client_state_->ad_uuid.empty()) {
    return;
  }

  client_state_->ad_uuid = ads_client_->GenerateUUID();

  SaveState();
}

void Client::UpdateAdsUUIDSeen(
    const std::string& uuid,
    const uint64_t value) {
  client_state_->ads_uuid_seen.insert({uuid, value});

  SaveState();
}

const std::map<std::string, uint64_t> Client::GetAdsUUIDSeen() {
  return client_state_->ads_uuid_seen;
}

void Client::ResetAdsUUIDSeen(
    const std::vector<AdInfo>& ads) {
  BLOG(INFO) << "Resetting seen Ads";

  for (const auto& ad : ads) {
    auto ad_uuid_seen = client_state_->ads_uuid_seen.find(ad.uuid);
    if (ad_uuid_seen != client_state_->ads_uuid_seen.end()) {
      client_state_->ads_uuid_seen.erase(ad_uuid_seen);
    }
  }

  SaveState();
}

void Client::SetAvailable(const bool available) {
  client_state_->available = available;

  SaveState();
}

bool Client::GetAvailable() const {
  return client_state_->available;
}

void Client::FlagShoppingState(
    const std::string& url,
    const uint64_t score) {
  client_state_->shop_activity = true;
  client_state_->shop_url = url;
  client_state_->score = score;
  client_state_->last_shop_time = helper::Time::NowInSeconds();

  SaveState();
}

void Client::UnflagShoppingState() {
  client_state_->shop_activity = false;

  SaveState();
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
  client_state_->last_search_time = helper::Time::NowInSeconds();

  SaveState();
}

void Client::UnflagSearchState(const std::string& url) {
  if (client_state_->search_url == url) {
    return;
  }

  client_state_->search_activity = false;
  client_state_->last_search_time = helper::Time::NowInSeconds();

  SaveState();
}

bool Client::GetSearchState() {
  return client_state_->search_activity;
}

void Client::UpdateLastUserActivity() {
  client_state_->last_user_activity = helper::Time::NowInSeconds();

  SaveState();
}

uint64_t Client::GetLastUserActivity() {
  return client_state_->last_user_activity;
}

void Client::UpdateLastUserIdleStopTime() {
  client_state_->last_user_idle_stop_time = helper::Time::NowInSeconds();

  SaveState();
}

void Client::SetLocale(const std::string& locale) {
  client_state_->locale = locale;

  SaveState();
}

const std::string Client::GetLocale() {
  return client_state_->locale;
}

void Client::SetLocales(const std::vector<std::string>& locales) {
  client_state_->locales = locales;

  SaveState();
}

const std::vector<std::string> Client::GetLocales() {
  return client_state_->locales;
}

void Client::SetLastPageClassification(
    const std::string& classification) {
  client_state_->last_page_classification = classification;

  SaveState();
}

const std::string Client::GetLastPageClassification() {
  return client_state_->last_page_classification;
}

void Client::AppendPageScoreToPageScoreHistory(
    const std::vector<double>& page_score) {
  client_state_->page_score_history.push_front(page_score);
  if (client_state_->page_score_history.size() >
      kMaximumEntriesInPageScoreHistory) {
    client_state_->page_score_history.pop_back();
  }

  SaveState();
}

const std::deque<std::vector<double>> Client::GetPageScoreHistory() {
  return client_state_->page_score_history;
}

void Client::AppendCurrentTimeToCreativeSetHistory(
    const std::string& creative_set_id) {
  if (client_state_->creative_set_history.find(creative_set_id) ==
      client_state_->creative_set_history.end()) {
    client_state_->creative_set_history.insert({creative_set_id, {}});
  }

  auto now_in_seconds = helper::Time::NowInSeconds();
  client_state_->creative_set_history.at(
      creative_set_id).push_back(now_in_seconds);

  SaveState();
}

const std::map<std::string, std::deque<uint64_t>>
    Client::GetCreativeSetHistory() const {
  return client_state_->creative_set_history;
}

void Client::AppendCurrentTimeToCampaignHistory(
    const std::string& campaign_id) {
  if (client_state_->campaign_history.find(campaign_id) ==
      client_state_->campaign_history.end()) {
    client_state_->campaign_history.insert({campaign_id, {}});
  }

  auto now_in_seconds = helper::Time::NowInSeconds();
  client_state_->campaign_history.at(campaign_id).push_back(now_in_seconds);

  SaveState();
}

const std::map<std::string, std::deque<uint64_t>>
    Client::GetCampaignHistory() const {
  return client_state_->campaign_history;
}

void Client::RemoveAllHistory() {
  BLOG(INFO) << "Removed all client state history";

  client_state_.reset(new ClientState());

  SaveState();
}

///////////////////////////////////////////////////////////////////////////////

void Client::OnStateSaved(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to save client state";

    return;
  }

  BLOG(INFO) << "Successfully saved client state";
}

void Client::OnStateLoaded(const Result result, const std::string& json) {
  is_initialized_ = true;
  state_has_loaded_ = true;

  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load client state, resetting to default values";

    client_state_.reset(new ClientState());
  } else {
    if (!FromJson(json)) {
      BLOG(ERROR) << "Failed to parse client state: " << json;

      return;
    }

    BLOG(INFO) << "Successfully loaded client state";
  }

  ads_->InitializeStep2();
}

bool Client::FromJson(const std::string& json) {
  ClientState state;
  std::string error_description;
  auto result = LoadFromJson(&state, json, &error_description);
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to parse client JSON (" << error_description <<
        "): " << json;

    return false;
  }

  client_state_.reset(new ClientState(state));

  SaveState();

  return true;
}

}  // namespace ads
