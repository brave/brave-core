/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_info.h"

#include <utility>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads {

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
  const auto* const segments_list = dict.FindList("segments");
  if (!segments_list) {
    return base::unexpected("Failed to load from JSON, segments missing");
  }

  std::vector<std::string> segments;
  for (const auto& item : *segments_list) {
    const std::string& segment = item.GetString();
    if (segment.empty()) {
      return base::unexpected("Failed to load from JSON, empty segment found");
    }

    segments.push_back(segment);
  }

  // Parsing field: "segment_keywords"
  const auto* const segment_keywords_dict = dict.FindDict("segment_keywords");
  if (!segment_keywords_dict) {
    return base::unexpected(
        "Failed to load from JSON, segment keywords missing");
  }

  for (const auto [keywords, indexes] : *segment_keywords_dict) {
    PurchaseIntentSegmentKeywordInfo purchase_intent_segment_keyword;

    purchase_intent_segment_keyword.keywords = keywords;

    for (const auto& index : indexes.GetList()) {
      CHECK(index.is_int());

      if (index.GetInt() >= static_cast<int>(segments.size())) {
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
  const auto* const funnel_keywords_dict = dict.FindDict("funnel_keywords");
  if (!funnel_keywords_dict) {
    return base::unexpected(
        "Failed to load from JSON, funnel keywords missing");
  }

  for (const auto [keywords, weight] : *funnel_keywords_dict) {
    PurchaseIntentFunnelKeywordInfo purchase_intent_funnel_keyword;
    purchase_intent_funnel_keyword.keywords = keywords;
    purchase_intent_funnel_keyword.weight = weight.GetInt();
    purchase_intent.funnel_keywords.push_back(
        std::move(purchase_intent_funnel_keyword));
  }

  // Parsing field: "funnel_sites"
  const auto* const funnel_sites_list = dict.FindList("funnel_sites");
  if (!funnel_sites_list) {
    return base::unexpected("Failed to load from JSON, funnel sites missing");
  }

  // For each set of sites and segments
  for (const auto& item : *funnel_sites_list) {
    if (!item.is_dict()) {
      return base::unexpected(
          "Failed to load from JSON, funnel site not of type dict");
    }
    const base::Value::Dict& item_dict = item.GetDict();

    // Get all segments...
    const auto* const funnel_site_segments_list =
        item_dict.FindList("segments");
    if (!funnel_site_segments_list) {
      return base::unexpected(
          "Failed to load from JSON, funnel site segments not of tyoe list");
    }

    std::vector<std::string> purchase_intent_segments;
    for (const auto& segment : *funnel_site_segments_list) {
      CHECK(segment.is_int());
      purchase_intent_segments.push_back(segments.at(segment.GetInt()));
    }

    // ...and for each site create info with appended segments
    const auto* const sites_list = item_dict.FindList("sites");
    if (!sites_list) {
      return base::unexpected(
          "Failed to load from JSON, sites site not of tyoe list");
    }

    for (const auto& site : *sites_list) {
      CHECK(site.is_string());

      PurchaseIntentSiteInfo purchase_intent_site;
      purchase_intent_site.segments = purchase_intent_segments;
      purchase_intent_site.url_netloc = GURL(site.GetString());
      purchase_intent_site.weight = 1;

      purchase_intent.sites.push_back(std::move(purchase_intent_site));
    }
  }

  return purchase_intent;
}

}  // namespace brave_ads
