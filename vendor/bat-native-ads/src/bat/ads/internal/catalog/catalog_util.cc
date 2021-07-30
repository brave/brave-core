/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_util.h"

#include <algorithm>
#include <cstdint>

#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {
const int kCatalogLifespanInDays = 1;
}

bool DoesCatalogExist() {
  return AdsClientHelper::Get()->GetIntegerPref(prefs::kCatalogVersion) > 0;
}

bool HasCatalogExpired() {
  const base::Time now = base::Time::Now();

  const int64_t catalog_last_updated =
      AdsClientHelper::Get()->GetInt64Pref(prefs::kCatalogLastUpdated);

  const base::Time time = base::Time::FromDoubleT(catalog_last_updated);

  if (now < time + base::TimeDelta::FromDays(kCatalogLifespanInDays)) {
    return false;
  }

  return true;
}

SegmentList GetSegments(const Catalog& catalog) {
  SegmentList segments;

  const CatalogCampaignList catalog_campaigns = catalog.GetCampaigns();
  for (const auto& catalog_campaign : catalog_campaigns) {
    CatalogCreativeSetList catalog_creative_sets =
        catalog_campaign.creative_sets;
    for (const auto& catalog_creative_set : catalog_creative_sets) {
      CatalogSegmentList catalog_segments = catalog_creative_set.segments;
      for (const auto& catalog_segment : catalog_segments) {
        segments.push_back(catalog_segment.name);
      }
    }
  }

  // Remove duplicates
  std::sort(segments.begin(), segments.end());
  const auto iter = std::unique(segments.begin(), segments.end());
  segments.erase(iter, segments.end());

  return segments;
}

}  // namespace ads
