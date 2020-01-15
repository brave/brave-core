/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_BANDWIDTH_SAVINGS_PREDICTOR_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_BANDWIDTH_SAVINGS_PREDICTOR_H_

#include <string>
#include <unordered_map>

#include "components/page_load_metrics/common/page_load_metrics.mojom.h"
#include "content/public/common/resource_load_info.mojom.h"

namespace brave_perf_predictor {

class BandwidthSavingsPredictor {
 public:
  explicit BandwidthSavingsPredictor();
  ~BandwidthSavingsPredictor();

  void OnPageLoadTimingUpdated(
      const page_load_metrics::mojom::PageLoadTiming& timing);
  void OnSubresourceBlocked(const std::string& resource_url);
  void OnResourceLoadComplete(
      const GURL& main_frame_url,
      const content::mojom::ResourceLoadInfo& resource_load_info);
  double predict();
  void Reset();

 private:
  GURL main_frame_url_;
  std::unordered_map<std::string, double> feature_map_;
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_BANDWIDTH_SAVINGS_PREDICTOR_H_
