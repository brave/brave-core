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
#include "content/public/common/resource_load_info.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_perf_predictor {

TEST(BandwidthSavingsPredictorTest, FeaturiseBlocked) {
  auto* predictor = new BandwidthSavingsPredictor();
  predictor->OnSubresourceBlocked("https://google-analytics.com");
  EXPECT_EQ(predictor->feature_map_["adblockRequests"], 1);
  EXPECT_EQ(predictor->feature_map_["thirdParties.Google Analytics.blocked"],
            1);
  predictor->OnSubresourceBlocked("https://test.m.facebook.com");
  EXPECT_EQ(predictor->feature_map_["adblockRequests"], 2);
}

TEST(BandwidthSavingsPredictorTest, FeaturiseTiming) {
  auto* predictor = new BandwidthSavingsPredictor();
  const auto empty_timing = page_load_metrics::CreatePageLoadTiming();
  predictor->OnPageLoadTimingUpdated(*empty_timing);
  EXPECT_EQ(predictor->feature_map_["metrics.firstMeaningfulPaint"], 0);
  EXPECT_EQ(predictor->feature_map_["metrics.observedDomContentLoaded"], 0);
  EXPECT_EQ(predictor->feature_map_["metrics.observedFirstVisualChange"], 0);
  EXPECT_EQ(predictor->feature_map_["metrics.observedLoad"], 0);
  EXPECT_EQ(predictor->feature_map_["metrics.interactive"], 0);

  auto timing = page_load_metrics::CreatePageLoadTiming();
  timing->document_timing->dom_content_loaded_event_start =
      base::TimeDelta::FromMilliseconds(1000);
  predictor->OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor->feature_map_["metrics.observedDomContentLoaded"], 1000);

  timing->document_timing->load_event_start =
      base::TimeDelta::FromMilliseconds(2000);
  predictor->OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor->feature_map_["metrics.observedLoad"], 2000);

  timing->paint_timing->first_meaningful_paint =
      base::TimeDelta::FromMilliseconds(1500);
  predictor->OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor->feature_map_["metrics.firstMeaningfulPaint"], 1500);

  timing->paint_timing->first_contentful_paint =
      base::TimeDelta::FromMilliseconds(800);
  predictor->OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor->feature_map_["metrics.observedFirstVisualChange"], 800);

  timing->interactive_timing->interactive =
      base::TimeDelta::FromMilliseconds(2500);
  predictor->OnPageLoadTimingUpdated(*timing);
  EXPECT_EQ(predictor->feature_map_["metrics.interactive"], 2500);
}

TEST(BandwidthSavingsPredictorTest, FeaturiseResourceLoading) {
  auto* predictor = new BandwidthSavingsPredictor();
  EXPECT_EQ(predictor->feature_map_["resources.third-party.requestCount"], 0);

  const GURL main_frame("https://brave.com/");

  auto fpStyle = predictors::CreateResourceLoadInfo(
      "https://brave.com/style.css", content::ResourceType::kStylesheet);
  fpStyle->raw_body_bytes = 1000;
  predictor->OnResourceLoadComplete(main_frame, *fpStyle);
  EXPECT_EQ(predictor->feature_map_["resources.third-party.requestCount"], 0);
  EXPECT_EQ(predictor->feature_map_["resources.stylesheet.requestCount"], 1);
  EXPECT_EQ(predictor->feature_map_["resources.stylesheet.size"], 1000);

  auto tpStyle = predictors::CreateResourceLoadInfo(
      "https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.js",
      content::ResourceType::kScript);
  tpStyle->raw_body_bytes = 1001;
  predictor->OnResourceLoadComplete(main_frame, *tpStyle);

  EXPECT_EQ(predictor->feature_map_["resources.third-party.requestCount"], 1);
  EXPECT_EQ(predictor->feature_map_["resources.stylesheet.requestCount"], 1);
  EXPECT_EQ(predictor->feature_map_["resources.script.requestCount"], 1);
  EXPECT_EQ(predictor->feature_map_["resources.stylesheet.size"], 1000);
  EXPECT_EQ(predictor->feature_map_["resources.script.size"], 1001);

  EXPECT_EQ(predictor->feature_map_["resources.total.requestCount"], 2);
  EXPECT_EQ(predictor->feature_map_["resources.total.size"], 2001);
}

TEST(BandwidthSavingsPredictorTest, PredictZeroNoData) {
  auto* predictor = new BandwidthSavingsPredictor();
  EXPECT_EQ(predictor->PredictSavingsBytes(), 0);
}

TEST(BandwidthSavingsPredictorTest, PredictZeroInternalUrl) {
  auto* predictor = new BandwidthSavingsPredictor();

  const GURL main_frame("brave://version");
  auto res = predictors::CreateResourceLoadInfo("brave://version");
  predictor->OnResourceLoadComplete(main_frame, *res);

  EXPECT_EQ(predictor->PredictSavingsBytes(), 0);
}

TEST(BandwidthSavingsPredictorTest, PredictZeroBadFrame) {
  auto* predictor = new BandwidthSavingsPredictor();

  const GURL main_frame("");
  auto res = predictors::CreateResourceLoadInfo(
      "https://brave.com/style.css", content::ResourceType::kStylesheet);
  res->raw_body_bytes = 1000;
  predictor->OnResourceLoadComplete(main_frame, *res);

  EXPECT_EQ(predictor->PredictSavingsBytes(), 0);
}

TEST(BandwidthSavingsPredictorTest, PredictZeroNoBlocks) {
  auto* predictor = new BandwidthSavingsPredictor();

  const GURL main_frame("https://brave.com");
  auto res = predictors::CreateResourceLoadInfo(
      "https://brave.com/style.css", content::ResourceType::kStylesheet);
  res->raw_body_bytes = 1000;
  predictor->OnResourceLoadComplete(main_frame, *res);

  EXPECT_EQ(predictor->PredictSavingsBytes(), 0);
}

TEST(BandwidthSavingsPredictorTest, PredictNonZero) {
  auto* predictor = new BandwidthSavingsPredictor();

  const GURL main_frame("https://brave.com");
  auto res = predictors::CreateResourceLoadInfo(
      "https://brave.com/style.css", content::ResourceType::kStylesheet);
  res->raw_body_bytes = 1000;
  predictor->OnResourceLoadComplete(main_frame, *res);

  predictor->OnSubresourceBlocked("https://google-analytics.com/ga.js");
  // resource still seen as complete, but with 0 bytes
  auto blocked = predictors::CreateResourceLoadInfo(
      "https://google-analytics.com/ga.js", content::ResourceType::kScript);
  blocked->raw_body_bytes = 0;
  predictor->OnResourceLoadComplete(main_frame, *blocked);

  EXPECT_NE(predictor->PredictSavingsBytes(), 0);
}

}  // namespace brave_perf_predictor
