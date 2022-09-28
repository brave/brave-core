// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

namespace {
constexpr char kTestCreativeMetricId[] = "2ba3659a-4737-4c4e-892a-a6a2e0e2a871";
constexpr char kHistogramPrefix[] = "creativeInstanceId.";

constexpr char kClicksEventType[] = "clicks";
constexpr char kViewsEventType[] = "views";
constexpr char kLandsEventType[] = "lands";
}  // namespace

class NTPP3AHelperImplTest : public testing::Test {
 public:
  NTPP3AHelperImplTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    histogram_tester_ = std::make_unique<base::HistogramTester>();

    brave::BraveP3AService::RegisterPrefs(local_state_.registry(),
                                          /*first_run*/ false);
    NTPP3AHelperImpl::RegisterLocalStatePrefs(local_state_.registry());

    p3a_service_ = scoped_refptr(
        new brave::BraveP3AService(&local_state_, "release", "2049-01-01"));

    ntp_p3a_helper_.reset(
        new NTPP3AHelperImpl(&local_state_, p3a_service_.get()));
  }

  std::string GetExpectedHistogramName(const std::string& event_type) {
    return base::StrCat(
        {kHistogramPrefix, kTestCreativeMetricId, ".", event_type});
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;

  scoped_refptr<brave::BraveP3AService> p3a_service_;
  TestingPrefServiceSimple local_state_;

  std::unique_ptr<NTPP3AHelperImpl> ntp_p3a_helper_;
};

TEST_F(NTPP3AHelperImplTest, OneEventTypeCountReported) {
  ntp_p3a_helper_->RecordView(kTestCreativeMetricId);

  const std::string histogram_name = GetExpectedHistogramName(kViewsEventType);

  ASSERT_TRUE(p3a_service_->IsDynamicMetricRegistered(histogram_name));

  histogram_tester_->ExpectTotalCount(histogram_name, 0);

  // Mock a P3A rotation to trigger just-in-time collection of metrics
  ntp_p3a_helper_->OnP3ARotation(/*is_express*/ true);

  histogram_tester_->ExpectBucketCount(histogram_name, 1, 1);

  ntp_p3a_helper_->RecordView(kTestCreativeMetricId);
  ntp_p3a_helper_->OnP3ARotation(/*is_express*/ true);

  histogram_tester_->ExpectBucketCount(histogram_name, 2, 1);
  ASSERT_TRUE(p3a_service_->IsDynamicMetricRegistered(histogram_name));

  ntp_p3a_helper_->OnP3AMetricSent(histogram_name);

  histogram_tester_->ExpectTotalCount(histogram_name, 2);
  ASSERT_FALSE(p3a_service_->IsDynamicMetricRegistered(histogram_name));
}

TEST_F(NTPP3AHelperImplTest, OneEventTypeCountReportedWhileInflight) {
  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);
  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);

  const std::string histogram_name = GetExpectedHistogramName(kClicksEventType);

  ASSERT_TRUE(p3a_service_->IsDynamicMetricRegistered(histogram_name));

  histogram_tester_->ExpectTotalCount(histogram_name, 0);

  ntp_p3a_helper_->OnP3ARotation(/*is_express*/ true);

  histogram_tester_->ExpectBucketCount(histogram_name, 2, 1);

  // Recorded a click while recorded count is "in-flight"
  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);
  ntp_p3a_helper_->OnP3AMetricSent(histogram_name);

  ASSERT_TRUE(p3a_service_->IsDynamicMetricRegistered(histogram_name));

  ntp_p3a_helper_->OnP3ARotation(/*is_express*/ true);
  histogram_tester_->ExpectBucketCount(histogram_name, 1, 1);

  ntp_p3a_helper_->OnP3AMetricSent(histogram_name);

  ASSERT_FALSE(p3a_service_->IsDynamicMetricRegistered(histogram_name));
  histogram_tester_->ExpectTotalCount(histogram_name, 2);
}

TEST_F(NTPP3AHelperImplTest, LandCountReported) {
  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);

  const std::string histogram_name = GetExpectedHistogramName(kLandsEventType);

  ASSERT_FALSE(p3a_service_->IsDynamicMetricRegistered(histogram_name));

  ntp_p3a_helper_->SetLastTabURL(GURL("https://adexample.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(6));

  // It's acceptable to access other pages,
  // as long as they're on the same host.
  ntp_p3a_helper_->SetLastTabURL(GURL("https://adexample.com/page2"));

  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_->ExpectTotalCount(histogram_name, 0);

  ntp_p3a_helper_->OnP3ARotation(/*is_express*/ true);
  ASSERT_TRUE(p3a_service_->IsDynamicMetricRegistered(histogram_name));
  histogram_tester_->ExpectBucketCount(histogram_name, 1, 1);

  ntp_p3a_helper_->RecordClickAndMaybeLand(kTestCreativeMetricId);

  ntp_p3a_helper_->SetLastTabURL(GURL("https://adexample.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(6));
  histogram_tester_->ExpectTotalCount(histogram_name, 1);

  // Should not trigger land, since user left page before "land time"
  ntp_p3a_helper_->SetLastTabURL(GURL("https://differenthost.com/page1"));

  task_environment_.FastForwardBy(base::Seconds(5));

  ntp_p3a_helper_->OnP3ARotation(/*is_express*/ true);
  histogram_tester_->ExpectBucketCount(histogram_name, 1, 2);

  ASSERT_TRUE(p3a_service_->IsDynamicMetricRegistered(histogram_name));
  ntp_p3a_helper_->OnP3AMetricSent(histogram_name);
  ASSERT_FALSE(p3a_service_->IsDynamicMetricRegistered(histogram_name));
}

}  // namespace ntp_background_images
