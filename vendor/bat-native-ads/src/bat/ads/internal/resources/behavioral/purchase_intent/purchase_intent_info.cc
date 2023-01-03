/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_info.h"

#include "absl/types/optional.h"
#include "base/values.h"
#include "bat/ads/internal/features/purchase_intent_features.h"
#include "url/gurl.h"

namespace ads::targeting {

PurchaseIntentInfo::PurchaseIntentInfo() = default;

PurchaseIntentInfo::~PurchaseIntentInfo() = default;

// static
// TODO(https://github.com/brave/brave-browser/issues/24942): Reduce cognitive
// complexity.
std::unique_ptr<PurchaseIntentInfo> PurchaseIntentInfo::CreateFromValue(
    base::Value resource_value,
    std::string* error_message) {
  DCHECK(error_message);
  auto purchase_intent = std::make_unique<PurchaseIntentInfo>();

  base::Value::Dict* resource = resource_value.GetIfDict();
  if (!resource) {
    *error_message = "Failed to load from JSON, json is not a dictionary";
    return {};
  }

  if (absl::optional<int> version = resource->FindInt("version")) {
    if (targeting::features::GetPurchaseIntentResourceVersion() != *version) {
      *error_message = "Failed to load from JSON, version missing";
      return {};
    }

    purchase_intent->version = *version;
  }

  // Parsing field: "segments"
  const base::Value::List* const incoming_segments =
      resource->FindList("segments");
  if (!incoming_segments) {
    *error_message = "Failed to load from JSON, segments missing";
    return {};
  }

  std::vector<std::string> segments;
  for (const auto& item : *incoming_segments) {
    const std::string& segment = item.GetString();
    if (segment.empty()) {
      *error_message = "Failed to load from JSON, empty segment found";
      return {};
    }
    segments.push_back(segment);
  }

  // Parsing field: "segment_keywords"
  const base::Value::Dict* const incoming_segment_keywords =
      resource->FindDict("segment_keywords");
  if (!incoming_segment_keywords) {
    *error_message = "Failed to load from JSON, segment keywords missing";
    return {};
  }

  for (const auto item : *incoming_segment_keywords) {
    PurchaseIntentSegmentKeywordInfo info;
    info.keywords = item.first;
    for (const auto& segment_ix : item.second.GetList()) {
      DCHECK(segment_ix.is_int());
      if (static_cast<size_t>(segment_ix.GetInt()) >= segments.size()) {
        *error_message =
            "Failed to load from JSON, segment keywords are ill-formed";
        return {};
      }
      info.segments.push_back(segments.at(segment_ix.GetInt()));
    }

    purchase_intent->segment_keywords.push_back(info);
  }

  // Parsing field: "funnel_keywords"
  const base::Value::Dict* const incoming_funnel_keywords =
      resource->FindDict("funnel_keywords");
  if (!incoming_funnel_keywords) {
    *error_message = "Failed to load from JSON, funnel keywords missing";
    return {};
  }

  for (const auto item : *incoming_funnel_keywords) {
    PurchaseIntentFunnelKeywordInfo info;
    info.keywords = item.first;
    info.weight = item.second.GetInt();
    purchase_intent->funnel_keywords.push_back(info);
  }

  // Parsing field: "funnel_sites"
  const base::Value::List* const incoming_funnel_sites =
      resource->FindList("funnel_sites");
  if (!incoming_funnel_sites) {
    *error_message = "Failed to load from JSON, sites missing";
    return {};
  }

  // For each set of sites and segments
  for (const auto& item : *incoming_funnel_sites) {
    if (!item.is_dict()) {
      *error_message = "Failed to load from JSON, site set not of type dict";
      return {};
    }
    const auto& set = item.GetDict();

    // Get all segments...
    const base::Value::List* const seg_list = set.FindList("segments");
    if (!seg_list) {
      *error_message =
          "Failed to load from JSON, get site segment list as dict";
      return {};
    }

    std::vector<std::string> site_segments;
    for (const auto& seg : *seg_list) {
      DCHECK(seg.is_int());
      site_segments.push_back(segments.at(seg.GetInt()));
    }

    // ...and for each site create info with appended segments
    const base::Value::List* const site_list = set.FindList("sites");
    if (!site_list) {
      *error_message = "Failed to load from JSON, get site list as dict";
      return {};
    }

    for (const auto& site : *site_list) {
      DCHECK(site.is_string());
      PurchaseIntentSiteInfo info;
      info.segments = site_segments;
      info.url_netloc = GURL(site.GetString());
      info.weight = 1;

      purchase_intent->sites.push_back(info);
    }
  }

  return purchase_intent;
}

}  // namespace ads::targeting
