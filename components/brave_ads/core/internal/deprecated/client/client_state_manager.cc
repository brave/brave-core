/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/hash/hash.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/history/history_constants.h"
#include "build/build_config.h"  // IWYU pragma: keep

namespace brave_ads {

namespace {

constexpr size_t kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory = 100;

FilteredAdvertiserList::iterator FindFilteredAdvertiser(
    const std::string& advertiser_id,
    FilteredAdvertiserList* filtered_advertisers) {
  CHECK(!advertiser_id.empty());
  CHECK(filtered_advertisers);

  return base::ranges::find(*filtered_advertisers, advertiser_id,
                            &FilteredAdvertiserInfo::id);
}

FilteredCategoryList::iterator FindFilteredCategory(
    const std::string& category,
    FilteredCategoryList* filtered_categories) {
  CHECK(!category.empty());
  CHECK(filtered_categories);

  return base::ranges::find(*filtered_categories, category,
                            &FilteredCategoryInfo::name);
}

mojom::UserReactionType ToggleLikeUserReactionType(
    const mojom::UserReactionType user_reaction_type) {
  return user_reaction_type == mojom::UserReactionType::kLike
             ? mojom::UserReactionType::kNeutral
             : mojom::UserReactionType::kLike;
}

mojom::UserReactionType ToggleDislikeUserReactionType(
    const mojom::UserReactionType user_reaction_type) {
  return user_reaction_type == mojom::UserReactionType::kDislike
             ? mojom::UserReactionType::kNeutral
             : mojom::UserReactionType::kDislike;
}

uint64_t GenerateHash(const std::string& value) {
  return uint64_t{base::PersistentHash(value)};
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

ClientStateManager::ClientStateManager() = default;

ClientStateManager::~ClientStateManager() = default;

// static
ClientStateManager& ClientStateManager::GetInstance() {
  return GlobalState::GetInstance()->GetClientStateManager();
}

const FilteredAdvertiserList& ClientStateManager::GetFilteredAdvertisers()
    const {
  CHECK(is_initialized_);

  return client_.ad_preferences.filtered_advertisers;
}

const FilteredCategoryList& ClientStateManager::GetFilteredCategories() const {
  CHECK(is_initialized_);

  return client_.ad_preferences.filtered_categories;
}

const FlaggedAdList& ClientStateManager::GetFlaggedAds() const {
  CHECK(is_initialized_);

  return client_.ad_preferences.flagged_ads;
}

void ClientStateManager::Initialize(InitializeCallback callback) {
  Load(std::move(callback));
}

void ClientStateManager::AppendHistory(const HistoryItemInfo& history_item) {
#if !BUILDFLAG(IS_IOS)
  CHECK(is_initialized_);

  client_.history_items.push_front(history_item);

  const base::Time distant_past = base::Time::Now() - kHistoryTimeWindow;

  const auto iter =
      std::remove_if(client_.history_items.begin(), client_.history_items.end(),
                     [distant_past](const HistoryItemInfo& history_item) {
                       return history_item.created_at < distant_past;
                     });

  client_.history_items.erase(iter, client_.history_items.cend());

  Save();
#endif
}

const HistoryItemList& ClientStateManager::GetHistory() const {
  CHECK(is_initialized_);

  return client_.history_items;
}

void ClientStateManager::AppendToPurchaseIntentSignalHistoryForSegment(
    const std::string& segment,
    const PurchaseIntentSignalHistoryInfo& history) {
  CHECK(is_initialized_);

  if (client_.purchase_intent_signal_history.find(segment) ==
      client_.purchase_intent_signal_history.cend()) {
    client_.purchase_intent_signal_history.insert({segment, {}});
  }

  client_.purchase_intent_signal_history.at(segment).push_back(history);

  if (client_.purchase_intent_signal_history.at(segment).size() >
      kMaximumEntriesPerSegmentInPurchaseIntentSignalHistory) {
    client_.purchase_intent_signal_history.at(segment).pop_back();
  }

  Save();
}

const PurchaseIntentSignalHistoryMap&
ClientStateManager::GetPurchaseIntentSignalHistory() const {
  CHECK(is_initialized_);

  return client_.purchase_intent_signal_history;
}

mojom::UserReactionType ClientStateManager::ToggleLikeAd(
    const AdContentInfo& ad_content) {
  CHECK(is_initialized_);

  const auto iter = FindFilteredAdvertiser(
      ad_content.advertiser_id, &client_.ad_preferences.filtered_advertisers);
  if (iter != client_.ad_preferences.filtered_advertisers.cend()) {
    client_.ad_preferences.filtered_advertisers.erase(iter);
  }

  const mojom::UserReactionType user_reaction_type =
      ad_content.ToggleLikeUserReactionType();

  for (auto& item : client_.history_items) {
    if (item.ad_content.advertiser_id == ad_content.advertiser_id) {
      item.ad_content.user_reaction_type = user_reaction_type;
    }
  }

  Save();

  return user_reaction_type;
}

mojom::UserReactionType ClientStateManager::ToggleDislikeAd(
    const AdContentInfo& ad_content) {
  CHECK(is_initialized_);

  const mojom::UserReactionType user_reaction_type =
      ad_content.ToggleDislikeUserReactionType();

  const auto iter = FindFilteredAdvertiser(
      ad_content.advertiser_id, &client_.ad_preferences.filtered_advertisers);

  if (user_reaction_type == mojom::UserReactionType::kNeutral) {
    if (iter != client_.ad_preferences.filtered_advertisers.cend()) {
      client_.ad_preferences.filtered_advertisers.erase(iter);
    }
  } else {
    if (iter == client_.ad_preferences.filtered_advertisers.cend()) {
      FilteredAdvertiserInfo filtered_advertiser;
      filtered_advertiser.id = ad_content.advertiser_id;

      client_.ad_preferences.filtered_advertisers.push_back(
          filtered_advertiser);
    }
  }

  for (auto& item : client_.history_items) {
    if (item.ad_content.advertiser_id == ad_content.advertiser_id) {
      item.ad_content.user_reaction_type = user_reaction_type;
    }
  }

  Save();

  return user_reaction_type;
}

mojom::UserReactionType ClientStateManager::GetUserReactionTypeForAdvertiser(
    const std::string& advertiser_id) {
  const auto iter = base::ranges::find_if(
      client_.history_items,
      [&advertiser_id](const HistoryItemInfo& history_item) -> bool {
        return history_item.ad_content.advertiser_id == advertiser_id;
      });

  if (iter == client_.history_items.cend()) {
    return mojom::UserReactionType::kNeutral;
  }

  return iter->ad_content.user_reaction_type;
}

mojom::UserReactionType ClientStateManager::ToggleLikeCategory(
    const std::string& category,
    const mojom::UserReactionType user_reaction_type) {
  CHECK(is_initialized_);

  const auto iter = FindFilteredCategory(
      category, &client_.ad_preferences.filtered_categories);
  if (iter != client_.ad_preferences.filtered_categories.cend()) {
    client_.ad_preferences.filtered_categories.erase(iter);
  }

  const mojom::UserReactionType toggled_user_reaction_type =
      ToggleLikeUserReactionType(user_reaction_type);

  for (auto& item : client_.history_items) {
    if (item.category_content.category == category) {
      item.category_content.user_reaction_type = toggled_user_reaction_type;
    }
  }

  Save();

  return toggled_user_reaction_type;
}

mojom::UserReactionType ClientStateManager::ToggleDislikeCategory(
    const std::string& category,
    const mojom::UserReactionType user_reaction_type) {
  CHECK(is_initialized_);

  const mojom::UserReactionType toggled_user_reaction_type =
      ToggleDislikeUserReactionType(user_reaction_type);

  const auto iter = FindFilteredCategory(
      category, &client_.ad_preferences.filtered_categories);

  if (toggled_user_reaction_type == mojom::UserReactionType::kNeutral) {
    if (iter != client_.ad_preferences.filtered_categories.cend()) {
      client_.ad_preferences.filtered_categories.erase(iter);
    }
  } else {
    if (iter == client_.ad_preferences.filtered_categories.cend()) {
      FilteredCategoryInfo filtered_category;
      filtered_category.name = category;
      client_.ad_preferences.filtered_categories.push_back(filtered_category);
    }
  }

  for (auto& item : client_.history_items) {
    if (item.category_content.category == category) {
      item.category_content.user_reaction_type = toggled_user_reaction_type;
    }
  }

  Save();

  return toggled_user_reaction_type;
}

mojom::UserReactionType ClientStateManager::GetUserReactionTypeForSegment(
    const std::string& segment) {
  const auto iter = base::ranges::find_if(
      client_.history_items,
      [&segment](const HistoryItemInfo& history_item) -> bool {
        return history_item.category_content.category == segment;
      });

  if (iter == client_.history_items.cend()) {
    return mojom::UserReactionType::kNeutral;
  }

  return iter->category_content.user_reaction_type;
}

bool ClientStateManager::ToggleSaveAd(const AdContentInfo& ad_content) {
  CHECK(is_initialized_);

  const bool is_saved = !ad_content.is_saved;
  if (is_saved) {
    SavedAdInfo saved_ad;
    saved_ad.creative_instance_id = ad_content.creative_instance_id;
    client_.ad_preferences.saved_ads.push_back(saved_ad);
  } else {
    const auto iter = base::ranges::find(client_.ad_preferences.saved_ads,
                                         ad_content.creative_instance_id,
                                         &SavedAdInfo::creative_instance_id);

    if (iter != client_.ad_preferences.saved_ads.cend()) {
      client_.ad_preferences.saved_ads.erase(iter);
    }
  }

  for (auto& item : client_.history_items) {
    if (item.ad_content.creative_instance_id ==
        ad_content.creative_instance_id) {
      item.ad_content.is_saved = is_saved;
    }
  }

  Save();

  return is_saved;
}

bool ClientStateManager::ToggleMarkAdAsInappropriate(
    const AdContentInfo& ad_content) {
  CHECK(is_initialized_);

  const bool is_flagged = !ad_content.is_flagged;
  if (is_flagged) {
    FlaggedAdInfo flagged_ad;
    flagged_ad.creative_set_id = ad_content.creative_set_id;
    client_.ad_preferences.flagged_ads.push_back(flagged_ad);
  } else {
    client_.ad_preferences.flagged_ads.erase(
        base::ranges::remove(client_.ad_preferences.flagged_ads,
                             ad_content.creative_set_id,
                             &FlaggedAdInfo::creative_set_id),
        client_.ad_preferences.flagged_ads.end());
  }

  auto iter = base::ranges::find_if(
      client_.history_items, [&ad_content](const HistoryItemInfo& item) {
        return item.ad_content.creative_set_id == ad_content.creative_set_id;
      });
  if (iter != client_.history_items.end()) {
    iter->ad_content.is_flagged = is_flagged;
  }

  Save();

  return is_flagged;
}

void ClientStateManager::UpdateSeenAd(const AdInfo& ad) {
  CHECK(is_initialized_);

  const std::string type_as_string = ad.type.ToString();
  client_.seen_ads[type_as_string][ad.creative_instance_id] = true;
  client_.seen_advertisers[type_as_string][ad.advertiser_id] = true;
  Save();
}

const std::map<std::string, bool>& ClientStateManager::GetSeenAdsForType(
    const AdType& type) {
  CHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  return client_.seen_ads[type_as_string];
}

void ClientStateManager::ResetSeenAdsForType(const CreativeAdList& creative_ads,
                                             const AdType& type) {
  CHECK(is_initialized_);

  const std::string type_as_string = type.ToString();

  BLOG(1, "Resetting seen " << type_as_string << "s");

  for (const auto& creative_ad : creative_ads) {
    const auto iter =
        client_.seen_ads[type_as_string].find(creative_ad.creative_instance_id);
    if (iter != client_.seen_ads[type_as_string].cend()) {
      client_.seen_ads[type_as_string].erase(iter);
    }
  }

  Save();
}

void ClientStateManager::ResetAllSeenAdsForType(const AdType& type) {
  CHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  BLOG(1, "Resetting seen " << type_as_string << "s");
  client_.seen_ads[type_as_string] = {};
  Save();
}

const std::map<std::string, bool>&
ClientStateManager::GetSeenAdvertisersForType(const AdType& type) {
  CHECK(is_initialized_);

  return client_.seen_advertisers[type.ToString()];
}

void ClientStateManager::ResetSeenAdvertisersForType(
    const CreativeAdList& creative_ads,
    const AdType& type) {
  CHECK(is_initialized_);

  const std::string type_as_string = type.ToString();

  BLOG(1, "Resetting seen " << type_as_string << " advertisers");

  for (const auto& creative_ad : creative_ads) {
    const auto iter = client_.seen_advertisers[type_as_string].find(
        creative_ad.advertiser_id);
    if (iter != client_.seen_advertisers[type_as_string].cend()) {
      client_.seen_advertisers[type_as_string].erase(iter);
    }
  }

  Save();
}

void ClientStateManager::ResetAllSeenAdvertisersForType(const AdType& type) {
  CHECK(is_initialized_);

  const std::string type_as_string = type.ToString();
  BLOG(1, "Resetting seen " << type_as_string << " advertisers");
  client_.seen_advertisers[type_as_string] = {};
  Save();
}

void ClientStateManager::AppendTextClassificationProbabilitiesToHistory(
    const TextClassificationProbabilityMap& probabilities) {
  CHECK(is_initialized_);

  client_.text_classification_probabilities.push_front(probabilities);

  const size_t maximum_entries =
      kTextClassificationPageProbabilitiesHistorySize.Get();
  if (client_.text_classification_probabilities.size() > maximum_entries) {
    client_.text_classification_probabilities.resize(maximum_entries);
  }

  Save();
}

const TextClassificationProbabilityList&
ClientStateManager::GetTextClassificationProbabilitiesHistory() const {
  CHECK(is_initialized_);

  return client_.text_classification_probabilities;
}

void ClientStateManager::RemoveAllHistory() {
  CHECK(is_initialized_);

  BLOG(1, "Successfully reset client state");

  client_ = {};

  Save();
}

///////////////////////////////////////////////////////////////////////////////

void ClientStateManager::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving client state");

  const std::string json = client_.ToJson();

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
      base::BindOnce(&ClientStateManager::OnLoaded, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void ClientStateManager::OnLoaded(InitializeCallback callback,
                                  const bool success,
                                  const std::string& json) {
  if (!success) {
    BLOG(3, "Client state does not exist, creating default state");

    is_initialized_ = true;

    client_ = {};
    Save();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load client state");

      BLOG(3, "Failed to parse client state: " << json);

      return std::move(callback).Run(/*success*/ false);
    }

    BLOG(3, "Successfully loaded client state");

    is_initialized_ = true;
  }

  is_mutated_ = IsMutated(client_.ToJson());
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

  client_ = client;

  return true;
}

}  // namespace brave_ads
