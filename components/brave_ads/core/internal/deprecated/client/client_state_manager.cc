/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "build/build_config.h"  // IWYU pragma: keep

namespace brave_ads {

namespace {

constexpr size_t kMaximumPurchaseIntentSignalHistoryEntriesPerSegment = 100;

FilteredAdvertiserList::iterator FindFilteredAdvertiser(
    const std::string& advertiser_id,
    FilteredAdvertiserList* const filtered_advertisers) {
  CHECK(!advertiser_id.empty());
  CHECK(filtered_advertisers);

  return base::ranges::find(*filtered_advertisers, advertiser_id,
                            &FilteredAdvertiserInfo::id);
}

FilteredCategoryList::iterator FindFilteredCategory(
    const std::string& category,
    FilteredCategoryList* const filtered_categories) {
  CHECK(!category.empty());
  CHECK(filtered_categories);

  return base::ranges::find(*filtered_categories, category,
                            &FilteredCategoryInfo::name);
}

mojom::ReactionType ToggleLikeReactionType(
    const mojom::ReactionType reaction_type) {
  return reaction_type == mojom::ReactionType::kLiked
             ? mojom::ReactionType::kNeutral
             : mojom::ReactionType::kLiked;
}

mojom::ReactionType ToggleDislikeReactionType(
    const mojom::ReactionType reaction_type) {
  return reaction_type == mojom::ReactionType::kDisliked
             ? mojom::ReactionType::kNeutral
             : mojom::ReactionType::kDisliked;
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

void ClientStateManager::LoadState(InitializeCallback callback) {
  BLOG(3, "Loading client state");

  Load(kClientJsonFilename,
       base::BindOnce(&ClientStateManager::LoadCallback,
                      weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ClientStateManager::AppendAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) {
#if !BUILDFLAG(IS_IOS)
  CHECK(is_initialized_);

  client_.ad_history.push_front(ad_history_item);

  const base::Time distant_past =
      base::Time::Now() - kAdHistoryRetentionPeriod.Get();

  base::EraseIf(client_.ad_history,
                [distant_past](const AdHistoryItemInfo& ad_history_item) {
                  return ad_history_item.created_at < distant_past;
                });

  SaveState();
#endif
}

const AdHistoryList& ClientStateManager::GetAdHistory() const {
  CHECK(is_initialized_);

  return client_.ad_history;
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
      kMaximumPurchaseIntentSignalHistoryEntriesPerSegment) {
    client_.purchase_intent_signal_history.at(segment).pop_back();
  }

  SaveState();
}

const PurchaseIntentSignalHistoryMap&
ClientStateManager::GetPurchaseIntentSignalHistory() const {
  CHECK(is_initialized_);

  return client_.purchase_intent_signal_history;
}

mojom::ReactionType ClientStateManager::ToggleLikeAd(
    const AdHistoryItemInfo& ad_history_item) {
  CHECK(is_initialized_);

  const auto iter =
      FindFilteredAdvertiser(ad_history_item.advertiser_id,
                             &client_.ad_preferences.filtered_advertisers);
  if (iter != client_.ad_preferences.filtered_advertisers.cend()) {
    client_.ad_preferences.filtered_advertisers.erase(iter);
  }

  const mojom::ReactionType toggled_reaction_type =
      ToggleLikeReactionType(ad_history_item.ad_reaction_type);

  for (auto& item : client_.ad_history) {
    if (item.advertiser_id == ad_history_item.advertiser_id) {
      item.ad_reaction_type = toggled_reaction_type;
    }
  }

  SaveState();

  return toggled_reaction_type;
}

mojom::ReactionType ClientStateManager::ToggleDislikeAd(
    const AdHistoryItemInfo& ad_history_item) {
  CHECK(is_initialized_);

  const mojom::ReactionType toggled_reaction_type =
      ToggleDislikeReactionType(ad_history_item.ad_reaction_type);

  const auto iter =
      FindFilteredAdvertiser(ad_history_item.advertiser_id,
                             &client_.ad_preferences.filtered_advertisers);

  if (toggled_reaction_type == mojom::ReactionType::kNeutral) {
    if (iter != client_.ad_preferences.filtered_advertisers.cend()) {
      client_.ad_preferences.filtered_advertisers.erase(iter);
    }
  } else {
    if (iter == client_.ad_preferences.filtered_advertisers.cend()) {
      FilteredAdvertiserInfo filtered_advertiser;
      filtered_advertiser.id = ad_history_item.advertiser_id;

      client_.ad_preferences.filtered_advertisers.push_back(
          filtered_advertiser);
    }
  }

  for (auto& item : client_.ad_history) {
    if (item.advertiser_id == ad_history_item.advertiser_id) {
      item.ad_reaction_type = toggled_reaction_type;
    }
  }

  SaveState();

  return toggled_reaction_type;
}

mojom::ReactionType ClientStateManager::GetReactionTypeForAd(const AdInfo& ad) {
  const auto iter = base::ranges::find_if(
      client_.ad_history,
      [&ad](const AdHistoryItemInfo& ad_history_item) -> bool {
        return ad_history_item.advertiser_id == ad.advertiser_id;
      });

  if (iter == client_.ad_history.cend()) {
    return mojom::ReactionType::kNeutral;
  }

  return iter->ad_reaction_type;
}

mojom::ReactionType ClientStateManager::ToggleLikeSegment(
    const AdHistoryItemInfo& ad_history_item) {
  CHECK(is_initialized_);

  const auto iter = FindFilteredCategory(
      ad_history_item.segment, &client_.ad_preferences.filtered_categories);
  if (iter != client_.ad_preferences.filtered_categories.cend()) {
    client_.ad_preferences.filtered_categories.erase(iter);
  }

  const mojom::ReactionType toggled_reaction_type =
      ToggleLikeReactionType(ad_history_item.segment_reaction_type);

  for (auto& item : client_.ad_history) {
    if (item.segment == ad_history_item.segment) {
      item.segment_reaction_type = toggled_reaction_type;
    }
  }

  SaveState();

  return toggled_reaction_type;
}

mojom::ReactionType ClientStateManager::ToggleDislikeSegment(
    const AdHistoryItemInfo& ad_history_item) {
  CHECK(is_initialized_);

  const mojom::ReactionType toggled_reaction_type =
      ToggleDislikeReactionType(ad_history_item.segment_reaction_type);

  const auto iter = FindFilteredCategory(
      ad_history_item.segment, &client_.ad_preferences.filtered_categories);

  if (toggled_reaction_type == mojom::ReactionType::kNeutral) {
    if (iter != client_.ad_preferences.filtered_categories.cend()) {
      client_.ad_preferences.filtered_categories.erase(iter);
    }
  } else {
    if (iter == client_.ad_preferences.filtered_categories.cend()) {
      FilteredCategoryInfo filtered_category;
      filtered_category.name = ad_history_item.segment;
      client_.ad_preferences.filtered_categories.push_back(filtered_category);
    }
  }

  for (auto& item : client_.ad_history) {
    if (item.segment == ad_history_item.segment) {
      item.segment_reaction_type = toggled_reaction_type;
    }
  }

  SaveState();

  return toggled_reaction_type;
}

mojom::ReactionType ClientStateManager::GetReactionTypeForSegment(
    const std::string& segment) {
  const auto iter = base::ranges::find_if(
      client_.ad_history,
      [&segment](const AdHistoryItemInfo& ad_history_item) -> bool {
        return ad_history_item.segment == segment;
      });

  if (iter == client_.ad_history.cend()) {
    return mojom::ReactionType::kNeutral;
  }

  return iter->segment_reaction_type;
}

bool ClientStateManager::ToggleSaveAd(
    const AdHistoryItemInfo& ad_history_item) {
  CHECK(is_initialized_);

  const bool is_saved = !ad_history_item.is_saved;
  if (is_saved) {
    SavedAdInfo saved_ad;
    saved_ad.creative_instance_id = ad_history_item.creative_instance_id;
    client_.ad_preferences.saved_ads.push_back(saved_ad);
  } else {
    const auto iter = base::ranges::find(client_.ad_preferences.saved_ads,
                                         ad_history_item.creative_instance_id,
                                         &SavedAdInfo::creative_instance_id);

    if (iter != client_.ad_preferences.saved_ads.cend()) {
      client_.ad_preferences.saved_ads.erase(iter);
    }
  }

  for (auto& item : client_.ad_history) {
    if (item.creative_instance_id == ad_history_item.creative_instance_id) {
      item.is_saved = is_saved;
    }
  }

  SaveState();

  return is_saved;
}

bool ClientStateManager::ToggleMarkAdAsInappropriate(
    const AdHistoryItemInfo& ad_history_item) {
  CHECK(is_initialized_);

  const bool is_marked_as_inappropriate =
      !ad_history_item.is_marked_as_inappropriate;
  if (is_marked_as_inappropriate) {
    FlaggedAdInfo flagged_ad;
    flagged_ad.creative_set_id = ad_history_item.creative_set_id;
    client_.ad_preferences.flagged_ads.push_back(flagged_ad);
  } else {
    client_.ad_preferences.flagged_ads.erase(
        base::ranges::remove(client_.ad_preferences.flagged_ads,
                             ad_history_item.creative_set_id,
                             &FlaggedAdInfo::creative_set_id),
        client_.ad_preferences.flagged_ads.cend());
  }

  const std::string creative_set_id = ad_history_item.creative_set_id;
  const auto iter = base::ranges::find_if(
      client_.ad_history,
      [&creative_set_id](const AdHistoryItemInfo& ad_history_item) {
        return ad_history_item.creative_set_id == creative_set_id;
      });

  if (iter != client_.ad_history.cend()) {
    iter->is_marked_as_inappropriate = is_marked_as_inappropriate;
  }

  SaveState();

  return is_marked_as_inappropriate;
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

  SaveState();
}

const TextClassificationProbabilityList&
ClientStateManager::GetTextClassificationProbabilitiesHistory() const {
  CHECK(is_initialized_);

  return client_.text_classification_probabilities;
}

///////////////////////////////////////////////////////////////////////////////

void ClientStateManager::SaveState() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving client state");

  Save(kClientJsonFilename, client_.ToJson(),
       base::BindOnce([](const bool success) {
         if (!success) {
           // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
           // potential defects using `DumpWithoutCrashing`.
           SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                     "Failed to save client state");
           base::debug::DumpWithoutCrashing();

           return BLOG(0, "Failed to save client state");
         }

         BLOG(9, "Successfully saved client state");
       }));
}

void ClientStateManager::LoadCallback(InitializeCallback callback,
                                      const std::optional<std::string>& json) {
  if (!json) {
    BLOG(3, "Client state does not exist, creating default state");

    is_initialized_ = true;
    client_ = {};

    SaveState();
  } else {
    if (!FromJson(*json)) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
      // potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Failed to parse client state");
      base::debug::DumpWithoutCrashing();

      BLOG(1, "Failed to parse client state: " << *json);

      return std::move(callback).Run(/*success=*/false);
    }

    BLOG(3, "Successfully loaded client state");

    is_initialized_ = true;
  }

  std::move(callback).Run(/*success=*/true);
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
