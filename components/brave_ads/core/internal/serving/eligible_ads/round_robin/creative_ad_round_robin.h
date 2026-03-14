/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ROUND_ROBIN_CREATIVE_AD_ROUND_ROBIN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ROUND_ROBIN_CREATIVE_AD_ROUND_ROBIN_H_

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"

namespace brave_ads {

// Ensures every ad is shown once per rotation before any repeats, and prevents
// the most recently seen ad from being the first shown in the next rotation
// when possible. State is intentionally kept in memory only and not persisted.
class CreativeAdRoundRobin {
 public:
  CreativeAdRoundRobin();

  CreativeAdRoundRobin(const CreativeAdRoundRobin&) = delete;
  CreativeAdRoundRobin& operator=(const CreativeAdRoundRobin&) = delete;

  ~CreativeAdRoundRobin();

  // Filters out seen `creative_ads`, resetting the rotation when all eligible
  // ads have been shown. On reset, excludes the last seen ad to avoid an
  // immediate repeat, unless it would leave no ads to serve.
  template <typename T>
  void Filter(std::vector<T>& creative_ads) {
    if (!kShouldRoundRobin.Get()) {
      return;
    }

    if (creative_ads.empty()) {
      // Filtering an empty ad list indicates that no campaigns are active,
      // so the round-robin state should reset.
      seen_creative_instance_ids_.clear();
      return;
    }

    // Check if all creative ads have already been shown.
    if (std::ranges::all_of(creative_ads, [&](const T& creative_ad) {
          return seen_creative_instance_ids_.contains(
              creative_ad.creative_instance_id);
        })) {
      // All ads have been seen, so reset for the next round, including any
      // creative instance IDs from removed campaigns.
      seen_creative_instance_ids_.clear();

      if (last_seen_creative_instance_id_ && creative_ads.size() > 1) {
        // Exclude the last seen creative ad to avoid an immediate repeat if
        // there is more than one creative ad.
        seen_creative_instance_ids_.insert(*last_seen_creative_instance_id_);
      }
    }

    // Remove seen ads from the eligible ads for this serving round.
    std::erase_if(creative_ads, [&](const T& creative_ad) {
      return seen_creative_instance_ids_.contains(
          creative_ad.creative_instance_id);
    });
  }

  // Marks a creative ad as seen to avoid repeats within a rotation.
  template <typename T>
  void MarkAsSeen(const T& creative_ad) {
    if (!kShouldRoundRobin.Get()) {
      return;
    }

    seen_creative_instance_ids_.insert(creative_ad.creative_instance_id);
    last_seen_creative_instance_id_ = creative_ad.creative_instance_id;
  }

 private:
  base::flat_set<std::string> seen_creative_instance_ids_;
  std::optional<std::string> last_seen_creative_instance_id_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ROUND_ROBIN_CREATIVE_AD_ROUND_ROBIN_H_
