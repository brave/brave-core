/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/purchase_intent_signal_history.h"
#include "bat/ads/internal/filtered_ad.h"
#include "bat/ads/internal/filtered_category.h"
#include "bat/ads/internal/flagged_ad.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/saved_ad.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/time_util.h"

#include "base/guid.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ads {

namespace {

FilteredAdsList::iterator FindFilteredAd(
    const std::string& creative_instance_id,
    FilteredAdsList* filtered_ads) {
  return std::find_if(filtered_ads->begin(), filtered_ads->end(),
      [&creative_instance_id](const FilteredAd& ad) {
    return ad.creative_instance_id == creative_instance_id;
  });
}

FilteredCategoriesList::iterator FindFilteredCategory(
    const std::string& name,
    FilteredCategoriesList* filtered_categories) {
  return std::find_if(filtered_categories->begin(), filtered_categories->end(),
      [&name](const FilteredCategory& category) {
    return category.name == name;
  });
}

}  // namespace

Client::Client(
    AdsImpl* ads)
    : is_initialized_(false),
      ads_(ads),
      client_state_(new ClientState()) {
  (void)ads_;
}

Client::~Client() = default;

FilteredAdsList Client::get_filtered_ads() const {
  return client_state_->ad_prefs.filtered_ads;
}

FilteredCategoriesList Client::get_filtered_categories() const {
  return client_state_->ad_prefs.filtered_categories;
}

FlaggedAdsList Client::get_flagged_ads() const {
  return client_state_->ad_prefs.flagged_ads;
}

void Client::Initialize(
    InitializeCallback callback) {
  callback_ = callback;

  LoadState();
}

void Client::AppendAdHistoryToAdsHistory(
    const AdHistory& ad_history) {
  client_state_->ads_shown_history.push_front(ad_history);

  if (client_state_->ads_shown_history.size() >
      kMaximumEntriesInAdsShownHistory) {
    client_state_->ads_shown_history.pop_back();
  }

  SaveState();
}

const std::deque<AdHistory>& Client::GetAdsHistory() const {
  return client_state_->ads_shown_history;
}

void Client::AppendToPurchaseIntentSignalHistoryForSegment(
    const std::string& segment,
    const PurchaseIntentSignalHistory& history) {
  if (client_state_->purchase_intent_signal_history.find(segment) ==
      client_state_->purchase_intent_signal_history.end()) {
    client_state_->purchase_intent_signal_history.insert({segment, {}});
  }

  client_state_->purchase_intent_signal_history.at(
      segment).push_back(history);

  if (client_state_->purchase_intent_signal_history.at(segment).size() >
      kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory) {
    client_state_->purchase_intent_signal_history.at(segment).pop_back();
  }

  SaveState();
}

const PurchaseIntentSignalSegmentHistoryMap&
    Client::GetPurchaseIntentSignalHistory() const {
  return client_state_->purchase_intent_signal_history;
}

AdContent::LikeAction Client::ToggleAdThumbUp(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContent::LikeAction action) {
  AdContent::LikeAction like_action;
  if (action == AdContent::LikeAction::kThumbsUp) {
    like_action = AdContent::LikeAction::kNone;
  } else {
    like_action = AdContent::LikeAction::kThumbsUp;
  }

  // Remove this ad from the filtered ads list
  auto it_ad = FindFilteredAd(creative_instance_id,
      &client_state_->ad_prefs.filtered_ads);
  if (it_ad != client_state_->ad_prefs.filtered_ads.end()) {
    client_state_->ad_prefs.filtered_ads.erase(it_ad);
  }

  // Update the history detail for ads matching this UUID
  for (auto& item : client_state_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.like_action = like_action;
    }
  }

  SaveState();

  return like_action;
}

AdContent::LikeAction Client::ToggleAdThumbDown(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContent::LikeAction action) {
  AdContent::LikeAction like_action;
  if (action == AdContent::LikeAction::kThumbsDown) {
    like_action = AdContent::LikeAction::kNone;
  } else {
    like_action = AdContent::LikeAction::kThumbsDown;
  }

  // Update this ad in the filtered ads list
  auto it_ad = FindFilteredAd(creative_instance_id,
      &client_state_->ad_prefs.filtered_ads);
  if (like_action == AdContent::LikeAction::kNone) {
    if (it_ad != client_state_->ad_prefs.filtered_ads.end()) {
      client_state_->ad_prefs.filtered_ads.erase(it_ad);
    }
  } else {
    if (it_ad == client_state_->ad_prefs.filtered_ads.end()) {
      FilteredAd filtered_ad;
      filtered_ad.creative_instance_id = creative_instance_id;
      filtered_ad.creative_set_id = creative_set_id;
      client_state_->ad_prefs.filtered_ads.push_back(filtered_ad);
    }
  }

  // Update the history detail for ads matching this UUID
  for (auto& item : client_state_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.like_action = like_action;
    }
  }

  SaveState();

  return like_action;
}

CategoryContent::OptAction Client::ToggleAdOptInAction(
    const std::string& category,
    const CategoryContent::OptAction action) {
  CategoryContent::OptAction opt_action;
  if (action == CategoryContent::OptAction::kOptIn) {
    opt_action = CategoryContent::OptAction::kNone;
  } else {
    opt_action = CategoryContent::OptAction::kOptIn;
  }

  // Remove this category from the filtered categories list
  auto it = FindFilteredCategory(category,
      &client_state_->ad_prefs.filtered_categories);
  if (it != client_state_->ad_prefs.filtered_categories.end()) {
    client_state_->ad_prefs.filtered_categories.erase(it);
  }

  // Update the history for this category
  for (auto& item : client_state_->ads_shown_history) {
    if (item.category_content.category == category) {
      item.category_content.opt_action = opt_action;
    }
  }

  SaveState();

  return opt_action;
}

CategoryContent::OptAction Client::ToggleAdOptOutAction(
    const std::string& category,
    const CategoryContent::OptAction action) {
  CategoryContent::OptAction opt_action;
  if (action == CategoryContent::OptAction::kOptOut) {
    opt_action = CategoryContent::OptAction::kNone;
  } else {
    opt_action = CategoryContent::CategoryContent::OptAction::kOptOut;
  }

  // Update this category in the filtered categories list
  auto it = FindFilteredCategory(category,
      &client_state_->ad_prefs.filtered_categories);
  if (opt_action == CategoryContent::OptAction::kNone) {
    if (it != client_state_->ad_prefs.filtered_categories.end()) {
      client_state_->ad_prefs.filtered_categories.erase(it);
    }
  } else {
    if (it == client_state_->ad_prefs.filtered_categories.end()) {
      FilteredCategory filtered_category;
      filtered_category.name = category;
      client_state_->ad_prefs.filtered_categories.push_back(filtered_category);
    }
  }

  // Update the history for this category
  for (auto& item : client_state_->ads_shown_history) {
    if (item.category_content.category == category) {
      item.category_content.opt_action = opt_action;
    }
  }

  SaveState();

  return opt_action;
}

bool Client::ToggleSaveAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool saved) {
  const bool saved_ad = !saved;

  // Update this ad in the saved ads list
  auto it_ad = std::find_if(client_state_->ad_prefs.saved_ads.begin(),
      client_state_->ad_prefs.saved_ads.end(),
          [&creative_instance_id](const SavedAd& ad) {
    return ad.creative_instance_id == creative_instance_id;
  });

  if (saved_ad) {
    if (it_ad == client_state_->ad_prefs.saved_ads.end()) {
      SavedAd saved_ad;
      saved_ad.creative_instance_id = creative_instance_id;
      saved_ad.creative_set_id = creative_set_id;
      client_state_->ad_prefs.saved_ads.push_back(saved_ad);
    }
  } else {
    if (it_ad != client_state_->ad_prefs.saved_ads.end()) {
      client_state_->ad_prefs.saved_ads.erase(it_ad);
    }
  }

  // Update the history detail for ads matching this UUID
  for (auto& item : client_state_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.saved_ad = saved_ad;
    }
  }

  SaveState();

  return saved_ad;
}

bool Client::ToggleFlagAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool flagged) {
  const bool flagged_ad = !flagged;

  // Update this ad in the flagged ads list
  auto it_ad = std::find_if(client_state_->ad_prefs.flagged_ads.begin(),
      client_state_->ad_prefs.flagged_ads.end(),
          [&creative_instance_id](const FlaggedAd& ad) {
    return ad.creative_instance_id == creative_instance_id;
  });

  if (flagged_ad) {
    if (it_ad == client_state_->ad_prefs.flagged_ads.end()) {
      FlaggedAd flagged_ad;
      flagged_ad.creative_instance_id = creative_instance_id;
      flagged_ad.creative_set_id = creative_set_id;
      client_state_->ad_prefs.flagged_ads.push_back(flagged_ad);
    }
  } else {
    if (it_ad != client_state_->ad_prefs.flagged_ads.end()) {
      client_state_->ad_prefs.flagged_ads.erase(it_ad);
    }
  }

  // Update the history detail for ads matching this UUID
  for (auto& item : client_state_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.flagged_ad = flagged_ad;
    }
  }

  SaveState();

  return flagged_ad;
}

void Client::UpdateAdUUID() {
  if (!client_state_->ad_uuid.empty()) {
    return;
  }

  client_state_->ad_uuid = base::GenerateGUID();

  SaveState();
}

void Client::UpdateSeenAdNotification(
    const std::string& creative_instance_id,
    const uint64_t value) {
  client_state_->seen_ad_notifications.insert({creative_instance_id, value});

  SaveState();
}

const std::map<std::string, uint64_t>& Client::GetSeenAdNotifications() {
  return client_state_->seen_ad_notifications;
}

void Client::ResetSeenAdNotifications(
    const CreativeAdNotificationList& ads) {
  BLOG(1, "Resetting seen ad notifications");

  for (const auto& ad : ads) {
    auto seen_ad_notification =
        client_state_->seen_ad_notifications.find(ad.creative_instance_id);
    if (seen_ad_notification != client_state_->seen_ad_notifications.end()) {
      client_state_->seen_ad_notifications.erase(seen_ad_notification);
    }
  }

  SaveState();
}

void Client::UpdateSeenAdvertiser(
    const std::string& advertiser_id,
    const uint64_t value) {
  client_state_->seen_advertisers.insert({advertiser_id, value});

  SaveState();
}

const std::map<std::string, uint64_t>& Client::GetSeenAdvertisers() {
  return client_state_->seen_advertisers;
}

void Client::ResetSeenAdvertisers(
     const CreativeAdNotificationList& ads) {
  BLOG(1, "Resetting seen advertisers");

  for (const auto& ad : ads) {
    auto seen_advertiser =
        client_state_->seen_advertisers.find(ad.advertiser_id);
    if (seen_advertiser != client_state_->seen_advertisers.end()) {
      client_state_->seen_advertisers.erase(seen_advertiser);
    }
  }

  SaveState();
}

void Client::SetNextCheckServeAdNotificationTimestampInSeconds(
    const uint64_t timestamp_in_seconds) {
  client_state_->next_check_serve_ad_timestamp_in_seconds
      = timestamp_in_seconds;

  SaveState();
}

uint64_t Client::GetNextCheckServeAdNotificationTimestampInSeconds() {
  return client_state_->next_check_serve_ad_timestamp_in_seconds;
}

void Client::SetAvailable(
    const bool available) {
  client_state_->available = available;

  SaveState();
}

bool Client::GetAvailable() const {
  return client_state_->available;
}

void Client::SetUserModelLanguage(
    const std::string& language) {
  client_state_->user_model_language = language;

  SaveState();
}

std::string Client::GetUserModelLanguage() {
  return client_state_->user_model_language;
}

void Client::SetUserModelLanguages(
    const std::vector<std::string>& languages) {
  client_state_->user_model_languages = languages;

  SaveState();
}

const std::vector<std::string>& Client::GetUserModelLanguages() {
  return client_state_->user_model_languages;
}

void Client::AppendPageProbabilitiesToHistory(
    const classification::PageProbabilitiesMap& page_probabilities) {
  client_state_->page_probabilities_history.push_front(page_probabilities);
  if (client_state_->page_probabilities_history.size() >
      kMaximumPageProbabilityHistoryEntries) {
    client_state_->page_probabilities_history.pop_back();
  }

  SaveState();
}

const classification::PageProbabilitiesList&
Client::GetPageProbabilitiesHistory() {
  return client_state_->page_probabilities_history;
}

void Client::AppendTimestampToCreativeSetHistory(
    const std::string& creative_instance_id,
    const uint64_t timestamp_in_seconds) {
  if (client_state_->creative_set_history.find(creative_instance_id) ==
      client_state_->creative_set_history.end()) {
    client_state_->creative_set_history.insert({creative_instance_id, {}});
  }

  client_state_->creative_set_history.at(
      creative_instance_id).push_back(timestamp_in_seconds);

  SaveState();
}

const std::map<std::string, std::deque<uint64_t>>&
Client::GetCreativeSetHistory() const {
  return client_state_->creative_set_history;
}

void Client::AppendTimestampToAdConversionHistory(
    const std::string& creative_set_id,
    const uint64_t timestamp_in_seconds) {
  DCHECK(!creative_set_id.empty());
  if (creative_set_id.empty()) {
    return;
  }

  if (client_state_->ad_conversion_history.find(creative_set_id) ==
      client_state_->ad_conversion_history.end()) {
    client_state_->ad_conversion_history.insert({creative_set_id, {}});
  }

  client_state_->ad_conversion_history.at(
      creative_set_id).push_back(timestamp_in_seconds);

  SaveState();
}

const std::map<std::string, std::deque<uint64_t>>&
Client::GetAdConversionHistory() const {
  return client_state_->ad_conversion_history;
}

void Client::AppendTimestampToCampaignHistory(
    const std::string& creative_instance_id,
    const uint64_t timestamp_in_seconds) {
  if (client_state_->campaign_history.find(creative_instance_id) ==
      client_state_->campaign_history.end()) {
    client_state_->campaign_history.insert({creative_instance_id, {}});
  }

  client_state_->campaign_history.at(
      creative_instance_id).push_back(timestamp_in_seconds);

  SaveState();
}

const std::map<std::string, std::deque<uint64_t>>&
Client::GetCampaignHistory() const {
  return client_state_->campaign_history;
}

void Client::RemoveAllHistory() {
  BLOG(1, "Successfully reset client state");

  client_state_.reset(new ClientState());

  SaveState();
}

std::string Client::GetVersionCode() const {
  return client_state_->version_code;
}

void Client::SetVersionCode(
    const std::string& value) {
  client_state_->version_code = value;

  SaveState();
}

///////////////////////////////////////////////////////////////////////////////

void Client::SaveState() {
  if (!is_initialized_) {
    return;
  }

  BLOG(3, "Saving client state");

  auto json = client_state_->ToJson();
  auto callback = std::bind(&Client::OnStateSaved, this, _1);
  ads_->get_ads_client()->Save(_client_resource_name, json, callback);
}

void Client::OnStateSaved(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to save client state");

    return;
  }

  BLOG(3, "Successfully saved client state");
}

void Client::LoadState() {
  BLOG(3, "Loading client state");

  auto callback = std::bind(&Client::OnStateLoaded, this, _1, _2);
  ads_->get_ads_client()->Load(_client_resource_name, callback);
}

void Client::OnStateLoaded(
    const Result result,
    const std::string& json) {
  is_initialized_ = true;

  if (result != SUCCESS) {
    BLOG(3, "Client state does not exist, creating default state");

    client_state_.reset(new ClientState());
    SaveState();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load client state");

      BLOG(3, "Failed to parse client state: " << json);

      callback_(FAILED);
      return;
    }

    BLOG(3, "Successfully loaded client state");
  }

  callback_(SUCCESS);
}

bool Client::FromJson(
    const std::string& json) {
  ClientState state;
  std::string error_description;
  auto result = LoadFromJson(&state, json, &error_description);
  if (result != SUCCESS) {
    return false;
  }

  client_state_.reset(new ClientState(state));
  SaveState();

  return true;
}

}  // namespace ads
