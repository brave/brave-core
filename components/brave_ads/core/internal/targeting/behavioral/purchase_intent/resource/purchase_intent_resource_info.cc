/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource_info.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_value_util.h"

namespace brave_ads {

PurchaseIntentResourceInfo::PurchaseIntentResourceInfo() = default;

PurchaseIntentResourceInfo::PurchaseIntentResourceInfo(
    PurchaseIntentResourceInfo&& other) noexcept = default;

PurchaseIntentResourceInfo& PurchaseIntentResourceInfo::operator=(
    PurchaseIntentResourceInfo&& other) noexcept = default;

PurchaseIntentResourceInfo::~PurchaseIntentResourceInfo() = default;

// static
base::expected<PurchaseIntentResourceInfo, std::string>
PurchaseIntentResourceInfo::CreateFromValue(const base::Value::Dict dict) {
  const std::optional<int> version = ParseVersion(dict);
  if (!version) {
    return base::unexpected("Failed to parse purchase intent resource version");
  }

  if (version != kPurchaseIntentResourceVersion.Get()) {
    return base::unexpected("Purchase intent resource version mismatch");
  }

  const std::optional<SegmentList> segments = ParseSegments(dict);
  if (!segments) {
    return base::unexpected(
        "Failed to parse purchase intent resource segments");
  }

  std::optional<PurchaseIntentSegmentKeyphraseList> segment_keyphrases =
      ParseSegmentKeyphrases(*segments, dict);
  if (!segment_keyphrases) {
    return base::unexpected(
        "Failed to parse purchase intent resource segment keyphrases");
  }

  std::optional<PurchaseIntentFunnelKeyphraseList> funnel_keyphrases =
      ParseFunnelKeyphrases(dict);
  if (!funnel_keyphrases) {
    return base::unexpected(
        "Failed to parse purchase intent resource funnel keyphrases");
  }

  const std::optional<PurchaseIntentFunnelSiteMap> funnel_sites =
      ParseFunnelSites(*segments, dict);
  if (!funnel_sites) {
    return base::unexpected(
        "Failed to parse purchase intent resource funnel sites");
  }

  PurchaseIntentResourceInfo purchase_intent;
  purchase_intent.version = version;
  purchase_intent.segment_keyphrases = std::move(*segment_keyphrases);
  purchase_intent.funnel_keyphrases = std::move(*funnel_keyphrases);
  purchase_intent.funnel_sites = *funnel_sites;

  return purchase_intent;
}

}  // namespace brave_ads
