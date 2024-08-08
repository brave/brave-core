/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_H_

#include <map>
#include <set>
#include <string>

#include "brave/components/brave_ads/core/internal/history/ad_history_manager_observer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads {

// `id` can be either `advertiser_id` for liking/disliking an ad or `segment`
// for liking/disliking a segment.
using ReactionMap = std::map</*id*/ std::string, mojom::ReactionType>;

// `id` can be either `creative_instance_id` for saving an ad or
// `creative_set_id` for marking an ad as inappropriate.
using ReactionSet = std::set</*id*/ std::string>;

struct AdHistoryItemInfo;

class Reactions final : public AdHistoryManagerObserver {
 public:
  Reactions();

  Reactions(const Reactions&) = delete;
  Reactions& operator=(const Reactions&) = delete;

  Reactions(Reactions&&) noexcept = delete;
  Reactions& operator=(Reactions&&) noexcept = delete;

  ~Reactions() override;

  void ToggleLikeAd(const std::string& advertiser_id);
  void ToggleDislikeAd(const std::string& advertiser_id);
  mojom::ReactionType AdReactionTypeForId(
      const std::string& advertiser_id) const;
  const ReactionMap& Ads() const { return ad_reactions_; }

  void ToggleLikeSegment(const std::string& segment);
  void ToggleDislikeSegment(const std::string& segment);
  mojom::ReactionType SegmentReactionTypeForId(
      const std::string& segment) const;
  const ReactionMap& Segments() const { return segment_reactions_; }

  void ToggleSaveAd(const std::string& creative_instance_id);
  bool IsAdSaved(const std::string& creative_instance_id) const;

  void ToggleMarkAdAsInappropriate(const std::string& creative_set_id);
  bool IsAdMarkedAsInappropriate(const std::string& creative_set_id) const;

 private:
  void LoadAdReactions();
  void LoadSegmentReactions();
  void LoadSavedAds();
  void LoadMarkedAsInappropriate();
  void Load();

  static void Deposit(const AdHistoryItemInfo& ad_history_item,
                      ConfirmationType confirmation_type);

  // AdHistoryManagerObserver:
  void OnDidLikeAd(const AdHistoryItemInfo& ad_history_item) override;
  void OnDidDislikeAd(const AdHistoryItemInfo& ad_history_item) override;
  void OnDidLikeSegment(const AdHistoryItemInfo& ad_history_item) override;
  void OnDidDislikeSegment(const AdHistoryItemInfo& ad_history_item) override;
  void OnDidToggleSaveAd(const AdHistoryItemInfo& ad_history_item) override;
  void OnDidToggleMarkAdAsInappropriate(
      const AdHistoryItemInfo& ad_history_item) override;

  ReactionMap ad_reactions_;
  ReactionMap segment_reactions_;
  ReactionSet saved_ads_;
  ReactionSet marked_as_inappropriate_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_H_
