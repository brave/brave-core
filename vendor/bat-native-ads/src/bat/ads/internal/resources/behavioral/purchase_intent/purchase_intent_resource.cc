/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"

#include <vector>

#include "base/json/json_reader.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"
#include "bat/ads/internal/logging.h"
#include "brave/components/l10n/common/locale_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace resource {

namespace {
const char kResourceId[] = "bejenkminijgplakmkmcgkhjjnkelbld";
}  // namespace

PurchaseIntent::PurchaseIntent() = default;

PurchaseIntent::~PurchaseIntent() = default;

bool PurchaseIntent::IsInitialized() const {
  return is_initialized_;
}

void PurchaseIntent::Load() {
  AdsClientHelper::Get()->LoadAdsResource(
      kResourceId, features::GetPurchaseIntentResourceVersion(),
      [=](const bool success, const std::string& json) {
        if (!success) {
          BLOG(1,
               "Failed to load " << kResourceId << " purchase intent resource");
          is_initialized_ = false;
          return;
        }

        BLOG(1, "Successfully loaded " << kResourceId
                                       << " purchase intent resource");

        if (!FromJson(json)) {
          BLOG(1, "Failed to initialize " << kResourceId
                                          << " purchase intent resource");
          is_initialized_ = false;
          return;
        }

        is_initialized_ = true;

        BLOG(1, "Successfully initialized " << kResourceId
                                            << " purchase intent resource");
      });
}

ad_targeting::PurchaseIntentInfo PurchaseIntent::get() const {
  return purchase_intent_;
}

///////////////////////////////////////////////////////////////////////////////

bool PurchaseIntent::FromJson(const std::string& json) {
  ad_targeting::PurchaseIntentInfo purchase_intent;

  absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root) {
    BLOG(1, "Failed to load from JSON, root missing");
    return false;
  }

  if (absl::optional<int> version = root->FindIntPath("version")) {
    if (features::GetPurchaseIntentResourceVersion() != *version) {
      BLOG(1, "Failed to load from JSON, version missing");
      return false;
    }

    purchase_intent.version = *version;
  }

  // Parsing field: "segments"
  base::Value* incoming_segments = root->FindListPath("segments");
  if (!incoming_segments) {
    BLOG(1, "Failed to load from JSON, segments missing");
    return false;
  }

  if (!incoming_segments->is_list()) {
    BLOG(1, "Failed to load from JSON, segments is not of type list");
    return false;
  }

  base::ListValue* list3;
  if (!incoming_segments->GetAsList(&list3)) {
    BLOG(1, "Failed to load from JSON, get segments as list");
    return false;
  }

  std::vector<std::string> segments;
  for (const auto& segment_value : list3->GetList()) {
    const std::string segment = segment_value.GetString();
    if (segment.empty()) {
      BLOG(1, "Failed to load from JSON, empty segment found");
      return false;
    }
    segments.push_back(segment);
  }

  // Parsing field: "segment_keywords"
  base::Value* incoming_segment_keywords =
      root->FindDictPath("segment_keywords");
  if (!incoming_segment_keywords) {
    BLOG(1, "Failed to load from JSON, segment keywords missing");
    return false;
  }

  if (!incoming_segment_keywords->is_dict()) {
    BLOG(1, "Failed to load from JSON, segment keywords not of type dict");
    return false;
  }

  base::DictionaryValue* dict2;
  if (!incoming_segment_keywords->GetAsDictionary(&dict2)) {
    BLOG(1, "Failed to load from JSON, get segment keywords as dict");
    return false;
  }

  for (base::DictionaryValue::Iterator it(*dict2); !it.IsAtEnd();
       it.Advance()) {
    ad_targeting::PurchaseIntentSegmentKeywordInfo info;
    info.keywords = it.key();
    for (const auto& segment_ix : it.value().GetList()) {
      if (static_cast<size_t>(segment_ix.GetInt()) >= segments.size()) {
        BLOG(1, "Failed to load from JSON, segment keywords are ill-formed");
        return false;
      }
      info.segments.push_back(segments.at(segment_ix.GetInt()));
    }

    purchase_intent.segment_keywords.push_back(info);
  }

  // Parsing field: "funnel_keywords"
  base::Value* incoming_funnel_keywords = root->FindDictPath("funnel_keywords");
  if (!incoming_funnel_keywords) {
    BLOG(1, "Failed to load from JSON, funnel keywords missing");
    return false;
  }

  if (!incoming_funnel_keywords->is_dict()) {
    BLOG(1, "Failed to load from JSON, funnel keywords not of type dict");
    return false;
  }

  base::DictionaryValue* dict;
  if (!incoming_funnel_keywords->GetAsDictionary(&dict)) {
    BLOG(1, "Failed to load from JSON, get funnel keywords as dict");
    return false;
  }

  for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd(); it.Advance()) {
    ad_targeting::PurchaseIntentFunnelKeywordInfo info;
    info.keywords = it.key();
    info.weight = it.value().GetInt();
    purchase_intent.funnel_keywords.push_back(info);
  }

  // Parsing field: "funnel_sites"
  base::Value* incoming_funnel_sites = root->FindListPath("funnel_sites");
  if (!incoming_funnel_sites) {
    BLOG(1, "Failed to load from JSON, sites missing");
    return false;
  }

  if (!incoming_funnel_sites->is_list()) {
    BLOG(1, "Failed to load from JSON, sites not of type dict");
    return false;
  }

  base::ListValue* list1;
  if (!incoming_funnel_sites->GetAsList(&list1)) {
    BLOG(1, "Failed to load from JSON, get sites as dict");
    return false;
  }

  // For each set of sites and segments
  for (auto& set : list1->GetList()) {
    if (!set.is_dict()) {
      BLOG(1, "Failed to load from JSON, site set not of type dict");
      return false;
    }

    // Get all segments...
    base::ListValue* seg_list;
    base::Value* seg_value = set.FindListPath("segments");
    if (!seg_value->GetAsList(&seg_list)) {
      BLOG(1, "Failed to load from JSON, get site segment list as dict");
      return false;
    }

    std::vector<std::string> site_segments;
    for (auto& seg : seg_list->GetList()) {
      site_segments.push_back(segments.at(seg.GetInt()));
    }

    // ...and for each site create info with appended segments
    base::ListValue* site_list;
    base::Value* site_value = set.FindListPath("sites");
    if (!site_value->GetAsList(&site_list)) {
      BLOG(1, "Failed to load from JSON, get site list as dict");
      return false;
    }

    for (const auto& site : site_list->GetList()) {
      ad_targeting::PurchaseIntentSiteInfo info;
      info.segments = site_segments;
      info.url_netloc = site.GetString();
      info.weight = 1;

      purchase_intent.sites.push_back(info);
    }
  }

  purchase_intent_ = purchase_intent;

  BLOG(1,
       "Parsed purchase intent resource version " << purchase_intent.version);

  return true;
}

}  // namespace resource
}  // namespace ads
