/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  return UserHasOptedInToBraveNewsAds() || UserHasOptedInToNotificationAds();
}

}  // namespace

EpsilonGreedyBanditResource::EpsilonGreedyBanditResource(Catalog& catalog)
    : catalog_(catalog) {
  AddAdsClientNotifierObserver(this);
  catalog_->AddObserver(this);
}

EpsilonGreedyBanditResource::~EpsilonGreedyBanditResource() {
  RemoveAdsClientNotifierObserver(this);
  catalog_->RemoveObserver(this);
}

// static
SegmentList EpsilonGreedyBanditResource::Get() {
  return GetEpsilonGreedyBanditEligibleSegments();
}

///////////////////////////////////////////////////////////////////////////////

void EpsilonGreedyBanditResource::LoadFromCatalog(const CatalogInfo& catalog) {
  const SegmentList segments = GetSegments(catalog);
  const SegmentList parent_segments = GetParentSegments(segments);

  BLOG(2, "Successfully loaded epsilon greedy bandit segments:");
  for (const auto& segment : parent_segments) {
    BLOG(2, "  " << segment);
  }

  SetEpsilonGreedyBanditEligibleSegments(parent_segments);

  is_initialized_ = true;
}

void EpsilonGreedyBanditResource::MaybeReset() {
  if (IsInitialized() && !DoesRequireResource()) {
    Reset();
  }
}

void EpsilonGreedyBanditResource::Reset() {
  BLOG(1, "Reset epsilon greedy bandit resource");
  ResetEpsilonGreedyBanditEligibleSegments();
  is_initialized_ = false;
}

void EpsilonGreedyBanditResource::OnNotifyPrefDidChange(
    const std::string& path) {
  if (path == brave_rewards::prefs::kEnabled ||
      path == prefs::kOptedInToNotificationAds ||
      path == brave_news::prefs::kBraveNewsOptedIn ||
      path == brave_news::prefs::kNewTabPageShowToday) {
    MaybeReset();
  }
}

void EpsilonGreedyBanditResource::OnDidUpdateCatalog(
    const CatalogInfo& catalog) {
  if (DoesRequireResource()) {
    LoadFromCatalog(catalog);
  }
}

}  // namespace brave_ads
