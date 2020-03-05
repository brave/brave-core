/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/internal/classification_helper.h"
#include "bat/ads/internal/filtered_ad.h"
#include "bat/ads/internal/filtered_category.h"
#include "bat/ads/internal/flagged_ad.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/saved_ad.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/time.h"

#include "base/guid.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

ads::FilteredAdList::iterator FindFilteredAd(
    const std::string& creative_instance_id,
    ads::FilteredAdList* filtered_ad) {
  return std::find_if(filtered_ad->begin(), filtered_ad->end(),
      [&creative_instance_id](const ads::FilteredAd& ad) {
    return ad.creative_instance_id == creative_instance_id;
  });
}

ads::FilteredCategoryList::iterator FindFilteredCategory(
    const std::string& name,
    ads::FilteredCategoryList* filtered_category) {
  return std::find_if(filtered_category->begin(), filtered_category->end(),
      [&name](const ads::FilteredCategory& category) {
    return category.name == name;
  });
}

}  // namespace

namespace ads {

Client::Client(
    AdsImpl* ads,
    AdsClient* ads_client)
    : is_initialized_(false),
      ads_(ads),
      ads_client_(ads_client),
      client_state_(new ClientState()) {
  (void)ads_;
}

Client::~Client() = default;

void Client::Initialize(
    InitializeCallback callback) {
  callback_ = callback;

  LoadState();
}

void Client::AppendAdHistoryToAdsShownHistory(
    const AdHistory& ad_history) {
  client_state_->ads_shown_history.push_front(ad_history);

  if (client_state_->ads_shown_history.size() >
      kMaximumEntriesInAdsShownHistory) {
    client_state_->ads_shown_history.pop_back();
  }

  SaveState();
}

std::deque<AdHistory> Client::GetAdsShownHistory() const {
  return client_state_->ads_shown_history;
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

bool Client::IsFilteredCategory(
    const std::string& category) const {
  // If passed in category has a subcategory and the current filter
  // does not, check if it's a child of the filter. Conversely, if the
  // passed in category has no subcategory but the current filter
  // does, it can't be a match at all so move on to the next
  // filter. Otherwise, perform an exact match to determine whether or
  // not to filter the category.
  std::vector<std::string> category_classifications =
      helper::Classification::GetClassifications(category);
  for (const auto& filtered_category :
       client_state_->ad_prefs.filtered_categories) {
    std::vector<std::string> filtered_classifications =
        helper::Classification::GetClassifications(filtered_category.name);
    if (category_classifications.size() > 1 &&
        filtered_classifications.size() == 1) {
      if (category_classifications[0] == filtered_classifications[0]) {
        return true;
      }
    } else if (category_classifications.size() == 1 &&
               filtered_classifications.size() > 1) {
      continue;
    } else if (filtered_category.name == category) {
      return true;
    }
  }

  return false;
}

bool Client::IsFilteredAd(
    const std::string& creative_set_id) const {
  auto it = std::find_if(client_state_->ad_prefs.filtered_ads.begin(),
      client_state_->ad_prefs.filtered_ads.end(),
          [&creative_set_id](const FilteredAd& ad) {
    return ad.creative_set_id == creative_set_id;
  });
  return it != client_state_->ad_prefs.filtered_ads.end();
}

bool Client::IsFlaggedAd(
    const std::string& creative_set_id) const {
  auto it = std::find_if(client_state_->ad_prefs.flagged_ads.begin(),
                         client_state_->ad_prefs.flagged_ads.end(),
                         [&creative_set_id](const FlaggedAd& ad) {
                           return ad.creative_set_id == creative_set_id;
                         });
  return it != client_state_->ad_prefs.flagged_ads.end();
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

std::map<std::string, uint64_t> Client::GetSeenAdNotifications() {
  return client_state_->seen_ad_notifications;
}

void Client::ResetSeenAdNotifications(
    const CreativeAdNotificationList& ads) {
  BLOG(INFO) << "Resetting seen ad notifications";

  for (const auto& ad : ads) {
    auto seen_ad_notification =
        client_state_->seen_ad_notifications.find(ad.creative_instance_id);
    if (seen_ad_notification != client_state_->seen_ad_notifications.end()) {
      client_state_->seen_ad_notifications.erase(seen_ad_notification);
    }
  }

  SaveState();
}

void Client::UpdateSeenPublisherAd(
    const std::string& creative_instance_id,
    const uint64_t value) {
  client_state_->seen_publisher_ads.insert({creative_instance_id, value});

  SaveState();
}

const std::map<std::string, uint64_t> Client::GetSeenPublisherAds() {
  return client_state_->seen_publisher_ads;
}

void Client::ResetSeenPublisherAds(
    const CreativePublisherAdList& ads) {
  BLOG(INFO) << "Resetting seen publisher ads";

  for (const auto& ad : ads) {
    auto publisher_ad_uuid_seen =
        client_state_->seen_publisher_ads.find(ad.creative_instance_id);
    if (publisher_ad_uuid_seen !=
        client_state_->seen_publisher_ads.end()) {
      client_state_->seen_publisher_ads.erase(publisher_ad_uuid_seen);
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

std::map<std::string, uint64_t> Client::GetSeenAdvertisers() {
  return client_state_->seen_advertisers;
}

void Client::ResetSeenAdvertisers(
     const CreativeAdNotificationList& ads) {
  BLOG(INFO) << "Resetting seen advertisers";

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

void Client::FlagShoppingState(
    const std::string& url,
    const uint64_t score) {
  client_state_->shop_activity = true;
  client_state_->shop_url = url;
  client_state_->score = score;
  client_state_->last_shop_time = Time::NowInSeconds();

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
  client_state_->last_search_time = Time::NowInSeconds();

  SaveState();
}

void Client::UnflagSearchState(
    const std::string& url) {
  if (client_state_->search_url == url) {
    return;
  }

  client_state_->search_activity = false;
  client_state_->last_search_time = Time::NowInSeconds();

  SaveState();
}

bool Client::GetSearchState() {
  return client_state_->search_activity;
}

void Client::UpdateLastUserActivity() {
  client_state_->last_user_activity = Time::NowInSeconds();

  SaveState();
}

uint64_t Client::GetLastUserActivity() {
  return client_state_->last_user_activity;
}

void Client::UpdateLastUserIdleStopTime() {
  client_state_->last_user_idle_stop_time = Time::NowInSeconds();

  SaveState();
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

std::vector<std::string> Client::GetUserModelLanguages() {
  return client_state_->user_model_languages;
}

void Client::SetLastPageClassification(
    const std::string& classification) {
  client_state_->last_page_classification = classification;

  SaveState();
}

std::string Client::GetLastPageClassification() {
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

std::deque<std::vector<double>> Client::GetPageScoreHistory() {
  return client_state_->page_score_history;
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

std::map<std::string, std::deque<uint64_t>>
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

std::map<std::string, std::deque<uint64_t>>
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

std::map<std::string, std::deque<uint64_t>> Client::GetCampaignHistory() const {
  return client_state_->campaign_history;
}

void Client::RemoveAllHistory() {
  BLOG(INFO) << "Removed all client state history";

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

  auto json = client_state_->ToJson();
  auto callback = std::bind(&Client::OnStateSaved, this, _1);
  ads_client_->Save(_client_resource_name, json, callback);
}

void Client::OnStateSaved(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to save client state";

    return;
  }

  BLOG(INFO) << "Successfully saved client state";
}

void Client::LoadState() {
  auto callback = std::bind(&Client::OnStateLoaded, this, _1, _2);
  ads_client_->Load(_client_resource_name, callback);
}

void Client::OnStateLoaded(
    const Result result,
    const std::string& json) {
  is_initialized_ = true;

  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load client state, resetting to default values";

    client_state_.reset(new ClientState());
    SaveState();
  } else {
    if (!FromJson(json)) {
      BLOG(ERROR) << "Failed to parse client state: " << json;
      callback_(FAILED);
      return;
    }

    BLOG(INFO) << "Successfully loaded client state";
  }

  callback_(SUCCESS);
}

bool Client::FromJson(
    const std::string& json) {
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
