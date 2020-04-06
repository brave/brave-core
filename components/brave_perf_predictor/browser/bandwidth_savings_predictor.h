/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_BANDWIDTH_SAVINGS_PREDICTOR_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_BANDWIDTH_SAVINGS_PREDICTOR_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "url/gurl.h"

namespace page_load_metrics {
namespace mojom {

class PageLoadTiming;

}  // namespace mojom
}  // namespace page_load_metrics

namespace blink {
namespace mojom {

class ResourceLoadInfo;

}  // namespace mojom
}  // namespace blink

namespace brave_perf_predictor {

// Accumulates statistics for a page being loaded and produces estimated
// bandwidth savings when queried. If reused, caller is responsible for
// resetting the predictor's state by calling |Reset|.
//
// The predictor expects to receive a series of |PageLoadTiming| inputs to
// extract relevant standard performance metrics from, as well as notifications
// of any resources fully loaded or blocked.
class BandwidthSavingsPredictor {
 public:
  BandwidthSavingsPredictor();
  ~BandwidthSavingsPredictor();

  BandwidthSavingsPredictor(const BandwidthSavingsPredictor&) = delete;
  BandwidthSavingsPredictor& operator=(const BandwidthSavingsPredictor&) =
      delete;

  void OnPageLoadTimingUpdated(
      const page_load_metrics::mojom::PageLoadTiming& timing);
  void OnSubresourceBlocked(const std::string& resource_url);
  void OnResourceLoadComplete(
      const GURL& main_frame_url,
      const blink::mojom::ResourceLoadInfo& resource_load_info);
  double PredictSavingsBytes() const;
  void Reset();

 private:
  FRIEND_TEST_ALL_PREFIXES(BandwidthSavingsPredictorTest, FeaturiseBlocked);
  FRIEND_TEST_ALL_PREFIXES(BandwidthSavingsPredictorTest, FeaturiseTiming);
  FRIEND_TEST_ALL_PREFIXES(BandwidthSavingsPredictorTest,
                           FeaturiseResourceLoading);

  GURL main_frame_url_;
  base::flat_map<std::string, double> feature_map_;
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_BANDWIDTH_SAVINGS_PREDICTOR_H_
