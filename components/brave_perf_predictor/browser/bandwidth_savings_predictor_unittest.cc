/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/bandwidth_savings_predictor.h"

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "chrome/browser/predictors/loading_test_util.h"
#include "components/page_load_metrics/common/page_load_metrics.mojom.h"
#include "components/page_load_metrics/common/page_load_timing.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"
#include "url/gurl.h"

namespace brave_perf_predictor {

TEST(BandwidthSavingsPredictorTest, FeaturiseBlocked) {
  BandwidthSavingsPredictor predictor;
  predictor.OnSubresourceBlocked("https://google-analytics.com");
  EXPECT_EQ(predictor.feature_map_["adblockRequests"], 1);
  EXPECT_EQ(predictor.feature_map_["thirdParties.Google Analytics.blocked"],
            1);
  predictor.OnSubresourceBlocked("https://test.m.facebook.com");
  EXPECT_EQ(predictor.feature_map_["adblockRequests"], 2);
}

TEST(BandwidthSavingsPredictorTest, FeaturiseTiming) {
  BandwidthSavingsPredictor predictor;
  const auto empty_timing = page_load_metrics::CreatePageLoadTiming();
  predictor.OnPageLoadTimingUpdated(*empty_timing);
  EXPECT_EQ(predictor.feature_map_["metrics.firstMeaningfulPaint"], 0);
  EXPECT_EQ(predictor.feature_map_["metrics.observedDomContentLoaded"], 0);
  EXPECT_EQ(predictor.feature_map_["metrics.observedFirstVisualChange"], 0);
  EXPECT_EQ(predictor.feature_map_["metrics.observedLoad"], 0);

  auto timing = page_load_metrics::CreatePageLoadTiming();
  timing->document_timing->dom_content_loaded_event_start =
      base::TimeDelta::FromMilliseconds(1000);
  predictor.OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor.feature_map_["metrics.observedDomContentLoaded"], 1000);

  timing->document_timing->load_event_start =
      base::TimeDelta::FromMilliseconds(2000);
  predictor.OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor.feature_map_["metrics.observedLoad"], 2000);

  timing->paint_timing->first_meaningful_paint =
      base::TimeDelta::FromMilliseconds(1500);
  predictor.OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor.feature_map_["metrics.firstMeaningfulPaint"], 1500);

  timing->paint_timing->first_contentful_paint =
      base::TimeDelta::FromMilliseconds(800);
  predictor.OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor.feature_map_["metrics.observedFirstVisualChange"], 800);
}

TEST(BandwidthSavingsPredictorTest, FeaturiseResourceLoading) {
  BandwidthSavingsPredictor predictor;
  EXPECT_EQ(predictor.feature_map_["resources.third-party.requestCount"], 0);

  const GURL main_frame("https://brave.com/");

  auto fp_style = predictors::CreateResourceLoadInfo(
      "https://brave.com/style.css",
      network::mojom::RequestDestination::kStyle);
  fp_style->raw_body_bytes = 1000;
  predictor.OnResourceLoadComplete(main_frame, *fp_style);
  EXPECT_EQ(predictor.feature_map_["resources.third-party.requestCount"], 0);
  EXPECT_EQ(predictor.feature_map_["resources.stylesheet.requestCount"], 1);
  EXPECT_EQ(predictor.feature_map_["resources.stylesheet.size"], 1000);

  auto tp_style = predictors::CreateResourceLoadInfo(
      "https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.js",
      network::mojom::RequestDestination::kScript);
  tp_style->raw_body_bytes = 1001;
  predictor.OnResourceLoadComplete(main_frame, *tp_style);

  EXPECT_EQ(predictor.feature_map_["resources.third-party.requestCount"], 1);
  EXPECT_EQ(predictor.feature_map_["resources.stylesheet.requestCount"], 1);
  EXPECT_EQ(predictor.feature_map_["resources.script.requestCount"], 1);
  EXPECT_EQ(predictor.feature_map_["resources.stylesheet.size"], 1000);
  EXPECT_EQ(predictor.feature_map_["resources.script.size"], 1001);

  EXPECT_EQ(predictor.feature_map_["resources.total.requestCount"], 2);
  EXPECT_EQ(predictor.feature_map_["resources.total.size"], 2001);
}

TEST(BandwidthSavingsPredictorTest, PredictZeroNoData) {
  BandwidthSavingsPredictor predictor;
  EXPECT_EQ(predictor.PredictSavingsBytes(), 0);
}

TEST(BandwidthSavingsPredictorTest, PredictZeroInternalUrl) {
  BandwidthSavingsPredictor predictor;

  const GURL main_frame("brave://version");
  auto res = predictors::CreateResourceLoadInfo("brave://version");
  predictor.OnResourceLoadComplete(main_frame, *res);

  EXPECT_EQ(predictor.PredictSavingsBytes(), 0);
}

TEST(BandwidthSavingsPredictorTest, PredictZeroBadFrame) {
  BandwidthSavingsPredictor predictor;

  const GURL main_frame("");
  auto res = predictors::CreateResourceLoadInfo(
      "https://brave.com/style.css",
      network::mojom::RequestDestination::kStyle);
  res->raw_body_bytes = 1000;
  predictor.OnResourceLoadComplete(main_frame, *res);

  EXPECT_EQ(predictor.PredictSavingsBytes(), 0);
}

TEST(BandwidthSavingsPredictorTest, PredictZeroNoBlocks) {
  BandwidthSavingsPredictor predictor;

  const GURL main_frame("https://brave.com");
  auto res = predictors::CreateResourceLoadInfo(
      "https://brave.com/style.css",
      network::mojom::RequestDestination::kStyle);
  res->raw_body_bytes = 1000;
  predictor.OnResourceLoadComplete(main_frame, *res);

  EXPECT_EQ(predictor.PredictSavingsBytes(), 0);
}

TEST(BandwidthSavingsPredictorTest, PredictNonZero) {
  BandwidthSavingsPredictor predictor;

  const GURL main_frame("https://brave.com");
  auto res = predictors::CreateResourceLoadInfo(
      "https://brave.com/style.css",
      network::mojom::RequestDestination::kStyle);
  res->raw_body_bytes = 200000;
  res->total_received_bytes = 200000;
  predictor.OnResourceLoadComplete(main_frame, *res);

  predictor.OnSubresourceBlocked("https://google-analytics.com/ga.js");
  // resource still seen as complete, but with 0 bytes
  auto blocked = predictors::CreateResourceLoadInfo(
      "https://google-analytics.com/ga.js",
      network::mojom::RequestDestination::kScript);
  blocked->raw_body_bytes = 0;
  predictor.OnResourceLoadComplete(main_frame, *blocked);

  EXPECT_NE(predictor.PredictSavingsBytes(), 0);
}

}  // namespace brave_perf_predictor
