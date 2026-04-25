/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_importer_p3a.h"

#include "base/test/metrics/histogram_tester.h"
#include "components/user_data_importer/common/importer_type.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveImporterP3ATest : public testing::Test {
 protected:
  base::HistogramTester histogram_tester_;
};

TEST_F(BraveImporterP3ATest, RecordChromeSource) {
  RecordImporterP3A(user_data_importer::TYPE_CHROME);
  histogram_tester_.ExpectBucketCount("Brave.Importer.ImporterSource.2",
                                      2 /* kChrome */, 1);
}

TEST_F(BraveImporterP3ATest, RecordEdgeSource) {
  RecordImporterP3A(user_data_importer::TYPE_EDGE_CHROMIUM);
  histogram_tester_.ExpectBucketCount("Brave.Importer.ImporterSource.2",
                                      4 /* kMicrosoft */, 1);
}

TEST_F(BraveImporterP3ATest, RecordOperaSource) {
  RecordImporterP3A(user_data_importer::TYPE_OPERA);
  histogram_tester_.ExpectBucketCount("Brave.Importer.ImporterSource.2",
                                      5 /* kOpera */, 1);
}

TEST_F(BraveImporterP3ATest, RecordBraveSource) {
  RecordImporterP3A(user_data_importer::TYPE_BRAVE);
  // TYPE_BRAVE should be bucketed as kOther (7), same as Vivaldi/Yandex/Whale.
  histogram_tester_.ExpectBucketCount("Brave.Importer.ImporterSource.2",
                                      7 /* kOther */, 1);
}

TEST_F(BraveImporterP3ATest, RecordVivaldiSource) {
  RecordImporterP3A(user_data_importer::TYPE_VIVALDI);
  histogram_tester_.ExpectBucketCount("Brave.Importer.ImporterSource.2",
                                      7 /* kOther */, 1);
}

TEST_F(BraveImporterP3ATest, RecordFirefoxSource) {
  RecordImporterP3A(user_data_importer::TYPE_FIREFOX);
  histogram_tester_.ExpectBucketCount("Brave.Importer.ImporterSource.2",
                                      3 /* kFirefox */, 1);
}

TEST_F(BraveImporterP3ATest, RecordBookmarksFileSource) {
  RecordImporterP3A(user_data_importer::TYPE_BOOKMARKS_FILE);
  histogram_tester_.ExpectBucketCount("Brave.Importer.ImporterSource.2",
                                      1 /* kBookmarksHTMLFile */, 1);
}
