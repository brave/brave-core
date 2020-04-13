/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/bandwidth_savings_predictor.h"

#include <iostream>

#include "base/logging.h"
#include "brave/components/brave_perf_predictor/browser/bandwidth_linreg.h"
#include "brave/components/brave_perf_predictor/browser/named_third_party_registry.h"
#include "components/page_load_metrics/common/page_load_metrics.mojom.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"

namespace brave_perf_predictor {

BandwidthSavingsPredictor::BandwidthSavingsPredictor() = default;

BandwidthSavingsPredictor::~BandwidthSavingsPredictor() = default;

void BandwidthSavingsPredictor::OnPageLoadTimingUpdated(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  // First meaningful paint
  if (timing.paint_timing->first_meaningful_paint.has_value())
    feature_map_["metrics.firstMeaningfulPaint"] =
        timing.paint_timing->first_meaningful_paint.value().InMillisecondsF();

  // DOM Content Loaded
  if (timing.document_timing->dom_content_loaded_event_start.has_value())
    feature_map_["metrics.observedDomContentLoaded"] =
        timing.document_timing->dom_content_loaded_event_start.value()
            .InMillisecondsF();

  // First contentful paint
  if (timing.paint_timing->first_contentful_paint.has_value())
    feature_map_["metrics.observedFirstVisualChange"] =
        timing.paint_timing->first_contentful_paint.value().InMillisecondsF();

  // Load
  if (timing.document_timing->load_event_start.has_value())
    feature_map_["metrics.observedLoad"] =
        timing.document_timing->load_event_start.value().InMillisecondsF();
}

void BandwidthSavingsPredictor::OnSubresourceBlocked(
    const std::string& resource_url) {
  feature_map_["adblockRequests"] += 1;

  const NamedThirdPartyRegistry* tp_registry =
      NamedThirdPartyRegistry::GetInstance();
  if (tp_registry) {
    const auto tp_name = tp_registry->GetThirdParty(resource_url);
    if (tp_name.has_value())
      feature_map_["thirdParties." + tp_name.value() + ".blocked"] = 1;
  }
}

void BandwidthSavingsPredictor::OnResourceLoadComplete(
    const GURL& main_frame_url,
    const blink::mojom::ResourceLoadInfo& resource_load_info) {
  // If the resource load info comes without a valid corresponding
  // main frame URL, ignore it
  if (main_frame_url.is_empty() || !main_frame_url.has_host() ||
      !main_frame_url.SchemeIsHTTPOrHTTPS()) {
    return;
  }
  main_frame_url_ = main_frame_url;

  const bool is_third_party =
      !net::registry_controlled_domains::SameDomainOrHost(
          main_frame_url, resource_load_info.final_url,
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
  switch (resource_load_info.request_destination) {
    case network::mojom::RequestDestination::kDocument:
      resource_type = "document";
      break;
    case network::mojom::RequestDestination::kIframe:
      resource_type = "document";
      break;
    case network::mojom::RequestDestination::kStyle:
      resource_type = "stylesheet";
      break;
    case network::mojom::RequestDestination::kScript:
      resource_type = "script";
      break;
    case network::mojom::RequestDestination::kImage:
      resource_type = "image";
      break;
    case network::mojom::RequestDestination::kFont:
      resource_type = "font";
      break;
    case network::mojom::RequestDestination::kAudio:
    case network::mojom::RequestDestination::kTrack:
    case network::mojom::RequestDestination::kVideo:
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

double BandwidthSavingsPredictor::PredictSavingsBytes() const {
  if (!main_frame_url_.is_valid() || !main_frame_url_.has_host() ||
      !main_frame_url_.SchemeIsHTTPOrHTTPS()) {
    return 0;
  }
  const auto total_size = feature_map_.find("transfer.total.size");
  if (total_size != feature_map_.end() && total_size->second > 0) {
    VLOG(2) << main_frame_url_ << " total download size " << total_size->second
            << " bytes";
  } else {
    return 0;
  }

  // Short-circuit if nothing got blocked
  const auto adblock_requests = feature_map_.find("adblockRequests");
  if (adblock_requests == feature_map_.end() || adblock_requests->second < 1) {
    return 0;
  }
  if (VLOG_IS_ON(3)) {
    VLOG(3) << "Predicting on feature map:";
    for (const auto& feature : feature_map_) {
      VLOG(3) << feature.first << " :: " << feature.second;
    }
  }
  double prediction = ::brave_perf_predictor::LinregPredictNamed(feature_map_);
  VLOG(2) << main_frame_url_ << " estimated saving " << prediction << " bytes";
  // Sanity check for predicted saving
  if (prediction > kSavingsAbsoluteOutlier &&
      (prediction / kOutlierThreshold) > total_size->second) {
    return 0;
  }
  return prediction;
}

void BandwidthSavingsPredictor::Reset() {
  feature_map_.clear();
  main_frame_url_ = {};
}

}  // namespace brave_perf_predictor
