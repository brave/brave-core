/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/targeting/data_types/behavioral/purchase_intent/purchase_intent_info.h"

#include "base/values.h"
#include "bat/ads/internal/serving/targeting/models/behavioral/purchase_intent/purchase_intent_features.h"

namespace ads {
namespace targeting {

PurchaseIntentInfo::PurchaseIntentInfo() = default;

PurchaseIntentInfo::~PurchaseIntentInfo() = default;

// static
std::unique_ptr<PurchaseIntentInfo> PurchaseIntentInfo::CreateFromValue(
    base::Value resource_value,
    std::string* error_message) {
  DCHECK(error_message);
  auto purchase_intent = std::make_unique<targeting::PurchaseIntentInfo>();

  if (!resource_value.is_dict()) {
    *error_message = "Failed to load from JSON, json is not a dictionary";
    return {};
  }

  if (absl::optional<int> version = resource_value.FindIntPath("version")) {
    if (features::GetPurchaseIntentResourceVersion() != *version) {
      *error_message = "Failed to load from JSON, version missing";
      return {};
    }

    purchase_intent->version = *version;
  }

  // Parsing field: "segments"
  base::Value* incoming_segments = resource_value.FindListPath("segments");
  if (!incoming_segments) {
    *error_message = "Failed to load from JSON, segments missing";
    return {};
  }

  if (!incoming_segments->is_list()) {
    *error_message = "Failed to load from JSON, segments is not of type list";
    return {};
  }

  base::ListValue* list3;
  if (!incoming_segments->GetAsList(&list3)) {
    *error_message = "Failed to load from JSON, get segments as list";
    return {};
  }

  std::vector<std::string> segments;
  for (const auto& segment_value : list3->GetList()) {
    const std::string segment = segment_value.GetString();
    if (segment.empty()) {
      *error_message = "Failed to load from JSON, empty segment found";
      return {};
    }
    segments.push_back(segment);
  }

  // Parsing field: "segment_keywords"
  base::Value* incoming_segment_keywords =
      resource_value.FindDictPath("segment_keywords");
  if (!incoming_segment_keywords) {
    *error_message = "Failed to load from JSON, segment keywords missing";
    return {};
  }

  if (!incoming_segment_keywords->is_dict()) {
    *error_message =
        "Failed to load from JSON, segment keywords not of type dict";
    return {};
  }

  base::DictionaryValue* dict2;
  if (!incoming_segment_keywords->GetAsDictionary(&dict2)) {
    *error_message = "Failed to load from JSON, get segment keywords as dict";
    return {};
  }

  for (base::DictionaryValue::Iterator it(*dict2); !it.IsAtEnd();
       it.Advance()) {
    targeting::PurchaseIntentSegmentKeywordInfo info;
    info.keywords = it.key();
    for (const auto& segment_ix : it.value().GetList()) {
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
  base::Value* incoming_funnel_keywords =
      resource_value.FindDictPath("funnel_keywords");
  if (!incoming_funnel_keywords) {
    *error_message = "Failed to load from JSON, funnel keywords missing";
    return {};
  }

  if (!incoming_funnel_keywords->is_dict()) {
    *error_message =
        "Failed to load from JSON, funnel keywords not of type dict";
    return {};
  }

  base::DictionaryValue* dict;
  if (!incoming_funnel_keywords->GetAsDictionary(&dict)) {
    *error_message = "Failed to load from JSON, get funnel keywords as dict";
    return {};
  }

  for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd(); it.Advance()) {
    targeting::PurchaseIntentFunnelKeywordInfo info;
    info.keywords = it.key();
    info.weight = it.value().GetInt();
    purchase_intent->funnel_keywords.push_back(info);
  }

  // Parsing field: "funnel_sites"
  base::Value* incoming_funnel_sites =
      resource_value.FindListPath("funnel_sites");
  if (!incoming_funnel_sites) {
    *error_message = "Failed to load from JSON, sites missing";
    return {};
  }

  if (!incoming_funnel_sites->is_list()) {
    *error_message = "Failed to load from JSON, sites not of type dict";
    return {};
  }

  base::ListValue* list1;
  if (!incoming_funnel_sites->GetAsList(&list1)) {
    *error_message = "Failed to load from JSON, get sites as dict";
    return {};
  }

  // For each set of sites and segments
  for (auto& set : list1->GetList()) {
    if (!set.is_dict()) {
      *error_message = "Failed to load from JSON, site set not of type dict";
      return {};
    }

    // Get all segments...
    base::ListValue* seg_list;
    base::Value* seg_value = set.FindListPath("segments");
    if (!seg_value->GetAsList(&seg_list)) {
      *error_message =
          "Failed to load from JSON, get site segment list as dict";
      return {};
    }

    std::vector<std::string> site_segments;
    for (auto& seg : seg_list->GetList()) {
      site_segments.push_back(segments.at(seg.GetInt()));
    }

    // ...and for each site create info with appended segments
    base::ListValue* site_list;
    base::Value* site_value = set.FindListPath("sites");
    if (!site_value->GetAsList(&site_list)) {
      *error_message = "Failed to load from JSON, get site list as dict";
      return {};
    }

    for (const auto& site : site_list->GetList()) {
      targeting::PurchaseIntentSiteInfo info;
      info.segments = site_segments;
      info.url_netloc = GURL(site.GetString());
      info.weight = 1;

      purchase_intent->sites.push_back(info);
    }
  }

  return purchase_intent;
}

}  // namespace targeting
}  // namespace ads
