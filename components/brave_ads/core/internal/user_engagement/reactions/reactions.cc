/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

#include <utility>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_type_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_value_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

Reactions::Reactions() {
  Load();
}

Reactions::~Reactions() = default;

void Reactions::ToggleLikeAd(mojom::ReactionInfoPtr mojom_reaction,
                             ToggleReactionCallback callback) {
  if (!mojom_reaction) {
    return std::move(callback).Run(/*success=*/false);
  }

  const std::string advertiser_id = mojom_reaction->advertiser_id;
  const mojom::ReactionType mojom_reaction_type =
      AdReactionTypeForId(advertiser_id);
  const mojom::ReactionType toggled_mojom_reaction_type =
      ToggleLikedReactionType(mojom_reaction_type);
  if (toggled_mojom_reaction_type == mojom::ReactionType::kNeutral) {
    // Neutral reaction, so remove it.
    ad_reactions_.erase(advertiser_id);
  } else {
    // Non-neutral reaction, so set it.
    ad_reactions_[advertiser_id] = toggled_mojom_reaction_type;
  }
  SetProfileDictPref(prefs::kAdReactions, ReactionMapToDict(ad_reactions_));

  if (toggled_mojom_reaction_type == mojom::ReactionType::kLiked) {
    NotifyDidLikeAd(mojom_reaction->advertiser_id);

    Deposit(&*mojom_reaction, mojom::ConfirmationType::kLikedAd);
  }

  std::move(callback).Run(/*success=*/true);
}

void Reactions::ToggleDislikeAd(mojom::ReactionInfoPtr mojom_reaction,
                                ToggleReactionCallback callback) {
  if (!mojom_reaction) {
    return std::move(callback).Run(/*success=*/false);
  }

  const std::string advertiser_id = mojom_reaction->advertiser_id;
  const mojom::ReactionType mojom_reaction_type =
      AdReactionTypeForId(advertiser_id);
  const mojom::ReactionType toggled_mojom_reaction_type =
      ToggleDislikedReactionType(mojom_reaction_type);
  if (toggled_mojom_reaction_type == mojom::ReactionType::kNeutral) {
    // Neutral reaction, so remove it.
    ad_reactions_.erase(advertiser_id);
  } else {
    // Non-neutral reaction, so set it.
    ad_reactions_[advertiser_id] = toggled_mojom_reaction_type;
  }
  SetProfileDictPref(prefs::kAdReactions, ReactionMapToDict(ad_reactions_));

  if (toggled_mojom_reaction_type == mojom::ReactionType::kDisliked) {
    NotifyDidDislikeAd(mojom_reaction->advertiser_id);

    Deposit(&*mojom_reaction, mojom::ConfirmationType::kDislikedAd);
  }

  std::move(callback).Run(/*success=*/true);
}

mojom::ReactionType Reactions::AdReactionTypeForId(
    const std::string& advertiser_id) const {
  const auto iter = ad_reactions_.find(advertiser_id);
  if (iter == ad_reactions_.cend()) {
    // No reaction, so neutral.
    return mojom::ReactionType::kNeutral;
  }
  // Reaction found.
  const auto& [_, reaction_type] = *iter;
  return reaction_type;
}

void Reactions::ToggleLikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                                  ToggleReactionCallback callback) {
  if (!mojom_reaction) {
    return std::move(callback).Run(/*success=*/false);
  }

  const std::string segment = mojom_reaction->segment;
  const mojom::ReactionType mojom_reaction_type =
      SegmentReactionTypeForId(segment);
  const mojom::ReactionType toggled_mojom_reaction_type =
      ToggleLikedReactionType(mojom_reaction_type);
  if (toggled_mojom_reaction_type == mojom::ReactionType::kNeutral) {
    // Neutral reaction, so remove it.
    segment_reactions_.erase(segment);
  } else {
    // Non-neutral reaction, so set it.
    segment_reactions_[segment] = toggled_mojom_reaction_type;
  }
  SetProfileDictPref(prefs::kSegmentReactions,
                     ReactionMapToDict(segment_reactions_));

  if (toggled_mojom_reaction_type == mojom::ReactionType::kLiked) {
    NotifyDidLikeSegment(mojom_reaction->segment);
  }

  std::move(callback).Run(/*success=*/true);
}

void Reactions::ToggleDislikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                                     ToggleReactionCallback callback) {
  if (!mojom_reaction) {
    return std::move(callback).Run(/*success=*/false);
  }

  const std::string segment = mojom_reaction->segment;
  const mojom::ReactionType mojom_reaction_type =
      SegmentReactionTypeForId(segment);
  const mojom::ReactionType toggled_mojom_reaction_type =
      ToggleDislikedReactionType(mojom_reaction_type);
  if (toggled_mojom_reaction_type == mojom::ReactionType::kNeutral) {
    // Neutral reaction, so remove it.
    segment_reactions_.erase(segment);
  } else {
    // Non-neutral reaction, so set it.
    segment_reactions_[segment] = toggled_mojom_reaction_type;
  }
  SetProfileDictPref(prefs::kSegmentReactions,
                     ReactionMapToDict(segment_reactions_));

  if (toggled_mojom_reaction_type == mojom::ReactionType::kDisliked) {
    NotifyDidDislikeSegment(mojom_reaction->segment);
  }

  std::move(callback).Run(/*success=*/true);
}

mojom::ReactionType Reactions::SegmentReactionTypeForId(
    const std::string& segment) const {
  const auto iter = segment_reactions_.find(segment);
  if (iter == segment_reactions_.cend()) {
    // No reaction, so neutral.
    return mojom::ReactionType::kNeutral;
  }
  const auto [_, reaction_type] = *iter;
  return reaction_type;
}

void Reactions::ToggleSaveAd(mojom::ReactionInfoPtr mojom_reaction,
                             ToggleReactionCallback callback) {
  if (!mojom_reaction) {
    return std::move(callback).Run(/*success=*/false);
  }

  const auto [iter, inserted] =
      saved_ads_.insert(mojom_reaction->creative_instance_id);
  if (!inserted) {
    // Already saved, so unsave it.
    saved_ads_.erase(iter);
  }
  SetProfileListPref(prefs::kSaveAds, ReactionSetToList(saved_ads_));

  if (inserted) {
    NotifyDidToggleSaveAd(mojom_reaction->creative_instance_id);

    Deposit(&*mojom_reaction, mojom::ConfirmationType::kSavedAd);
  }

  std::move(callback).Run(/*success=*/true);
}

bool Reactions::IsAdSaved(const std::string& creative_instance_id) const {
  return base::Contains(saved_ads_, creative_instance_id);
}

void Reactions::ToggleMarkAdAsInappropriate(
    mojom::ReactionInfoPtr mojom_reaction,
    ToggleReactionCallback callback) {
  if (!mojom_reaction) {
    return std::move(callback).Run(/*success=*/false);
  }

  const auto [iter, inserted] =
      marked_as_inappropriate_.insert(mojom_reaction->creative_set_id);
  if (!inserted) {
    // Already marked as inappropriate, so unmark it.
    marked_as_inappropriate_.erase(iter);
  }
  SetProfileListPref(prefs::kMarkedAsInappropriate,
                     ReactionSetToList(marked_as_inappropriate_));

  if (inserted) {
    NotifyDidToggleSaveAd(mojom_reaction->creative_set_id);

    Deposit(&*mojom_reaction, mojom::ConfirmationType::kMarkAdAsInappropriate);
  }

  std::move(callback).Run(/*success=*/true);
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
void Reactions::Deposit(const mojom::ReactionInfo* const mojom_reaction,
                        const mojom::ConfirmationType mojom_confirmation_type) {
  CHECK(mojom_reaction);

  GetAccount().Deposit(mojom_reaction->creative_instance_id,
                       mojom_reaction->segment, mojom_reaction->mojom_ad_type,
                       mojom_confirmation_type);
}

void Reactions::NotifyDidLikeAd(const std::string& advertiser_id) const {
  for (ReactionsObserver& observer : observers_) {
    observer.OnDidLikeAd(advertiser_id);
  }
}

void Reactions::NotifyDidDislikeAd(const std::string& advertiser_id) const {
  for (ReactionsObserver& observer : observers_) {
    observer.OnDidDislikeAd(advertiser_id);
  }
}

void Reactions::NotifyDidLikeSegment(const std::string& segment) const {
  for (ReactionsObserver& observer : observers_) {
    observer.OnDidLikeSegment(segment);
  }
}

void Reactions::NotifyDidDislikeSegment(const std::string& segment) const {
  for (ReactionsObserver& observer : observers_) {
    observer.OnDidDislikeSegment(segment);
  }
}

void Reactions::NotifyDidToggleSaveAd(
    const std::string& creative_instance_id) const {
  for (ReactionsObserver& observer : observers_) {
    observer.OnDidToggleSaveAd(creative_instance_id);
  }
}

void Reactions::NotifyDidToggleMarkAdAsInappropriate(
    const std::string& creative_set) const {
  for (ReactionsObserver& observer : observers_) {
    observer.OnDidToggleMarkAdAsInappropriate(creative_set);
  }
}

}  // namespace brave_ads
