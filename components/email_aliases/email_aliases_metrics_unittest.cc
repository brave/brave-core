/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_metrics.h"

#include "base/strings/string_number_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "brave/components/email_aliases/email_aliases_notes.h"
#include "brave/components/email_aliases/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace email_aliases {

class EmailAliasesMetricsTest : public testing::Test {
 public:
  EmailAliasesMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    EmailAliasesMetrics::RegisterProfilePrefs(pref_service_.registry());
    EmailAliasesNotes::RegisterProfilePrefs(pref_service_.registry());
    metrics_ = std::make_unique<EmailAliasesMetrics>(pref_service_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<EmailAliasesMetrics> metrics_;
};

TEST_F(EmailAliasesMetricsTest, NotesCount) {
  // With aliases present and no notes, reports bucket 0.
  metrics_->ReportEmailAliasPresence(true);
  histogram_tester_.ExpectUniqueSample(kNotesCountHistogramName, 0, 1);

  // 1 note = bucket 1 (1-5).
  EmailAliasesNotes notes(pref_service_, "user@example.com");
  notes.UpdateNote("alias@example.com", "note");
  histogram_tester_.ExpectBucketCount(kNotesCountHistogramName, 1, 1);

  // 6 notes = bucket 2 (6-15).
  for (int i = 1; i <= 5; i++) {
    notes.UpdateNote("alias" + base::NumberToString(i) + "@example.com",
                     "note");
  }
  histogram_tester_.ExpectBucketCount(kNotesCountHistogramName, 2, 1);

  // Without aliases present, further note changes should not report.
  metrics_->ReportEmailAliasPresence(false);
  histogram_tester_.ExpectTotalCount(kNotesCountHistogramName, 7);
  notes.UpdateNote("alias100@example.com", "note");
  histogram_tester_.ExpectTotalCount(kNotesCountHistogramName, 7);
}

TEST_F(EmailAliasesMetricsTest, CopyCount) {
  metrics_->ReportEmailAliasPresence(true);
  histogram_tester_.ExpectTotalCount(kClipboardCopyCountHistogramName, 0);

  // 3 copies = bucket 1 (1-5). Running sum recorded each call:
  // call 1: sum=1 -> bucket 1, call 2: sum=2 -> bucket 1, call 3: sum=3 ->
  // bucket 1.
  metrics_->OnAliasCopied();
  metrics_->OnAliasCopied();
  metrics_->OnAliasCopied();
  histogram_tester_.ExpectUniqueSample(kClipboardCopyCountHistogramName, 1, 3);

  // 6 more copies pushes sum to 9 = bucket 2 (6-15).
  for (int i = 0; i < 6; i++) {
    metrics_->OnAliasCopied();
  }
  histogram_tester_.ExpectBucketCount(kClipboardCopyCountHistogramName, 2, 4);

  // Timer fires daily for 14 days — 7 additional reports while in weekly
  // window.
  task_environment_.FastForwardBy(base::Days(14));
  histogram_tester_.ExpectTotalCount(kClipboardCopyCountHistogramName, 15);
}

TEST_F(EmailAliasesMetricsTest, SettingsPageNavigation) {
  metrics_->RecordSettingsPageNavigation(SettingsPageMethod::kManualNavigation);
  histogram_tester_.ExpectUniqueSample(kSettingsPageMethodHistogramName,
                                       SettingsPageMethod::kManualNavigation,
                                       1);

  // Second call and existing users with aliases should be suppressed.
  metrics_->RecordSettingsPageNavigation(SettingsPageMethod::kAppMenu);
  histogram_tester_.ExpectTotalCount(kSettingsPageMethodHistogramName, 1);

  metrics_->ReportEmailAliasPresence(true);
  metrics_->RecordSettingsPageNavigation(SettingsPageMethod::kAppMenu);
  histogram_tester_.ExpectTotalCount(kSettingsPageMethodHistogramName, 1);
}

TEST_F(EmailAliasesMetricsTest, ReportAllMetricsOnTimer) {
  metrics_->ReportEmailAliasPresence(true);
  histogram_tester_.ExpectTotalCount(kNotesCountHistogramName, 1);

  task_environment_.FastForwardBy(base::Days(1));

  histogram_tester_.ExpectTotalCount(kNotesCountHistogramName, 2);
}

}  // namespace email_aliases
