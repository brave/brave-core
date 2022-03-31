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
#include "bat/ads/internal/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_signal_history_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/client/client_info.h"
#include "bat/ads/internal/features/text_classification/text_classification_features.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"
#include "build/build_config.h"

namespace ads {

namespace {

Client* g_client = nullptr;

constexpr char kClientFilename[] = "client.json";

constexpr uint64_t kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory = 100;

FilteredAdvertiserList::iterator FindFilteredAdvertiser(
    const std::string& advertiser_id,
    FilteredAdvertiserList* filtered_advertisers) {
  DCHECK(!advertiser_id.empty());
  DCHECK(filtered_advertisers);

  return std::find_if(
      filtered_advertisers->begin(), filtered_advertisers->end(),
      [&advertiser_id](const FilteredAdvertiserInfo& filtered_advertiser) {
        return filtered_advertiser.id == advertiser_id;
      });
}

FilteredCategoryList::iterator FindFilteredCategory(
    const std::string& category,
    FilteredCategoryList* filtered_categories) {
  DCHECK(filtered_categories);

  return std::find_if(
      filtered_categories->begin(), filtered_categories->end(),
      [&category](const FilteredCategoryInfo& filtered_category) {
        return filtered_category.name == category;
      });
}

CategoryContentOptActionType ToggleOptInActionType(
    const CategoryContentOptActionType action_type) {
  if (action_type == CategoryContentOptActionType::kOptIn) {
    return CategoryContentOptActionType::kNone;
  }

  return CategoryContentOptActionType::kOptIn;
}

CategoryContentOptActionType ToggleOptOutActionType(
    const CategoryContentOptActionType action_type) {
  if (action_type == CategoryContentOptActionType::kOptOut) {
    return CategoryContentOptActionType::kNone;
  }

  return CategoryContentOptActionType::kOptOut;
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

FilteredAdvertiserList Client::GetFilteredAdvertisers() const {
  DCHECK(is_initialized_);

  return client_->ad_preferences.filtered_advertisers;
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
#if !BUILDFLAG(IS_IOS)
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

AdContentLikeActionType Client::ToggleAdThumbUp(
    const AdContentInfo& ad_content) {
  DCHECK(is_initialized_);

  const auto iter = FindFilteredAdvertiser(
      ad_content.advertiser_id, &client_->ad_preferences.filtered_advertisers);
  if (iter != client_->ad_preferences.filtered_advertisers.end()) {
    client_->ad_preferences.filtered_advertisers.erase(iter);
  }

  const AdContentLikeActionType like_action_type =
      ad_content.ToggleThumbUpActionType();

  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.advertiser_id == ad_content.advertiser_id) {
      item.ad_content.like_action_type = like_action_type;
    }
  }

  Save();

  return like_action_type;
}

AdContentLikeActionType Client::ToggleAdThumbDown(
    const AdContentInfo& ad_content) {
  DCHECK(is_initialized_);

  const AdContentLikeActionType like_action_type =
      ad_content.ToggleThumbDownActionType();

  const auto iter = FindFilteredAdvertiser(
      ad_content.advertiser_id, &client_->ad_preferences.filtered_advertisers);

  if (like_action_type == AdContentLikeActionType::kNeutral) {
    if (iter != client_->ad_preferences.filtered_advertisers.end()) {
      client_->ad_preferences.filtered_advertisers.erase(iter);
    }
  } else {
    if (iter == client_->ad_preferences.filtered_advertisers.end()) {
      FilteredAdvertiserInfo filtered_advertiser;
      filtered_advertiser.id = ad_content.advertiser_id;

      client_->ad_preferences.filtered_advertisers.push_back(
          filtered_advertiser);
    }
  }

  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.advertiser_id == ad_content.advertiser_id) {
      item.ad_content.like_action_type = like_action_type;
    }
  }

  Save();

  return like_action_type;
}

AdContentLikeActionType Client::GetAdContentLikeActionTypeForAdvertiser(
    const std::string& advertiser_id) {
  for (const auto& element : client_->ads_shown_history) {
    if (element.ad_content.advertiser_id == advertiser_id) {
      return element.ad_content.like_action_type;
    }
  }

  return AdContentLikeActionType::kNeutral;
}

CategoryContentOptActionType Client::ToggleAdOptIn(
    const std::string& category,
    const CategoryContentOptActionType opt_action_type) {
  DCHECK(is_initialized_);

  const auto iter = FindFilteredCategory(
      category, &client_->ad_preferences.filtered_categories);
  if (iter != client_->ad_preferences.filtered_categories.end()) {
    client_->ad_preferences.filtered_categories.erase(iter);
  }

  const CategoryContentOptActionType toggled_opt_action_type =
      ToggleOptInActionType(opt_action_type);

  for (auto& item : client_->ads_shown_history) {
    if (item.category_content.category == category) {
      item.category_content.opt_action_type = toggled_opt_action_type;
    }
  }

  Save();

  return toggled_opt_action_type;
}

CategoryContentOptActionType Client::ToggleAdOptOut(
    const std::string& category,
    const CategoryContentOptActionType opt_action_type) {
  DCHECK(is_initialized_);

  const CategoryContentOptActionType toggled_opt_action_type =
      ToggleOptOutActionType(opt_action_type);

  const auto iter = FindFilteredCategory(
      category, &client_->ad_preferences.filtered_categories);

  if (toggled_opt_action_type == CategoryContentOptActionType::kNone) {
    if (iter != client_->ad_preferences.filtered_categories.end()) {
      client_->ad_preferences.filtered_categories.erase(iter);
    }
  } else {
    if (iter == client_->ad_preferences.filtered_categories.end()) {
      FilteredCategoryInfo filtered_category;
      filtered_category.name = category;
      client_->ad_preferences.filtered_categories.push_back(filtered_category);
    }
  }

  for (auto& item : client_->ads_shown_history) {
    if (item.category_content.category == category) {
      item.category_content.opt_action_type = toggled_opt_action_type;
    }
  }

  Save();

  return toggled_opt_action_type;
}

CategoryContentOptActionType Client::GetCategoryContentOptActionTypeForSegment(
    const std::string& segment) {
  for (const auto& element : client_->ads_shown_history) {
    if (element.category_content.category == segment) {
      return element.category_content.opt_action_type;
    }
  }

  return CategoryContentOptActionType::kNone;
}

bool Client::ToggleSavedAd(const AdContentInfo& ad_content) {
  DCHECK(is_initialized_);

  const bool is_saved = !ad_content.is_saved;
  if (is_saved) {
    SavedAdInfo saved_ad;
    saved_ad.creative_instance_id = ad_content.creative_instance_id;
    client_->ad_preferences.saved_ads.push_back(saved_ad);
  } else {
    const auto iter = std::find_if(client_->ad_preferences.saved_ads.cbegin(),
                                   client_->ad_preferences.saved_ads.cend(),
                                   [&ad_content](const SavedAdInfo& saved_ad) {
                                     return saved_ad.creative_instance_id ==
                                            ad_content.creative_instance_id;
                                   });

    if (iter != client_->ad_preferences.saved_ads.end()) {
      client_->ad_preferences.saved_ads.erase(iter);
    }
  }

  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.creative_instance_id ==
        ad_content.creative_instance_id) {
      item.ad_content.is_saved = is_saved;
    }
  }

  Save();

  return is_saved;
}

bool Client::ToggleFlaggedAd(const AdContentInfo& ad_content) {
  DCHECK(is_initialized_);

  const bool is_flagged = !ad_content.is_flagged;
  if (is_flagged) {
    FlaggedAdInfo flagged_ad;
    flagged_ad.creative_set_id = ad_content.creative_set_id;
    client_->ad_preferences.flagged_ads.push_back(flagged_ad);
  } else {
    const auto iter = std::find_if(
        client_->ad_preferences.flagged_ads.cbegin(),
        client_->ad_preferences.flagged_ads.cend(),
        [&ad_content](const FlaggedAdInfo& flagged_ad) {
          return flagged_ad.creative_set_id == ad_content.creative_set_id;
        });

    if (iter != client_->ad_preferences.flagged_ads.end()) {
      client_->ad_preferences.flagged_ads.erase(iter);
    }
  }

  for (auto& item : client_->ads_shown_history) {
    if (item.ad_content.creative_set_id == ad_content.creative_set_id) {
      item.ad_content.is_flagged = is_flagged;
    }
  }

  Save();

  return is_flagged;
}

void Client::UpdateSeenAd(const AdInfo& ad) {
  DCHECK(is_initialized_);

  const std::string type_as_string = ad.type.ToString();
  client_->seen_ads[type_as_string][ad.creative_instance_id] = true;
  client_->seen_advertisers[type_as_string][ad.advertiser_id] = true;
  Save();
}

const std::map<std::string, bool>& Client::GetSeenAdsForType(
    const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  return client_->seen_ads[type_as_string];
}

void Client::ResetSeenAdsForType(const CreativeAdList& creative_ads,
                                 const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();

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

  const std::string type_as_string = type.ToString();
  BLOG(1, "Resetting seen " << type_as_string << "s");
  client_->seen_ads[type_as_string] = {};
  Save();
}

const std::map<std::string, bool>& Client::GetSeenAdvertisersForType(
    const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  return client_->seen_advertisers[type_as_string];
}

void Client::ResetSeenAdvertisersForType(const CreativeAdList& creative_ads,
                                         const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();

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

  const std::string type_as_string = type.ToString();
  BLOG(1, "Resetting seen " << type_as_string << " advertisers");
  client_->seen_advertisers[type_as_string] = {};
  Save();
}

void Client::SetServeAdAt(const base::Time time) {
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
