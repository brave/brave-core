/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/multi_armed_bandits/epsilon_greedy_bandit_resource.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/multi_armed_bandits/epsilon_greedy_bandit_resource_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

namespace brave_ads::resource {

EpsilonGreedyBandit::EpsilonGreedyBandit(Catalog* catalog) : catalog_(catalog) {
  DCHECK(catalog_);

  catalog_->AddObserver(this);
}

EpsilonGreedyBandit::~EpsilonGreedyBandit() {
  catalog_->RemoveObserver(this);
}

bool EpsilonGreedyBandit::IsInitialized() const {
  return is_initialized_;
}

void EpsilonGreedyBandit::LoadFromCatalog(const CatalogInfo& catalog) {
  const SegmentList segments = GetSegments(catalog);
  const SegmentList parent_segments = GetParentSegments(segments);

  BLOG(2, "Successfully loaded epsilon greedy bandit segments:");
  for (const auto& segment : parent_segments) {
    BLOG(2, "  " << segment);
  }

  SetEpsilonGreedyBanditEligibleSegments(parent_segments);

  is_initialized_ = true;
}

// static
SegmentList EpsilonGreedyBandit::Get() {
  return GetEpsilonGreedyBanditEligibleSegments();
}

///////////////////////////////////////////////////////////////////////////////

void EpsilonGreedyBandit::OnDidUpdateCatalog(const CatalogInfo& catalog) {
  LoadFromCatalog(catalog);
}

}  // namespace brave_ads::resource
