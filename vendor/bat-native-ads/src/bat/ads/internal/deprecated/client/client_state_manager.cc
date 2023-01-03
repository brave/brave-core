/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/client/client_state_manager.h"

#include <cstdint>
#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/hash/hash.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/deprecated/client/client_info.h"
#include "bat/ads/internal/deprecated/client/client_state_manager_constants.h"
#include "bat/ads/internal/features/text_classification_features.h"
#include "bat/ads/internal/history/history_constants.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "build/build_config.h"  // IWYU pragma: keep

namespace ads {

namespace {

ClientStateManager* g_client_instance = nullptr;

constexpr uint64_t kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory = 100;

FilteredAdvertiserList::iterator FindFilteredAdvertiser(
    const std::string& advertiser_id,
    FilteredAdvertiserList* filtered_advertisers) {
  DCHECK(!advertiser_id.empty());
  DCHECK(filtered_advertisers);

  return base::ranges::find(*filtered_advertisers, advertiser_id,
                            &FilteredAdvertiserInfo::id);
}

FilteredCategoryList::iterator FindFilteredCategory(
    const std::string& category,
    FilteredCategoryList* filtered_categories) {
  DCHECK(filtered_categories);

  return base::ranges::find(*filtered_categories, category,
                            &FilteredCategoryInfo::name);
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

uint64_t GenerateHash(const std::string& value) {
  return static_cast<uint64_t>(base::PersistentHash(value));
}

void SetHash(const std::string& value) {
  AdsClientHelper::GetInstance()->SetUint64Pref(prefs::kClientHash,
                                                GenerateHash(value));
}

bool IsMutated(const std::string& value) {
  return AdsClientHelper::GetInstance()->GetUint64Pref(prefs::kClientHash) !=
         GenerateHash(value);
}

void OnSaved(const bool success) {
  if (!success) {
    BLOG(0, "Failed to save client state");

    return;
  }

  BLOG(9, "Successfully saved client state");
}

}  // namespace

ClientStateManager::ClientStateManager() : client_(new ClientInfo()) {
  DCHECK(!g_client_instance);
  g_client_instance = this;
}

ClientStateManager::~ClientStateManager() {
  DCHECK_EQ(this, g_client_instance);
  g_client_instance = nullptr;
}

// static
ClientStateManager* ClientStateManager::GetInstance() {
  DCHECK(g_client_instance);
  return g_client_instance;
}

// static
bool ClientStateManager::HasInstance() {
  return !!g_client_instance;
}

const FilteredAdvertiserList& ClientStateManager::GetFilteredAdvertisers()
    const {
  DCHECK(is_initialized_);

  return client_->ad_preferences.filtered_advertisers;
}

const FilteredCategoryList& ClientStateManager::GetFilteredCategories() const {
  DCHECK(is_initialized_);

  return client_->ad_preferences.filtered_categories;
}

const FlaggedAdList& ClientStateManager::GetFlaggedAds() const {
  DCHECK(is_initialized_);

  return client_->ad_preferences.flagged_ads;
}

void ClientStateManager::Initialize(InitializeCallback callback) {
  Load(std::move(callback));
}

void ClientStateManager::AppendHistory(const HistoryItemInfo& history_item) {
#if !BUILDFLAG(IS_IOS)
  DCHECK(is_initialized_);

  client_->history_items.push_front(history_item);

  const base::Time distant_past = base::Time::Now() - kHistoryTimeWindow;

  const auto iter = std::remove_if(
      client_->history_items.begin(), client_->history_items.end(),
      [distant_past](const HistoryItemInfo& history_item) {
        return history_item.created_at < distant_past;
      });

  client_->history_items.erase(iter, client_->history_items.cend());

  Save();
#endif
}

const HistoryItemList& ClientStateManager::GetHistory() const {
  DCHECK(is_initialized_);

  return client_->history_items;
}

void ClientStateManager::AppendToPurchaseIntentSignalHistoryForSegment(
    const std::string& segment,
    const targeting::PurchaseIntentSignalHistoryInfo& history) {
  DCHECK(is_initialized_);

  if (client_->purchase_intent_signal_history.find(segment) ==
      client_->purchase_intent_signal_history.cend()) {
    client_->purchase_intent_signal_history.insert({segment, {}});
  }

  client_->purchase_intent_signal_history.at(segment).push_back(history);

  if (client_->purchase_intent_signal_history.at(segment).size() >
      kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory) {
    client_->purchase_intent_signal_history.at(segment).pop_back();
  }

  Save();
}

const targeting::PurchaseIntentSignalHistoryMap&
ClientStateManager::GetPurchaseIntentSignalHistory() const {
  DCHECK(is_initialized_);

  return client_->purchase_intent_signal_history;
}

AdContentLikeActionType ClientStateManager::ToggleAdThumbUp(
    const AdContentInfo& ad_content) {
  DCHECK(is_initialized_);

  const auto iter = FindFilteredAdvertiser(
      ad_content.advertiser_id, &client_->ad_preferences.filtered_advertisers);
  if (iter != client_->ad_preferences.filtered_advertisers.cend()) {
    client_->ad_preferences.filtered_advertisers.erase(iter);
  }

  const AdContentLikeActionType like_action_type =
      ad_content.ToggleThumbUpActionType();

  for (auto& item : client_->history_items) {
    if (item.ad_content.advertiser_id == ad_content.advertiser_id) {
      item.ad_content.like_action_type = like_action_type;
    }
  }

  Save();

  return like_action_type;
}

AdContentLikeActionType ClientStateManager::ToggleAdThumbDown(
    const AdContentInfo& ad_content) {
  DCHECK(is_initialized_);

  const AdContentLikeActionType like_action_type =
      ad_content.ToggleThumbDownActionType();

  const auto iter = FindFilteredAdvertiser(
      ad_content.advertiser_id, &client_->ad_preferences.filtered_advertisers);

  if (like_action_type == AdContentLikeActionType::kNeutral) {
    if (iter != client_->ad_preferences.filtered_advertisers.cend()) {
      client_->ad_preferences.filtered_advertisers.erase(iter);
    }
  } else {
    if (iter == client_->ad_preferences.filtered_advertisers.cend()) {
      FilteredAdvertiserInfo filtered_advertiser;
      filtered_advertiser.id = ad_content.advertiser_id;

      client_->ad_preferences.filtered_advertisers.push_back(
          filtered_advertiser);
    }
  }

  for (auto& item : client_->history_items) {
    if (item.ad_content.advertiser_id == ad_content.advertiser_id) {
      item.ad_content.like_action_type = like_action_type;
    }
  }

  Save();

  return like_action_type;
}

AdContentLikeActionType
ClientStateManager::GetAdContentLikeActionTypeForAdvertiser(
    const std::string& advertiser_id) {
  const auto iter = base::ranges::find_if(
      client_->history_items,
      [&advertiser_id](const HistoryItemInfo& history_item) -> bool {
        return history_item.ad_content.advertiser_id == advertiser_id;
      });

  if (iter == client_->history_items.cend()) {
    return AdContentLikeActionType::kNeutral;
  }

  return iter->ad_content.like_action_type;
}

CategoryContentOptActionType ClientStateManager::ToggleAdOptIn(
    const std::string& category,
    const CategoryContentOptActionType opt_action_type) {
  DCHECK(is_initialized_);

  const auto iter = FindFilteredCategory(
      category, &client_->ad_preferences.filtered_categories);
  if (iter != client_->ad_preferences.filtered_categories.cend()) {
    client_->ad_preferences.filtered_categories.erase(iter);
  }

  const CategoryContentOptActionType toggled_opt_action_type =
      ToggleOptInActionType(opt_action_type);

  for (auto& item : client_->history_items) {
    if (item.category_content.category == category) {
      item.category_content.opt_action_type = toggled_opt_action_type;
    }
  }

  Save();

  return toggled_opt_action_type;
}

CategoryContentOptActionType ClientStateManager::ToggleAdOptOut(
    const std::string& category,
    const CategoryContentOptActionType opt_action_type) {
  DCHECK(is_initialized_);

  const CategoryContentOptActionType toggled_opt_action_type =
      ToggleOptOutActionType(opt_action_type);

  const auto iter = FindFilteredCategory(
      category, &client_->ad_preferences.filtered_categories);

  if (toggled_opt_action_type == CategoryContentOptActionType::kNone) {
    if (iter != client_->ad_preferences.filtered_categories.cend()) {
      client_->ad_preferences.filtered_categories.erase(iter);
    }
  } else {
    if (iter == client_->ad_preferences.filtered_categories.cend()) {
      FilteredCategoryInfo filtered_category;
      filtered_category.name = category;
      client_->ad_preferences.filtered_categories.push_back(filtered_category);
    }
  }

  for (auto& item : client_->history_items) {
    if (item.category_content.category == category) {
      item.category_content.opt_action_type = toggled_opt_action_type;
    }
  }

  Save();

  return toggled_opt_action_type;
}

CategoryContentOptActionType
ClientStateManager::GetCategoryContentOptActionTypeForSegment(
    const std::string& segment) {
  const auto iter = base::ranges::find_if(
      client_->history_items,
      [&segment](const HistoryItemInfo& history_item) -> bool {
        return history_item.category_content.category == segment;
      });

  if (iter == client_->history_items.cend()) {
    return CategoryContentOptActionType::kNone;
  }

  return iter->category_content.opt_action_type;
}

bool ClientStateManager::ToggleSavedAd(const AdContentInfo& ad_content) {
  DCHECK(is_initialized_);

  const bool is_saved = !ad_content.is_saved;
  if (is_saved) {
    SavedAdInfo saved_ad;
    saved_ad.creative_instance_id = ad_content.creative_instance_id;
    client_->ad_preferences.saved_ads.push_back(saved_ad);
  } else {
    const auto iter = base::ranges::find(client_->ad_preferences.saved_ads,
                                         ad_content.creative_instance_id,
                                         &SavedAdInfo::creative_instance_id);

    if (iter != client_->ad_preferences.saved_ads.cend()) {
      client_->ad_preferences.saved_ads.erase(iter);
    }
  }

  for (auto& item : client_->history_items) {
    if (item.ad_content.creative_instance_id ==
        ad_content.creative_instance_id) {
      item.ad_content.is_saved = is_saved;
    }
  }

  Save();

  return is_saved;
}

bool ClientStateManager::ToggleFlaggedAd(const AdContentInfo& ad_content) {
  DCHECK(is_initialized_);

  const bool is_flagged = !ad_content.is_flagged;
  if (is_flagged) {
    FlaggedAdInfo flagged_ad;
    flagged_ad.creative_set_id = ad_content.creative_set_id;
    client_->ad_preferences.flagged_ads.push_back(flagged_ad);
  } else {
    client_->ad_preferences.flagged_ads.erase(
        base::ranges::remove(client_->ad_preferences.flagged_ads,
                             ad_content.creative_set_id,
                             &FlaggedAdInfo::creative_set_id),
        client_->ad_preferences.flagged_ads.end());
  }

  auto item = base::ranges::find_if(
      client_->history_items, [&ad_content](const HistoryItemInfo& item) {
        return item.ad_content.creative_set_id == ad_content.creative_set_id;
      });
  if (item != client_->history_items.end()) {
    item->ad_content.is_flagged = is_flagged;
  }

  Save();

  return is_flagged;
}

void ClientStateManager::UpdateSeenAd(const AdInfo& ad) {
  DCHECK(is_initialized_);

  const std::string type_as_string = ad.type.ToString();
  client_->seen_ads[type_as_string][ad.creative_instance_id] = true;
  client_->seen_advertisers[type_as_string][ad.advertiser_id] = true;
  Save();
}

const std::map<std::string, bool>& ClientStateManager::GetSeenAdsForType(
    const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  return client_->seen_ads[type_as_string];
}

void ClientStateManager::ResetSeenAdsForType(const CreativeAdList& creative_ads,
                                             const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();

  BLOG(1, "Resetting seen " << type_as_string << "s");

  for (const auto& creative_ad : creative_ads) {
    const auto iter = client_->seen_ads[type_as_string].find(
        creative_ad.creative_instance_id);
    if (iter != client_->seen_ads[type_as_string].cend()) {
      client_->seen_ads[type_as_string].erase(iter);
    }
  }

  Save();
}

void ClientStateManager::ResetAllSeenAdsForType(const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  BLOG(1, "Resetting seen " << type_as_string << "s");
  client_->seen_ads[type_as_string] = {};
  Save();
}

const std::map<std::string, bool>&
ClientStateManager::GetSeenAdvertisersForType(const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  return client_->seen_advertisers[type_as_string];
}

void ClientStateManager::ResetSeenAdvertisersForType(
    const CreativeAdList& creative_ads,
    const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();

  BLOG(1, "Resetting seen " << type_as_string << " advertisers");

  for (const auto& creative_ad : creative_ads) {
    const auto iter = client_->seen_advertisers[type_as_string].find(
        creative_ad.advertiser_id);
    if (iter != client_->seen_advertisers[type_as_string].cend()) {
      client_->seen_advertisers[type_as_string].erase(iter);
    }
  }

  Save();
}

void ClientStateManager::ResetAllSeenAdvertisersForType(const AdType& type) {
  DCHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  BLOG(1, "Resetting seen " << type_as_string << " advertisers");
  client_->seen_advertisers[type_as_string] = {};
  Save();
}

void ClientStateManager::AppendTextClassificationProbabilitiesToHistory(
    const targeting::TextClassificationProbabilityMap& probabilities) {
  DCHECK(is_initialized_);

  client_->text_classification_probabilities.push_front(probabilities);

  const size_t maximum_entries =
      targeting::features::GetTextClassificationProbabilitiesHistorySize();
  if (client_->text_classification_probabilities.size() > maximum_entries) {
    client_->text_classification_probabilities.resize(maximum_entries);
  }

  Save();
}

const targeting::TextClassificationProbabilityList&
ClientStateManager::GetTextClassificationProbabilitiesHistory() {
  DCHECK(is_initialized_);

  return client_->text_classification_probabilities;
}

void ClientStateManager::RemoveAllHistory() {
  DCHECK(is_initialized_);

  BLOG(1, "Successfully reset client state");

  client_ = std::make_unique<ClientInfo>();

  Save();
}

///////////////////////////////////////////////////////////////////////////////

void ClientStateManager::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving client state");

  const std::string json = client_->ToJson();

  if (!is_mutated_) {
    SetHash(json);
  }

  AdsClientHelper::GetInstance()->Save(kClientStateFilename, json,
                                       base::BindOnce(&OnSaved));
}

void ClientStateManager::Load(InitializeCallback callback) {
  BLOG(3, "Loading client state");

  AdsClientHelper::GetInstance()->Load(
      kClientStateFilename,
      base::BindOnce(&ClientStateManager::OnLoaded, base::Unretained(this),
                     std::move(callback)));
}

void ClientStateManager::OnLoaded(InitializeCallback callback,
                                  const bool success,
                                  const std::string& json) {
  if (!success) {
    BLOG(3, "Client state does not exist, creating default state");

    is_initialized_ = true;

    client_ = std::make_unique<ClientInfo>();
    Save();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load client state");

      BLOG(3, "Failed to parse client state: " << json);

      std::move(callback).Run(/*success*/ false);
      return;
    }

    BLOG(3, "Successfully loaded client state");

    is_initialized_ = true;
  }

  is_mutated_ = IsMutated(client_->ToJson());
  if (is_mutated_) {
    BLOG(9, "Client state is mutated");
  }

  std::move(callback).Run(/*success */ true);
}

bool ClientStateManager::FromJson(const std::string& json) {
  ClientInfo client;
  if (!client.FromJson(json)) {
    return false;
  }

  client_ = std::make_unique<ClientInfo>(client);

  return true;
}

}  // namespace ads
