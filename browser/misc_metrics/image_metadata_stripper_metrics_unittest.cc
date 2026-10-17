/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/image_metadata_stripper_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace image_metadata_stripper::metrics {

class ImageMetadataStripperMetricsTest : public testing::Test {
 public:
  ImageMetadataStripperMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~ImageMetadataStripperMetricsTest() override = default;

  void SetUp() override {
    ImageMetadataStripperMetrics::RegisterPrefs(pref_service_.registry());
    CreateReporter();
  }

  // (Re)creates the reporter sharing the same local state. This mirrors a
  // browser process restart, which re-runs the construction-time MaybeReport().
  void CreateReporter() {
    reporter_ = std::make_unique<ImageMetadataStripperMetrics>(&pref_service_);
  }

  void RecordFbmd(StrippingClient client) { reporter_->RecordFbmd(client); }

  void ExpectNoSamples() {
    histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                       0);
    histogram_tester_.ExpectTotalCount(kUploadFBMDCountStrippedHistogramName,
                                       0);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<ImageMetadataStripperMetrics> reporter_;
  base::HistogramTester histogram_tester_;
};

TEST_F(ImageMetadataStripperMetricsTest, DoesNotReportOnConstruction) {
  ExpectNoSamples();
}

TEST_F(ImageMetadataStripperMetricsTest, DoesNotReportUntilInterval) {
  RecordFbmd(StrippingClient::kDownloadManager);
  RecordFbmd(StrippingClient::kFileSelect);
  task_environment_.FastForwardBy(base::Days(6));

  ExpectNoSamples();
}

TEST_F(ImageMetadataStripperMetricsTest,
       DoesNotRereportOnRestartWithinInterval) {
  RecordFbmd(StrippingClient::kDownloadManager);

  // A restart within the reporting window must not emit; it only reschedules
  // the timer for the remainder of the window.
  CreateReporter();

  ExpectNoSamples();
}

TEST_F(ImageMetadataStripperMetricsTest,
       RestartWithinIntervalKeepsOriginalWindow) {
  RecordFbmd(StrippingClient::kDownloadManager);
  task_environment_.FastForwardBy(base::Days(6));

  CreateReporter();
  ExpectNoSamples();

  // The report still fires at the end of the original window.
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                     1);

  // And the following window reports on schedule as well.
  RecordFbmd(StrippingClient::kDownloadManager);
  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                     2);
}

TEST_F(ImageMetadataStripperMetricsTest, ClockMovedBackStartsNewWindow) {
  RecordFbmd(StrippingClient::kDownloadManager);

  // Mock time cannot run backwards, so simulate the clock having been moved
  // back by storing a last recorded time that lies in the future.
  pref_service_.SetTime(kLastRecordedTimePref,
                        base::Time::Now() + base::Days(30));
  CreateReporter();

  // The future window is treated as expired: pending counts are reported and
  // a new window starts now.
  histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                     1);
  EXPECT_EQ(pref_service_.GetTime(kLastRecordedTimePref), base::Time::Now());
  EXPECT_TRUE(pref_service_.GetDict(kRootPrefKey).empty());

  // The next report follows the new window rather than waiting for the
  // future timestamp.
  RecordFbmd(StrippingClient::kDownloadManager);
  task_environment_.FastForwardBy(base::Days(6));
  histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                     1);
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                     2);
}

TEST_F(ImageMetadataStripperMetricsTest, ReportsDownloadCount) {
  RecordFbmd(StrippingClient::kDownloadManager);
  task_environment_.FastForwardBy(base::Days(7));

  // A single stripped download falls in the 1-5 bucket (index 0).
  histogram_tester_.ExpectBucketCount(kDownloadFBMDCountStrippedHistogramName,
                                      0, 1);
  histogram_tester_.ExpectTotalCount(kUploadFBMDCountStrippedHistogramName, 0);
}

TEST_F(ImageMetadataStripperMetricsTest,
       ReportsUploadCountFromBothUploadFlows) {
  // Both the file-select and drag-drop flows aggregate into the upload count.
  RecordFbmd(StrippingClient::kFileSelect);
  RecordFbmd(StrippingClient::kDragDrop);
  task_environment_.FastForwardBy(base::Days(7));

  // Two stripped uploads fall in the 1-5 bucket (index 0).
  histogram_tester_.ExpectBucketCount(kUploadFBMDCountStrippedHistogramName, 0,
                                      1);
  histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                     0);
}

TEST_F(ImageMetadataStripperMetricsTest, BucketsCounts) {
  // 6 stripped downloads fall in the 6-10 bucket (index 1).
  for (int i = 0; i < 6; ++i) {
    RecordFbmd(StrippingClient::kDownloadManager);
  }
  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kDownloadFBMDCountStrippedHistogramName,
                                      1, 1);
}

TEST_F(ImageMetadataStripperMetricsTest, StoresCountsInFbmdDict) {
  RecordFbmd(StrippingClient::kDownloadManager);
  RecordFbmd(StrippingClient::kDownloadManager);
  RecordFbmd(StrippingClient::kFileSelect);

  const base::DictValue& root = pref_service_.GetDict(kRootPrefKey);

  const base::DictValue* fbmd = root.FindDict(kFbmdPrefKey);
  ASSERT_TRUE(fbmd);
  EXPECT_EQ(fbmd->FindInt(kDownloadStrippedPrefKey), 2);
  EXPECT_EQ(fbmd->FindInt(kUploadStrippedPrefKey), 1);
}

TEST_F(ImageMetadataStripperMetricsTest, ClearsCountsAfterReporting) {
  RecordFbmd(StrippingClient::kDownloadManager);
  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kDownloadFBMDCountStrippedHistogramName,
                                      0, 1);
  // The stored counts are reset once reported.
  EXPECT_TRUE(pref_service_.GetDict(kRootPrefKey).empty());

  // An empty window emits nothing new; the previous sample is unchanged.
  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                     1);
}

}  // namespace image_metadata_stripper::metrics
