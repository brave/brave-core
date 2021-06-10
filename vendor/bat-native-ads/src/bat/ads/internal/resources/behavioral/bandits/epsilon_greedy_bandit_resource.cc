/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"

#include <string>

#include "bat/ads/internal/ad_targeting/ad_targeting_segment_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace resource {

namespace {

SegmentList GetSegments() {
  const std::string json = AdsClientHelper::Get()->GetStringPref(
      prefs::kEpsilonGreedyBanditEligibleSegments);

  return DeserializeSegments(json);
}

}  // namespace

EpsilonGreedyBandit::EpsilonGreedyBandit() = default;

EpsilonGreedyBandit::~EpsilonGreedyBandit() = default;

bool EpsilonGreedyBandit::IsInitialized() const {
  return is_initialized_;
}

void EpsilonGreedyBandit::LoadFromCatalog(const Catalog& catalog) {
  const SegmentList segments = GetSegments(catalog);

  const SegmentList parent_segments = GetParentSegments(segments);

  const std::string json = SerializeSegments(parent_segments);

  AdsClientHelper::Get()->SetStringPref(
      prefs::kEpsilonGreedyBanditEligibleSegments, json);

  BLOG(2, "Successfully loaded epsilon greedy bandit segments:");
  for (const auto& segment : parent_segments) {
    BLOG(2, "  " << segment);
  }

  is_initialized_ = true;
}

SegmentList EpsilonGreedyBandit::get() const {
  return GetSegments();
}

}  // namespace resource
}  // namespace ads
