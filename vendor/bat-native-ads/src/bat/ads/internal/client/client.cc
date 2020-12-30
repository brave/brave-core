/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/client.h"

#include <algorithm>
#include <functional>

#include "bat/ads/ad_content_info.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/category_content_info.h"
#include "bat/ads/internal/ad_targeting/data_types/purchase_intent/purchase_intent_signal_history_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/features/features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/json_helper.h"

namespace ads {

namespace {

Client* g_client = nullptr;

const char kClientFilename[] = "client.json";

// Maximum entries based upon 7 days of history for 20 ads per day, 3
// confirmation types (viewed, clicked and dismissed) for ad notifications and
// 2 confirmation types (viewed and clicked) for new tab page ads
const uint64_t kMaximumEntriesInAdsShownHistory = 7 * ((20 * 3) + (20 * 2));

const uint64_t kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory = 100;

FilteredAdList::iterator FindFilteredAd(
    const std::string& creative_instance_id,
    FilteredAdList* filtered_ads) {
  DCHECK(filtered_ads);

  return std::find_if(filtered_ads->begin(), filtered_ads->end(),
      [&creative_instance_id](const FilteredAdInfo& filtered_ad) {
    return filtered_ad.creative_instance_id == creative_instance_id;
  });
}

FilteredCategoryList::iterator FindFilteredCategory(
    const std::string& name,
    FilteredCategoryList* filtered_categories) {
  DCHECK(filtered_categories);

  return std::find_if(filtered_categories->begin(), filtered_categories->end(),
      [&name](const FilteredCategory& category) {
    return category.name == name;
  });
}

}  // namespace

Client::Client()
    : client_(new ClientInfo()) {
  DCHECK_EQ(g_client, nullptr);
  g_client = this;
}

Client::~Client() {
  DCHECK(g_client);
  g_client = nullptr;
}

// static
Client* Client::Get() {
  DCHECK(g_client);
  return g_client;
}

// static
bool Client::HasInstance() {
  return g_client;
}

FilteredAdList Client::get_filtered_ads() const {
  return client_->ad_preferences.filtered_ads;
}

FilteredCategoryList Client::get_filtered_categories() const {
  return client_->ad_preferences.filtered_categories;
}

FlaggedAdList Client::get_flagged_ads() const {
  return client_->ad_preferences.flagged_ads;
}

void Client::Initialize(
    InitializeCallback callback) {
  callback_ = callback;

  Load();
}

void Client::AppendAdHistoryToAdsHistory(
    const AdHistoryInfo& ad_history) {
  client_->ads_shown_history.push_front(ad_history);

  if (client_->ads_shown_history.size() >
      kMaximumEntriesInAdsShownHistory) {
    client_->ads_shown_history.pop_back();
  }

  Save();
}

const std::deque<AdHistoryInfo>& Client::GetAdsHistory() const {
  return client_->ads_shown_history;
}

void Client::AppendToPurchaseIntentSignalHistoryForSegment(
    const std::string& segment,
    const PurchaseIntentSignalHistoryInfo& history) {
  if (client_->purchase_intent_signal_history.find(segment) ==
      client_->purchase_intent_signal_history.end()) {
    client_->purchase_intent_signal_history.insert({segment, {}});
  }

  client_->purchase_intent_signal_history.at(
      segment).push_back(history);

  if (client_->purchase_intent_signal_history.at(segment).size() >
      kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory) {
    client_->purchase_intent_signal_history.at(segment).pop_back();
  }

  Save();
}

const PurchaseIntentSignalHistoryMap&
Client::GetPurchaseIntentSignalHistory() const {
  return client_->purchase_intent_signal_history;
}

AdContentInfo::LikeAction Client::ToggleAdThumbUp(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContentInfo::LikeAction action) {
  AdContentInfo::LikeAction like_action;
  if (action == AdContentInfo::LikeAction::kThumbsUp) {
    like_action = AdContentInfo::LikeAction::kNeutral;
  } else {
    like_action = AdContentInfo::LikeAction::kThumbsUp;
  }

  // Remove this ad from the filtered ads list
  auto it_ad = FindFilteredAd(creative_instance_id,
      &client_->ad_preferences.filtered_ads);
  if (it_ad != client_->ad_preferences.filtered_ads.end()) {
    client_->ad_preferences.filtered_ads.erase(it_ad);
  }

  // Update the history detail for ads matching this UUID
  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.like_action = like_action;
    }
  }

  Save();

  return like_action;
}

AdContentInfo::LikeAction Client::ToggleAdThumbDown(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContentInfo::LikeAction action) {
  AdContentInfo::LikeAction like_action;
  if (action == AdContentInfo::LikeAction::kThumbsDown) {
    like_action = AdContentInfo::LikeAction::kNeutral;
  } else {
    like_action = AdContentInfo::LikeAction::kThumbsDown;
  }

  // Update this ad in the filtered ads list
  auto it_ad = FindFilteredAd(creative_instance_id,
      &client_->ad_preferences.filtered_ads);
  if (like_action == AdContentInfo::LikeAction::kNeutral) {
    if (it_ad != client_->ad_preferences.filtered_ads.end()) {
      client_->ad_preferences.filtered_ads.erase(it_ad);
    }
  } else {
    if (it_ad == client_->ad_preferences.filtered_ads.end()) {
      FilteredAdInfo filtered_ad;
      filtered_ad.creative_instance_id = creative_instance_id;
      filtered_ad.creative_set_id = creative_set_id;
      client_->ad_preferences.filtered_ads.push_back(filtered_ad);
    }
  }

  // Update the history detail for ads matching this UUID
  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.like_action = like_action;
    }
  }

  Save();

  return like_action;
}

CategoryContentInfo::OptAction Client::ToggleAdOptInAction(
    const std::string& category,
    const CategoryContentInfo::OptAction action) {
  CategoryContentInfo::OptAction opt_action;
  if (action == CategoryContentInfo::OptAction::kOptIn) {
    opt_action = CategoryContentInfo::OptAction::kNone;
  } else {
    opt_action = CategoryContentInfo::OptAction::kOptIn;
  }

  // Remove this category from the filtered categories list
  auto it = FindFilteredCategory(category,
      &client_->ad_preferences.filtered_categories);
  if (it != client_->ad_preferences.filtered_categories.end()) {
    client_->ad_preferences.filtered_categories.erase(it);
  }

  // Update the history for this category
  for (auto& item : client_->ads_shown_history) {
    if (item.category_content.category == category) {
      item.category_content.opt_action = opt_action;
    }
  }

  Save();

  return opt_action;
}

CategoryContentInfo::OptAction Client::ToggleAdOptOutAction(
    const std::string& category,
    const CategoryContentInfo::OptAction action) {
  CategoryContentInfo::OptAction opt_action;
  if (action == CategoryContentInfo::OptAction::kOptOut) {
    opt_action = CategoryContentInfo::OptAction::kNone;
  } else {
    opt_action = CategoryContentInfo::CategoryContentInfo::OptAction::kOptOut;
  }

  // Update this category in the filtered categories list
  auto it = FindFilteredCategory(category,
      &client_->ad_preferences.filtered_categories);
  if (opt_action == CategoryContentInfo::OptAction::kNone) {
    if (it != client_->ad_preferences.filtered_categories.end()) {
      client_->ad_preferences.filtered_categories.erase(it);
    }
  } else {
    if (it == client_->ad_preferences.filtered_categories.end()) {
      FilteredCategory filtered_category;
      filtered_category.name = category;
      client_->ad_preferences.filtered_categories.push_back(filtered_category);
    }
  }

  // Update the history for this category
  for (auto& item : client_->ads_shown_history) {
    if (item.category_content.category == category) {
      item.category_content.opt_action = opt_action;
    }
  }

  Save();

  return opt_action;
}

bool Client::ToggleSaveAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool saved) {
  const bool saved_ad = !saved;

  // Update this ad in the saved ads list
  auto it_ad = std::find_if(client_->ad_preferences.saved_ads.begin(),
      client_->ad_preferences.saved_ads.end(),
          [&creative_instance_id](const SavedAdInfo& saved_ad) {
    return saved_ad.creative_instance_id == creative_instance_id;
  });

  if (saved_ad) {
    if (it_ad == client_->ad_preferences.saved_ads.end()) {
      SavedAdInfo saved_ad;
      saved_ad.creative_instance_id = creative_instance_id;
      saved_ad.creative_set_id = creative_set_id;
      client_->ad_preferences.saved_ads.push_back(saved_ad);
    }
  } else {
    if (it_ad != client_->ad_preferences.saved_ads.end()) {
      client_->ad_preferences.saved_ads.erase(it_ad);
    }
  }

  // Update the history detail for ads matching this UUID
  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.saved_ad = saved_ad;
    }
  }

  Save();

  return saved_ad;
}

bool Client::ToggleFlagAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool flagged) {
  const bool flagged_ad = !flagged;

  // Update this ad in the flagged ads list
  auto it_ad = std::find_if(client_->ad_preferences.flagged_ads.begin(),
      client_->ad_preferences.flagged_ads.end(),
          [&creative_instance_id](const FlaggedAdInfo& flagged_ad) {
    return flagged_ad.creative_instance_id == creative_instance_id;
  });

  if (flagged_ad) {
    if (it_ad == client_->ad_preferences.flagged_ads.end()) {
      FlaggedAdInfo flagged_ad;
      flagged_ad.creative_instance_id = creative_instance_id;
      flagged_ad.creative_set_id = creative_set_id;
      client_->ad_preferences.flagged_ads.push_back(flagged_ad);
    }
  } else {
    if (it_ad != client_->ad_preferences.flagged_ads.end()) {
      client_->ad_preferences.flagged_ads.erase(it_ad);
    }
  }

  // Update the history detail for ads matching this UUID
  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.flagged_ad = flagged_ad;
    }
  }

  Save();

  return flagged_ad;
}

void Client::UpdateSeenAdNotification(
    const std::string& creative_instance_id) {
  client_->seen_ad_notifications.insert({creative_instance_id, 1});

  Save();
}

const std::map<std::string, uint64_t>& Client::GetSeenAdNotifications() {
  return client_->seen_ad_notifications;
}

void Client::ResetSeenAdNotifications(
    const CreativeAdNotificationList& ads) {
  BLOG(1, "Resetting seen ad notifications");

  for (const auto& ad : ads) {
    auto seen_ad_notification =
        client_->seen_ad_notifications.find(ad.creative_instance_id);
    if (seen_ad_notification != client_->seen_ad_notifications.end()) {
      client_->seen_ad_notifications.erase(seen_ad_notification);
    }
  }

  Save();
}

void Client::UpdateSeenAdvertiser(
    const std::string& advertiser_id) {
  client_->seen_advertisers.insert({advertiser_id, 1});

  Save();
}

const std::map<std::string, uint64_t>& Client::GetSeenAdvertisers() {
  return client_->seen_advertisers;
}

void Client::ResetSeenAdvertisers(
    const CreativeAdNotificationList& ads) {
  BLOG(1, "Resetting seen advertisers");

  for (const auto& ad : ads) {
    auto seen_advertiser =
        client_->seen_advertisers.find(ad.advertiser_id);
    if (seen_advertiser != client_->seen_advertisers.end()) {
      client_->seen_advertisers.erase(seen_advertiser);
    }
  }

  Save();
}

void Client::SetNextAdServingInterval(
    const base::Time& next_check_serve_ad_date) {
  client_->next_ad_serving_interval_timestamp_
      = static_cast<uint64_t>(next_check_serve_ad_date.ToDoubleT());

  Save();
}

base::Time Client::GetNextAdServingInterval() {
  return base::Time::FromDoubleT(
      client_->next_ad_serving_interval_timestamp_);
}

void Client::AppendTextClassificationProbabilitiesToHistory(
    const TextClassificationProbabilitiesMap& probabilities) {
  client_->text_classification_probabilities.push_front(probabilities);

  const size_t maximum_entries = features::GetPageProbabilitiesHistorySize();
  if (client_->text_classification_probabilities.size() > maximum_entries) {
    client_->text_classification_probabilities.resize(maximum_entries);
  }

  Save();
}

const TextClassificationProbabilitiesList&
Client::GetTextClassificationProbabilitiesHistory() {
  return client_->text_classification_probabilities;
}

void Client::RemoveAllHistory() {
  BLOG(1, "Successfully reset client state");

  client_.reset(new ClientInfo());

  Save();
}

///////////////////////////////////////////////////////////////////////////////

void Client::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving client state");

  auto json = client_->ToJson();
  auto callback = std::bind(&Client::OnSaved, this, std::placeholders::_1);
  AdsClientHelper::Get()->Save(kClientFilename, json, callback);
}

void Client::OnSaved(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to save client state");

    return;
  }

  BLOG(9, "Successfully saved client state");
}

void Client::Load() {
  BLOG(3, "Loading client state");

  auto callback = std::bind(&Client::OnLoaded, this, std::placeholders::_1,
      std::placeholders::_2);
  AdsClientHelper::Get()->Load(kClientFilename, callback);
}

void Client::OnLoaded(
    const Result result,
    const std::string& json) {
  if (result != SUCCESS) {
    BLOG(3, "Client state does not exist, creating default state");

    is_initialized_ = true;

    client_.reset(new ClientInfo());
    Save();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load client state");

      BLOG(3, "Failed to parse client state: " << json);

      callback_(FAILED);
      return;
    }

    BLOG(3, "Successfully loaded client state");

    is_initialized_ = true;
  }

  callback_(SUCCESS);
}

bool Client::FromJson(
    const std::string& json) {
  ClientInfo client;
  auto result = LoadFromJson(&client, json);
  if (result != SUCCESS) {
    return false;
  }

  client_.reset(new ClientInfo(client));
  Save();

  return true;
}

}  // namespace ads
