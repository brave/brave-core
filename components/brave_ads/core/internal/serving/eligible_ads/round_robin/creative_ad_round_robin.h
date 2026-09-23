/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ROUND_ROBIN_CREATIVE_AD_ROUND_ROBIN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ROUND_ROBIN_CREATIVE_AD_ROUND_ROBIN_H_

#include <algorithm>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace brave_ads {

// Ensures every ad within a priority bucket is served once per rotation
// before any repeats. Each priority bucket rotates independently so that a
// higher priority bucket's rotation cannot affect a lower priority bucket's
// eligibility. State is intentionally kept in memory only and not persisted.
class CreativeAdRoundRobin final {
 public:
  CreativeAdRoundRobin();

  CreativeAdRoundRobin(const CreativeAdRoundRobin&) = delete;
  CreativeAdRoundRobin& operator=(const CreativeAdRoundRobin&) = delete;

  ~CreativeAdRoundRobin();

  // Filters out served `creative_ads`, resetting the rotation for their
  // priority bucket when all creatives in the current set have been served.
  // `creative_ads` must all share the same `priority`, as is the case when
  // called with a single bucket from `PrioritizedCreativeAdBuckets`.
  template <typename T>
  void Filter(std::vector<T>& creative_ads) {
    if (!kShouldRoundRobin.Get() || creative_ads.empty()) {
      return;
    }

    absl::flat_hash_set<std::string>& served_creative_instance_ids =
        served_creative_instance_ids_by_priority_[creative_ads.front()
                                                      .priority];

    // Check if all creative ads have been served.
    if (std::ranges::all_of(creative_ads, [&served_creative_instance_ids](
                                              const T& creative_ad) {
          return served_creative_instance_ids.contains(
              creative_ad.creative_instance_id);
        })) {
      // All ads have been served, so reset for the next round, including any
      // creative instance IDs from removed campaigns.
      served_creative_instance_ids.clear();
    }

    // Remove served ads from the eligible ads for this serving round.
    std::erase_if(creative_ads,
                  [&served_creative_instance_ids](const T& creative_ad) {
                    return served_creative_instance_ids.contains(
                        creative_ad.creative_instance_id);
                  });
  }

  // Marks a creative ad as served within its priority bucket to avoid
  // repeats within a rotation.
  template <typename T>
  void MarkAsServed(const T& creative_ad) {
    if (!kShouldRoundRobin.Get()) {
      return;
    }

    served_creative_instance_ids_by_priority_[creative_ad.priority].insert(
        creative_ad.creative_instance_id);
  }

 private:
  absl::flat_hash_map<int, absl::flat_hash_set<std::string>>
      served_creative_instance_ids_by_priority_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_ROUND_ROBIN_CREATIVE_AD_ROUND_ROBIN_H_
