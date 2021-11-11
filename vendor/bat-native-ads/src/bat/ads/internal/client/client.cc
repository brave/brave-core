/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/client.h"

#include <algorithm>
#include <cstdint>
#include <functional>

#include "base/check_op.h"
#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_signal_history_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/client/client_info.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/features/text_classification/text_classification_features.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {

Client* g_client = nullptr;

const char kClientFilename[] = "client.json";

const uint64_t kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory = 100;

FilteredAdList::iterator FindFilteredAd(const std::string& creative_instance_id,
                                        FilteredAdList* filtered_ads) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(filtered_ads);

  return std::find_if(
      filtered_ads->begin(), filtered_ads->end(),
      [&creative_instance_id](const FilteredAdInfo& filtered_ad) {
        return filtered_ad.creative_instance_id == creative_instance_id;
      });
}

FilteredCategoryList::iterator FindFilteredCategory(
    const std::string& name,
    FilteredCategoryList* filtered_categories) {
  DCHECK(filtered_categories);

  return std::find_if(filtered_categories->begin(), filtered_categories->end(),
                      [&name](const FilteredCategoryInfo& category) {
                        return category.name == name;
                      });
}

}  // namespace

Client::Client() : client_(new ClientInfo()) {
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

FilteredAdList Client::GetFilteredAds() const {
  DCHECK(is_initialized_);

  return client_->ad_preferences.filtered_ads;
}

FilteredCategoryList Client::GetFilteredCategories() const {
  DCHECK(is_initialized_);

  return client_->ad_preferences.filtered_categories;
}

FlaggedAdList Client::GetFlaggedAds() const {
  DCHECK(is_initialized_);

  return client_->ad_preferences.flagged_ads;
}

void Client::Initialize(InitializeCallback callback) {
  callback_ = callback;

  Load();
}

void Client::AppendAdHistory(const AdHistoryInfo& ad_history) {
#if !defined(OS_IOS)
  DCHECK(is_initialized_);

  client_->ads_shown_history.push_front(ad_history);

  const base::Time distant_past =
      base::Time::Now() - base::Days(history::kForDays);

  const auto iter = std::remove_if(
      client_->ads_shown_history.begin(), client_->ads_shown_history.end(),
      [&distant_past](const AdHistoryInfo& ad_history) {
        const base::Time time = base::Time::FromDoubleT(ad_history.timestamp);
        return time < distant_past;
      });

  client_->ads_shown_history.erase(iter, client_->ads_shown_history.end());

  Save();
#endif
}

const std::deque<AdHistoryInfo>& Client::GetAdsHistory() const {
  DCHECK(is_initialized_);

  return client_->ads_shown_history;
}

void Client::AppendToPurchaseIntentSignalHistoryForSegment(
    const std::string& segment,
    const ad_targeting::PurchaseIntentSignalHistoryInfo& history) {
  DCHECK(is_initialized_);

  if (client_->purchase_intent_signal_history.find(segment) ==
      client_->purchase_intent_signal_history.end()) {
    client_->purchase_intent_signal_history.insert({segment, {}});
  }

  client_->purchase_intent_signal_history.at(segment).push_back(history);

  if (client_->purchase_intent_signal_history.at(segment).size() >
      kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory) {
    client_->purchase_intent_signal_history.at(segment).pop_back();
  }

  Save();
}

const ad_targeting::PurchaseIntentSignalHistoryMap&
Client::GetPurchaseIntentSignalHistory() const {
  DCHECK(is_initialized_);

  return client_->purchase_intent_signal_history;
}

AdContentActionType Client::ToggleAdThumbUp(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContentActionType action) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(!creative_set_id.empty());

  DCHECK(is_initialized_);

  AdContentActionType like_action;
  if (action == AdContentActionType::kThumbsUp) {
    like_action = AdContentActionType::kNeutral;
  } else {
    like_action = AdContentActionType::kThumbsUp;
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

AdContentActionType Client::ToggleAdThumbDown(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContentActionType action) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(!creative_set_id.empty());

  DCHECK(is_initialized_);

  AdContentActionType like_action;
  if (action == AdContentActionType::kThumbsDown) {
    like_action = AdContentActionType::kNeutral;
  } else {
    like_action = AdContentActionType::kThumbsDown;
  }

  // Update this ad in the filtered ads list
  auto it_ad = FindFilteredAd(creative_instance_id,
                              &client_->ad_preferences.filtered_ads);
  if (like_action == AdContentActionType::kNeutral) {
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

AdContentActionType Client::GetLikeActionForSegment(
    const std::string& segment) {
  for (const auto& element : client_->ads_shown_history) {
    if (element.category_content.category == segment) {
      return element.ad_content.like_action;
    }
  }

  return AdContentActionType::kNeutral;
}

CategoryContentActionType Client::ToggleAdOptInAction(
    const std::string& category,
    const CategoryContentActionType action) {
  DCHECK(is_initialized_);

  CategoryContentActionType opt_action;
  if (action == CategoryContentActionType::kOptIn) {
    opt_action = CategoryContentActionType::kNone;
  } else {
    opt_action = CategoryContentActionType::kOptIn;
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

CategoryContentActionType Client::ToggleAdOptOutAction(
    const std::string& category,
    const CategoryContentActionType action) {
  DCHECK(is_initialized_);

  CategoryContentActionType opt_action;
  if (action == CategoryContentActionType::kOptOut) {
    opt_action = CategoryContentActionType::kNone;
  } else {
    opt_action = CategoryContentActionType::kOptOut;
  }

  // Update this category in the filtered categories list
  auto it = FindFilteredCategory(category,
                                 &client_->ad_preferences.filtered_categories);
  if (opt_action == CategoryContentActionType::kNone) {
    if (it != client_->ad_preferences.filtered_categories.end()) {
      client_->ad_preferences.filtered_categories.erase(it);
    }
  } else {
    if (it == client_->ad_preferences.filtered_categories.end()) {
      FilteredCategoryInfo filtered_category;
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

CategoryContentActionType Client::GetOptActionForSegment(
    const std::string& segment) {
  for (const auto& element : client_->ads_shown_history) {
    if (element.category_content.category == segment) {
      return element.category_content.opt_action;
    }
  }

  return CategoryContentActionType::kNone;
}

bool Client::ToggleSaveAd(const std::string& creative_instance_id,
                          const std::string& creative_set_id,
                          const bool saved) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(!creative_set_id.empty());

  DCHECK(is_initialized_);

  const bool is_saved_ad = !saved;

  // Update this ad in the saved ads list
  auto it_ad = std::find_if(
      client_->ad_preferences.saved_ads.cbegin(),
      client_->ad_preferences.saved_ads.cend(),
      [&creative_instance_id](const SavedAdInfo& saved_ad) {
        return saved_ad.creative_instance_id == creative_instance_id;
      });

  if (is_saved_ad) {
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

  // Update the history detail for ads matching this creative instance id
  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.saved_ad = is_saved_ad;
    }
  }

  Save();

  return is_saved_ad;
}

bool Client::GetSavedAdForCreativeInstanceId(
    const std::string& creative_instance_id) {
  for (const auto& element : client_->ads_shown_history) {
    if (element.ad_content.creative_instance_id == creative_instance_id) {
      return element.ad_content.saved_ad;
    }
  }

  return false;
}

bool Client::ToggleFlagAd(const std::string& creative_instance_id,
                          const std::string& creative_set_id,
                          const bool flagged) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(!creative_set_id.empty());

  DCHECK(is_initialized_);

  const bool is_flagged_ad = !flagged;

  // Update this ad in the flagged ads list
  auto it_ad = std::find_if(
      client_->ad_preferences.flagged_ads.cbegin(),
      client_->ad_preferences.flagged_ads.cend(),
      [&creative_instance_id](const FlaggedAdInfo& flagged_ad) {
        return flagged_ad.creative_instance_id == creative_instance_id;
      });

  if (is_flagged_ad) {
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

  // Update the history detail for ads matching this creative instance id
  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.creative_instance_id == creative_instance_id) {
      item.ad_content.flagged_ad = is_flagged_ad;
    }
  }

  Save();

  return is_flagged_ad;
}

bool Client::GetFlaggedAdForCreativeInstanceId(
    const std::string& creative_instance_id) {
  for (const auto& element : client_->ads_shown_history) {
    if (element.ad_content.creative_instance_id == creative_instance_id) {
      return element.ad_content.flagged_ad;
    }
  }

  return false;
}

void Client::UpdateSeenAd(const AdInfo& ad) {
  DCHECK(is_initialized_);

  const std::string type_as_string = std::string(ad.type);
  client_->seen_ads[type_as_string][ad.creative_instance_id] = true;
  client_->seen_advertisers[type_as_string][ad.advertiser_id] = true;
  Save();
}

const std::map<std::string, bool>& Client::GetSeenAdsForType(
    const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = std::string(type);
  return client_->seen_ads[type_as_string];
}

void Client::ResetSeenAdsForType(const CreativeAdList& creative_ads,
                                 const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = std::string(type);

  BLOG(1, "Resetting seen " << type_as_string << "s");

  for (const auto& creative_ad : creative_ads) {
    const auto iter = client_->seen_ads[type_as_string].find(
        creative_ad.creative_instance_id);
    if (iter != client_->seen_ads[type_as_string].end()) {
      client_->seen_ads[type_as_string].erase(iter);
    }
  }

  Save();
}

void Client::ResetAllSeenAdsForType(const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = std::string(type);
  BLOG(1, "Resetting seen " << type_as_string << "s");
  client_->seen_ads[type_as_string] = {};
  Save();
}

const std::map<std::string, bool>& Client::GetSeenAdvertisersForType(
    const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = std::string(type);
  return client_->seen_advertisers[type_as_string];
}

void Client::ResetSeenAdvertisersForType(const CreativeAdList& creative_ads,
                                         const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = std::string(type);

  BLOG(1, "Resetting seen " << type_as_string << " advertisers");

  for (const auto& creative_ad : creative_ads) {
    const auto iter = client_->seen_advertisers[type_as_string].find(
        creative_ad.advertiser_id);
    if (iter != client_->seen_advertisers[type_as_string].end()) {
      client_->seen_advertisers[type_as_string].erase(iter);
    }
  }

  Save();
}

void Client::ResetAllSeenAdvertisersForType(const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = std::string(type);
  BLOG(1, "Resetting seen " << type_as_string << " advertisers");
  client_->seen_advertisers[type_as_string] = {};
  Save();
}

void Client::SetServeAdAt(const base::Time& time) {
  DCHECK(is_initialized_);

  client_->serve_ad_at = time;

  Save();
}

base::Time Client::GetServeAdAt() {
  DCHECK(is_initialized_);

  return client_->serve_ad_at;
}

void Client::AppendTextClassificationProbabilitiesToHistory(
    const ad_targeting::TextClassificationProbabilitiesMap& probabilities) {
  DCHECK(is_initialized_);

  client_->text_classification_probabilities.push_front(probabilities);

  const size_t maximum_entries =
      features::GetTextClassificationProbabilitiesHistorySize();
  if (client_->text_classification_probabilities.size() > maximum_entries) {
    client_->text_classification_probabilities.resize(maximum_entries);
  }

  Save();
}

const ad_targeting::TextClassificationProbabilitiesList&
Client::GetTextClassificationProbabilitiesHistory() {
  DCHECK(is_initialized_);

  return client_->text_classification_probabilities;
}

void Client::RemoveAllHistory() {
  DCHECK(is_initialized_);

  BLOG(1, "Successfully reset client state");

  client_.reset(new ClientInfo());

  Save();
}

std::string Client::GetVersionCode() const {
  DCHECK(is_initialized_);

  return client_->version_code;
}

void Client::SetVersionCode(const std::string& value) {
  DCHECK(is_initialized_);

  client_->version_code = value;

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

void Client::OnSaved(const bool success) {
  if (!success) {
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

void Client::OnLoaded(const bool success, const std::string& json) {
  if (!success) {
    BLOG(3, "Client state does not exist, creating default state");

    is_initialized_ = true;

    client_.reset(new ClientInfo());
    Save();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load client state");

      BLOG(3, "Failed to parse client state: " << json);

      callback_(/* success */ false);
      return;
    }

    BLOG(3, "Successfully loaded client state");

    is_initialized_ = true;
  }

  callback_(/* success  */ true);
}

bool Client::FromJson(const std::string& json) {
  ClientInfo client;
  const bool success = LoadFromJson(&client, json);
  if (!success) {
    return false;
  }

  client_.reset(new ClientInfo(client));
  Save();

  return true;
}

}  // namespace ads
