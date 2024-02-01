/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_ads/browser/analytics/p3a/notification_ad.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads {

class NotificationAdP3ATest : public testing::Test {
 public:
  void SetUp() override {
    pref_service_.registry()->RegisterDoublePref(
        prefs::kNotificationAdLastNormalizedCoordinateX, 0.0);
    pref_service_.registry()->RegisterDoublePref(
        prefs::kNotificationAdLastNormalizedCoordinateY, 0.0);
    pref_service_.registry()->RegisterBooleanPref(
        prefs::kOptedInToNotificationAds, false);
  }

 protected:
  void SetNotificationPosition(double x, double y) {
    pref_service_.SetDouble(prefs::kNotificationAdLastNormalizedCoordinateX, x);
    pref_service_.SetDouble(prefs::kNotificationAdLastNormalizedCoordinateY, y);
  }

  void EnableAdNotifications() {
    pref_service_.SetBoolean(prefs::kOptedInToNotificationAds, true);
  }

  base::HistogramTester histogram_tester_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(NotificationAdP3ATest, CustomNotificationsDisabled) {
  RecordNotificationAdPositionMetric(false, &pref_service_);
  histogram_tester_.ExpectUniqueSample(kNotificationAdPositionHistogramName,
                                       INT_MAX - 1, 1);

  RecordNotificationAdPositionMetric(true, &pref_service_);
  histogram_tester_.ExpectUniqueSample(kNotificationAdPositionHistogramName,
                                       INT_MAX - 1, 2);

  EnableAdNotifications();
  RecordNotificationAdPositionMetric(true, &pref_service_);
  histogram_tester_.ExpectUniqueSample(kNotificationAdPositionHistogramName,
                                       INT_MAX - 1, 3);
  histogram_tester_.ExpectTotalCount(kNotificationAdPositionHistogramName, 3);
}

TEST_F(NotificationAdP3ATest, CustomNotificationsEnabled) {
  std::pair<std::pair<double, double>, int> cases[] = {
      {{0.15, 0.28}, 1},
      {{0.42, 0.1}, 2},
      {{0.73, 0.19}, 3},
      {{0.2, 0.45}, 4},
      {{0.61, 0.52}, INT_MAX - 1},
      {{0.71, 0.52}, 5},
      {{0.02, 0.91}, 6},
      {{0.66, 0.69}, 7},
      {{0.91, 0.9}, 8}};
  EnableAdNotifications();
  for (auto [position, answer] : cases) {
    auto [x, y] = position;
    SetNotificationPosition(x, y);
    RecordNotificationAdPositionMetric(true, &pref_service_);
    histogram_tester_.ExpectBucketCount(kNotificationAdPositionHistogramName,
                                        answer, 1);
  }

  histogram_tester_.ExpectTotalCount(kNotificationAdPositionHistogramName, 9);
}

}  // namespace brave_ads
