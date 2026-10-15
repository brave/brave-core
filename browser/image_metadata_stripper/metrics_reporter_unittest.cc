/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/image_metadata_stripper/metrics_reporter.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace image_metadata_stripper::metrics {

class ImageMetadataStripperMetricsReporterTest : public testing::Test {
 public:
  ImageMetadataStripperMetricsReporterTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~ImageMetadataStripperMetricsReporterTest() override = default;

  void SetUp() override {
    MetricsReporter::RegisterPrefs(pref_service_.registry());
    CreateReporter();
  }

  // (Re)creates the reporter sharing the same local state. This mirrors a
  // browser process restart, which re-runs the construction-time MaybeReport().
  void CreateReporter() {
    reporter_ = std::make_unique<MetricsReporter>(&pref_service_);
  }

  void RecordFbmd(StrippingClient client, bool was_stripped) {
    reporter_->MaybeRecordFbmd(client, was_stripped);
  }

  void ExpectNoSamples() {
    histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                       0);
    histogram_tester_.ExpectTotalCount(kUploadFBMDCountStrippedHistogramName,
                                       0);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<MetricsReporter> reporter_;
  base::HistogramTester histogram_tester_;
};

TEST_F(ImageMetadataStripperMetricsReporterTest, DoesNotReportOnConstruction) {
  ExpectNoSamples();
}

TEST_F(ImageMetadataStripperMetricsReporterTest, DoesNotReportUntilInterval) {
  RecordFbmd(StrippingClient::kDownloadManager, /*was_stripped=*/true);
  RecordFbmd(StrippingClient::kFileSelect, /*was_stripped=*/true);
  task_environment_.FastForwardBy(base::Days(6));

  ExpectNoSamples();
}

TEST_F(ImageMetadataStripperMetricsReporterTest,
       DoesNotRereportOnRestartWithinInterval) {
  RecordFbmd(StrippingClient::kDownloadManager, /*was_stripped=*/true);

  // A restart within the reporting window must not emit; it only reschedules
  // the timer for the remainder of the window.
  CreateReporter();

  ExpectNoSamples();
}

TEST_F(ImageMetadataStripperMetricsReporterTest, ReportsDownloadCount) {
  RecordFbmd(StrippingClient::kDownloadManager, /*was_stripped=*/true);
  task_environment_.FastForwardBy(base::Days(7));

  // A single stripped download falls in the 1-5 bucket (index 0).
  histogram_tester_.ExpectBucketCount(kDownloadFBMDCountStrippedHistogramName,
                                      0, 1);
  histogram_tester_.ExpectTotalCount(kUploadFBMDCountStrippedHistogramName, 0);
}

TEST_F(ImageMetadataStripperMetricsReporterTest,
       ReportsUploadCountFromBothUploadFlows) {
  // Both the file-select and drag-drop flows aggregate into the upload count.
  RecordFbmd(StrippingClient::kFileSelect, /*was_stripped=*/true);
  RecordFbmd(StrippingClient::kDragDrop, /*was_stripped=*/true);
  task_environment_.FastForwardBy(base::Days(7));

  // Two stripped uploads fall in the 1-5 bucket (index 0).
  histogram_tester_.ExpectBucketCount(kUploadFBMDCountStrippedHistogramName, 0,
                                      1);
  histogram_tester_.ExpectTotalCount(kDownloadFBMDCountStrippedHistogramName,
                                     0);
}

TEST_F(ImageMetadataStripperMetricsReporterTest, DoesNotRecordWhenNotStripped) {
  // An observed FBMD image that was not stripped must not emit anything.
  RecordFbmd(StrippingClient::kDownloadManager, /*was_stripped=*/false);
  RecordFbmd(StrippingClient::kFileSelect, /*was_stripped=*/false);
  task_environment_.FastForwardBy(base::Days(7));

  ExpectNoSamples();
}

TEST_F(ImageMetadataStripperMetricsReporterTest, BucketsCounts) {
  // 6 stripped downloads fall in the 6-10 bucket (index 1).
  for (int i = 0; i < 6; ++i) {
    RecordFbmd(StrippingClient::kDownloadManager, /*was_stripped=*/true);
  }
  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectBucketCount(kDownloadFBMDCountStrippedHistogramName,
                                      1, 1);
}

TEST_F(ImageMetadataStripperMetricsReporterTest, StoresCountsInNestedDict) {
  RecordFbmd(StrippingClient::kDownloadManager, /*was_stripped=*/true);
  RecordFbmd(StrippingClient::kDownloadManager, /*was_stripped=*/true);
  RecordFbmd(StrippingClient::kFileSelect, /*was_stripped=*/true);

  const base::DictValue& root = pref_service_.GetDict(kRootPrefKey);

  const base::DictValue* download = root.FindDict(kDownloadPrefKey);
  ASSERT_TRUE(download);
  const base::DictValue* download_fbmd =
      download->FindDict(kFBMDMetadataPrefKey);
  ASSERT_TRUE(download_fbmd);
  EXPECT_EQ(download_fbmd->FindInt(kStrippedPrefKey), 2);

  const base::DictValue* upload = root.FindDict(kUploadPrefKey);
  ASSERT_TRUE(upload);
  const base::DictValue* upload_fbmd = upload->FindDict(kFBMDMetadataPrefKey);
  ASSERT_TRUE(upload_fbmd);
  EXPECT_EQ(upload_fbmd->FindInt(kStrippedPrefKey), 1);
}

TEST_F(ImageMetadataStripperMetricsReporterTest, ClearsCountsAfterReporting) {
  RecordFbmd(StrippingClient::kDownloadManager, /*was_stripped=*/true);
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
