/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/bandwidth_savings_predictor.h"

#include "base/logging.h"
#include "content/public/common/resource_type.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#include "brave/components/brave_perf_predictor/browser/predictor.h"

namespace brave_perf_predictor {

BandwidthSavingsPredictor::BandwidthSavingsPredictor(
    ThirdPartyExtractor* third_party_extractor):
  third_party_extractor_(third_party_extractor) {}

BandwidthSavingsPredictor::~BandwidthSavingsPredictor() = default;

void BandwidthSavingsPredictor::OnPageLoadTimingUpdated(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  // First meaningful paint
  if (timing.paint_timing->first_meaningful_paint.has_value()) {
    feature_map_["metrics.firstMeaningfulPaint"] = 
      timing.paint_timing->first_meaningful_paint.value().InMillisecondsF();
  }
  // DOM Content Loaded
  if (timing.document_timing->
      dom_content_loaded_event_start.has_value()) {
    feature_map_["metrics.observedDomContentLoaded"] =
      timing.document_timing->
        dom_content_loaded_event_start.value().InMillisecondsF();
  }
  // First contentful paint
  if (timing.paint_timing->first_contentful_paint.has_value()) {
    feature_map_["metrics.observedFirstVisualChange"] =
      timing.paint_timing->first_contentful_paint.value().InMillisecondsF();
  }
  // Load
  if (timing.document_timing->load_event_start.has_value()) {
    feature_map_["metrics.observedLoad"] =
      timing.document_timing->load_event_start.value().InMillisecondsF();
  }
  // Interactive
  if (timing.interactive_timing->interactive.has_value()) {
    feature_map_["metrics.interactive"] =
      timing.interactive_timing->interactive.value().InMillisecondsF();
  }
}

void BandwidthSavingsPredictor::OnSubresourceBlocked(
    const std::string& resource_url) {
  feature_map_["adblockRequests"] += 1;
  if (third_party_extractor_) {
    auto entity_name = third_party_extractor_->get_entity(resource_url);
    if (entity_name.has_value()) {
      feature_map_["thirdParties." + entity_name.value() + ".blocked"] = 1;
    }
  }
}

void BandwidthSavingsPredictor::OnResourceLoadComplete(
    const GURL& main_frame_url,
    const content::mojom::ResourceLoadInfo& resource_load_info) {

  // If the resource load info comes without a valid corresponding
  // main frame URL, ignore it
  if (main_frame_url.is_empty() || !main_frame_url.has_host()
      || !main_frame_url.SchemeIsHTTPOrHTTPS()) {
    return;
  }
  main_frame_url_ = main_frame_url;
  
  bool is_third_party = !net::registry_controlled_domains::SameDomainOrHost(
      main_frame_url, resource_load_info.url,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  
  if (is_third_party) {
    feature_map_["resources.third-party.requestCount"] += 1;
    feature_map_["resources.third-party.size"] +=
      resource_load_info.raw_body_bytes;    
  }

  feature_map_["resources.total.requestCount"] += 1;
  feature_map_["resources.total.size"] += resource_load_info.raw_body_bytes;
  feature_map_["transfer.total.size"] +=
    resource_load_info.total_received_bytes;
  std::string resource_type;
  switch(resource_load_info.resource_type) {
    case content::ResourceType::kMainFrame:
      resource_type = "document";
      break;
    case content::ResourceType::kSubFrame:
      resource_type = "document";
      break;
    case content::ResourceType::kStylesheet:
      resource_type = "stylesheet";
      break;
    case content::ResourceType::kScript:
      resource_type = "script";
      break;
    case content::ResourceType::kImage:
      resource_type = "image";
      break;
    case content::ResourceType::kFontResource:
      resource_type = "font";
      break;
    case content::ResourceType::kMedia:
      resource_type = "media";
      break;
    default:
      resource_type = "other";
      break;
  }
  feature_map_["resources." + resource_type + ".requestCount"] += 1;
  feature_map_["resources." + resource_type + ".size"] +=
    resource_load_info.raw_body_bytes;
}

double BandwidthSavingsPredictor::predict() {
  if (main_frame_url_.is_empty() || !main_frame_url_.has_host()
      || !main_frame_url_.SchemeIsHTTPOrHTTPS()) {
    Reset();
    return 0;
  }
  if (feature_map_["transfer.total.size"] > 0) {
    VLOG(2) << main_frame_url_ << " total download size " 
      << feature_map_["transfer.total.size"] << " bytes";
  }

  // Short-circuit if nothing got blocked
  if (feature_map_["adblockRequests"] < 1) {
    Reset();
    return 0;
  }
  if (VLOG_IS_ON(3)) {
    VLOG(2) << "Predicting on feature map:";
    auto it = feature_map_.begin();
    while(it != feature_map_.end()) {
      VLOG(2) << it->first << " :: " << it->second;
      it++;
    }
  }
  double prediction = ::brave_perf_predictor::predict(feature_map_);
  VLOG(2) << main_frame_url_ << " estimated saving "
    << prediction << " bytes";
  Reset();
  return prediction;
}

void BandwidthSavingsPredictor::Reset() {
  feature_map_.clear();
}

}