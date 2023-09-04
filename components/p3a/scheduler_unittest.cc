// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/scheduler.h"

#include <memory>

#include "base/test/bind.h"
#include "base/time/time.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr base::TimeDelta kAverageUploadInterval = base::Minutes(1);

}  // namespace

class P3ASchedulerTest : public testing::Test {
 public:
  P3ASchedulerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUpScheduler(bool randomize_upload_interval) {
    scheduler_ = std::make_unique<Scheduler>(base::BindLambdaForTesting([&]() {
                                               upload_count_++;
                                               scheduler_->UploadFinished(true);
                                             }),
                                             randomize_upload_interval,
                                             kAverageUploadInterval);
    scheduler_->Start();
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<Scheduler> scheduler_;
  size_t upload_count_ = 0;
};

TEST_F(P3ASchedulerTest, NonRandom) {
  SetUpScheduler(false);

  task_environment_.FastForwardBy(base::Minutes(6));
  EXPECT_EQ(upload_count_, 6u);

  task_environment_.FastForwardBy(base::Minutes(10));
  EXPECT_EQ(upload_count_, 16u);
}

TEST_F(P3ASchedulerTest, Random) {
  SetUpScheduler(true);

  task_environment_.FastForwardBy(base::Hours(8));
  size_t first_upload_count = upload_count_;
  upload_count_ = 0;

  task_environment_.FastForwardBy(base::Hours(8));
  EXPECT_NE(upload_count_, first_upload_count);
}

}  // namespace p3a
