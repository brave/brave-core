/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

#include "base/containers/contains.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_type_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_value_util.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

Reactions::Reactions() {
  AdHistoryManager::GetInstance().AddObserver(this);

  Load();
}

Reactions::~Reactions() {
  AdHistoryManager::GetInstance().RemoveObserver(this);
}

void Reactions::ToggleLikeAd(const std::string& advertiser_id) {
  const mojom::ReactionType mojom_reaction_type =
      AdReactionTypeForId(advertiser_id);
  const mojom::ReactionType toggled_mojom_reaction_type =
      ToggleLikedReactionType(mojom_reaction_type);
  if (toggled_mojom_reaction_type == mojom::ReactionType::kNeutral) {
    ad_reactions_.erase(advertiser_id);
  } else {
    ad_reactions_[advertiser_id] = toggled_mojom_reaction_type;
  }

  SetProfileDictPref(prefs::kAdReactions, ReactionMapToDict(ad_reactions_));
}

void Reactions::ToggleDislikeAd(const std::string& advertiser_id) {
  const mojom::ReactionType mojom_reaction_type =
      AdReactionTypeForId(advertiser_id);
  const mojom::ReactionType toggled_mojom_reaction_type =
      ToggleDislikedReactionType(mojom_reaction_type);
  if (toggled_mojom_reaction_type == mojom::ReactionType::kNeutral) {
    ad_reactions_.erase(advertiser_id);
  } else {
    ad_reactions_[advertiser_id] = toggled_mojom_reaction_type;
  }

  SetProfileDictPref(prefs::kAdReactions, ReactionMapToDict(ad_reactions_));
}

mojom::ReactionType Reactions::AdReactionTypeForId(
    const std::string& advertiser_id) const {
  const auto iter = ad_reactions_.find(advertiser_id);
  if (iter == ad_reactions_.cend()) {
    return mojom::ReactionType::kNeutral;
  }
  const auto [_, reaction_type] = *iter;
  return reaction_type;
}

void Reactions::ToggleLikeSegment(const std::string& segment) {
  const mojom::ReactionType mojom_reaction_type =
      SegmentReactionTypeForId(segment);
  const mojom::ReactionType toggled_mojom_reaction_type =
      ToggleLikedReactionType(mojom_reaction_type);
  if (toggled_mojom_reaction_type == mojom::ReactionType::kNeutral) {
    segment_reactions_.erase(segment);
  } else {
    segment_reactions_[segment] = toggled_mojom_reaction_type;
  }

  SetProfileDictPref(prefs::kSegmentReactions,
                     ReactionMapToDict(segment_reactions_));
}

void Reactions::ToggleDislikeSegment(const std::string& segment) {
  const mojom::ReactionType mojom_reaction_type =
      SegmentReactionTypeForId(segment);
  const mojom::ReactionType toggled_mojom_reaction_type =
      ToggleDislikedReactionType(mojom_reaction_type);
  if (toggled_mojom_reaction_type == mojom::ReactionType::kNeutral) {
    segment_reactions_.erase(segment);
  } else {
    segment_reactions_[segment] = toggled_mojom_reaction_type;
  }

  SetProfileDictPref(prefs::kSegmentReactions,
                     ReactionMapToDict(segment_reactions_));
}

mojom::ReactionType Reactions::SegmentReactionTypeForId(
    const std::string& segment) const {
  const auto iter = segment_reactions_.find(segment);
  if (iter == segment_reactions_.cend()) {
    return mojom::ReactionType::kNeutral;
  }
  const auto [_, reaction_type] = *iter;
  return reaction_type;
}

void Reactions::ToggleSaveAd(const std::string& creative_instance_id) {
  const auto [iterator, inserted] = saved_ads_.insert(creative_instance_id);
  if (!inserted) {
    saved_ads_.erase(iterator);
  }

  SetProfileListPref(prefs::kSaveAds, ReactionSetToList(saved_ads_));
}

bool Reactions::IsAdSaved(const std::string& creative_instance_id) const {
  return base::Contains(saved_ads_, creative_instance_id);
}

void Reactions::ToggleMarkAdAsInappropriate(
    const std::string& creative_set_id) {
  const auto [iterator, inserted] =
      marked_as_inappropriate_.insert(creative_set_id);
  if (!inserted) {
    marked_as_inappropriate_.erase(iterator);
  }

  SetProfileListPref(prefs::kMarkedAsInappropriate,
                     ReactionSetToList(marked_as_inappropriate_));
}

bool Reactions::IsAdMarkedAsInappropriate(
    const std::string& creative_set_id) const {
  return base::Contains(marked_as_inappropriate_, creative_set_id);
}

///////////////////////////////////////////////////////////////////////////////

void Reactions::LoadAdReactions() {
  if (const std::optional<base::Value::Dict> dict =
          GetProfileDictPref(prefs::kAdReactions)) {
    ad_reactions_ = ReactionMapFromDict(*dict);
  }
}

void Reactions::LoadSegmentReactions() {
  if (const std::optional<base::Value::Dict> dict =
          GetProfileDictPref(prefs::kSegmentReactions)) {
    segment_reactions_ = ReactionMapFromDict(*dict);
  }
}

void Reactions::LoadSavedAds() {
  if (const std::optional<base::Value::List> list =
          GetProfileListPref(prefs::kSaveAds)) {
    saved_ads_ = ReactionSetFromList(*list);
  }
}

void Reactions::LoadMarkedAsInappropriate() {
  if (const std::optional<base::Value::List> list =
          GetProfileListPref(prefs::kMarkedAsInappropriate)) {
    marked_as_inappropriate_ = ReactionSetFromList(*list);
  }
}

void Reactions::Load() {
  LoadAdReactions();
  LoadSegmentReactions();
  LoadSavedAds();
  LoadMarkedAsInappropriate();
}

// static
void Reactions::Deposit(const AdHistoryItemInfo& ad_history_item,
                        const ConfirmationType confirmation_type) {
  GetAccount().Deposit(ad_history_item.creative_instance_id,
                       ad_history_item.segment, ad_history_item.type,
                       confirmation_type);
}

void Reactions::OnDidLikeAd(const AdHistoryItemInfo& ad_history_item) {
  ToggleLikeAd(ad_history_item.advertiser_id);

  Deposit(ad_history_item, ConfirmationType::kLikedAd);
}

void Reactions::OnDidDislikeAd(const AdHistoryItemInfo& ad_history_item) {
  ToggleDislikeAd(ad_history_item.advertiser_id);

  Deposit(ad_history_item, ConfirmationType::kDislikedAd);
}

void Reactions::OnDidLikeSegment(const AdHistoryItemInfo& ad_history_item) {
  ToggleLikeSegment(ad_history_item.segment);
}

void Reactions::OnDidDislikeSegment(const AdHistoryItemInfo& ad_history_item) {
  ToggleDislikeSegment(ad_history_item.segment);
}

void Reactions::OnDidToggleSaveAd(const AdHistoryItemInfo& ad_history_item) {
  ToggleSaveAd(ad_history_item.creative_instance_id);

  Deposit(ad_history_item, ConfirmationType::kSavedAd);
}

void Reactions::OnDidToggleMarkAdAsInappropriate(
    const AdHistoryItemInfo& ad_history_item) {
  ToggleMarkAdAsInappropriate(ad_history_item.creative_set_id);

  Deposit(ad_history_item, ConfirmationType::kMarkAdAsInappropriate);
}

}  // namespace brave_ads
