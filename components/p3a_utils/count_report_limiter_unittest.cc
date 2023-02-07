// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a_utils/count_report_limiter.h"

#include "base/time/time.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a_utils {

class CountReportLimiterTest : public testing::Test {
 public:
  CountReportLimiterTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        limiter_(30,
                 base::Seconds(1),
                 base::BindRepeating(&CountReportLimiterTest::OnReport,
                                     base::Unretained(this))),
        count_(0) {}

  void OnReport(uint64_t new_count) { count_ += new_count; }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  CountReportLimiter limiter_;
  uint64_t count_;
};

TEST_F(CountReportLimiterTest, Basic) {
  limiter_.Add(10);
  limiter_.Add(10);
  limiter_.Add(5);

  task_environment_.FastForwardBy(base::Seconds(1));

  EXPECT_EQ(count_, 25U);

  // Too many events, pausing report
  limiter_.Add(10);
  limiter_.Add(10);
  limiter_.Add(11);

  task_environment_.FastForwardBy(base::Seconds(1));

  EXPECT_EQ(count_, 25U);

  limiter_.Add(3);
  limiter_.Add(3);
  limiter_.Add(4);

  task_environment_.FastForwardBy(base::Seconds(1));

  EXPECT_EQ(count_, 35U);
}

}  // namespace p3a_utils
