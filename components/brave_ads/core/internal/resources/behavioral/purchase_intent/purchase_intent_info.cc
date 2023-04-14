/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_info.h"

#include <utility>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_features.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads::targeting {

PurchaseIntentInfo::PurchaseIntentInfo() = default;
PurchaseIntentInfo::PurchaseIntentInfo(PurchaseIntentInfo&& other) noexcept =
    default;
PurchaseIntentInfo& PurchaseIntentInfo::operator=(
    PurchaseIntentInfo&& other) noexcept = default;
PurchaseIntentInfo::~PurchaseIntentInfo() = default;

// static
base::expected<PurchaseIntentInfo, std::string>
PurchaseIntentInfo::CreateFromValue(const base::Value::Dict dict) {
  // TODO(https://github.com/brave/brave-browser/issues/24942): Reduce cognitive
  // complexity.
  PurchaseIntentInfo purchase_intent;

  if (absl::optional<int> version = dict.FindInt("version")) {
    if (kPurchaseIntentResourceVersion.Get() != *version) {
      return base::unexpected("Failed to load from JSON, version mismatch");
    }

    purchase_intent.version = *version;
  }

  // Parsing field: "segments"
  const base::Value::List* const incoming_segments = dict.FindList("segments");
  if (!incoming_segments) {
    return base::unexpected("Failed to load from JSON, segments missing");
  }

  std::vector<std::string> segments;
  for (const auto& item : *incoming_segments) {
    const std::string& segment = item.GetString();
    if (segment.empty()) {
      return base::unexpected("Failed to load from JSON, empty segment found");
    }
    segments.push_back(segment);
  }

  // Parsing field: "segment_keywords"
  const base::Value::Dict* const incoming_segment_keywords =
      dict.FindDict("segment_keywords");
  if (!incoming_segment_keywords) {
    return base::unexpected(
        "Failed to load from JSON, segment keywords missing");
  }

  for (const auto [keywords, indexes] : *incoming_segment_keywords) {
    PurchaseIntentSegmentKeywordInfo purchase_intent_segment_keyword;
    purchase_intent_segment_keyword.keywords = keywords;

    for (const auto& index : indexes.GetList()) {
      DCHECK(index.is_int());
      if (static_cast<size_t>(index.GetInt()) >= segments.size()) {
        return base::unexpected(
            "Failed to load from JSON, segment keywords are ill-formed");
      }
      purchase_intent_segment_keyword.segments.push_back(
          segments.at(index.GetInt()));
    }

    purchase_intent.segment_keywords.push_back(
        std::move(purchase_intent_segment_keyword));
  }

  // Parsing field: "funnel_keywords"
  const base::Value::Dict* const incoming_funnel_keywords =
      dict.FindDict("funnel_keywords");
  if (!incoming_funnel_keywords) {
    return base::unexpected(
        "Failed to load from JSON, funnel keywords missing");
  }

  for (const auto [keywords, weight] : *incoming_funnel_keywords) {
    PurchaseIntentFunnelKeywordInfo purchase_intent_funnel_keyword;
    purchase_intent_funnel_keyword.keywords = keywords;
    purchase_intent_funnel_keyword.weight = weight.GetInt();
    purchase_intent.funnel_keywords.push_back(
        std::move(purchase_intent_funnel_keyword));
  }

  // Parsing field: "funnel_sites"
  const base::Value::List* const incoming_funnel_sites =
      dict.FindList("funnel_sites");
  if (!incoming_funnel_sites) {
    return base::unexpected("Failed to load from JSON, sites missing");
  }

  // For each set of sites and segments
  for (const auto& item : *incoming_funnel_sites) {
    if (!item.is_dict()) {
      return base::unexpected(
          "Failed to load from JSON, site set not of type dict");
    }
    const auto& set = item.GetDict();

    // Get all segments...
    const base::Value::List* const seg_list = set.FindList("segments");
    if (!seg_list) {
      return base::unexpected(
          "Failed to load from JSON, get site segment list as dict");
    }

    std::vector<std::string> site_segments;
    for (const auto& seg : *seg_list) {
      DCHECK(seg.is_int());
      site_segments.push_back(segments.at(seg.GetInt()));
    }

    // ...and for each site create info with appended segments
    const base::Value::List* const site_list = set.FindList("sites");
    if (!site_list) {
      return base::unexpected(
          "Failed to load from JSON, get site list as dict");
    }

    for (const auto& site : *site_list) {
      DCHECK(site.is_string());
      PurchaseIntentSiteInfo purchase_intent_site;
      purchase_intent_site.segments = site_segments;
      purchase_intent_site.url_netloc = GURL(site.GetString());
      purchase_intent_site.weight = 1;

      purchase_intent.sites.push_back(std::move(purchase_intent_site));
    }
  }

  return purchase_intent;
}

}  // namespace brave_ads::targeting
