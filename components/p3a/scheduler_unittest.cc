// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/scheduler.h"

#include <memory>

#include "base/test/bind.h"
#include "base/time/time.h"
#include "brave/components/p3a/p3a_config.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr base::TimeDelta kAverageUploadInterval = base::Minutes(1);

base::TimeDelta InitialInterval() {
  return base::Seconds(Scheduler::GetInitialIntervalSeconds());
}

}  // namespace

class P3ASchedulerTest : public testing::Test {
 public:
  P3ASchedulerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUpScheduler(bool randomize_upload_interval,
                      bool priority_messages_pending = false) {
    scheduler_ = std::make_unique<Scheduler>(
        base::BindLambdaForTesting([&]() {
          upload_count_++;
          scheduler_->UploadFinished(upload_ok_, priority_messages_pending_);
        }),
        randomize_upload_interval, kAverageUploadInterval,
        kAveragePrepPriorityInterval);
    scheduler_->Start(priority_messages_pending);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<Scheduler> scheduler_;
  size_t upload_count_ = 0;
  // Values reported back to the scheduler once a task completes.
  bool upload_ok_ = true;
  bool priority_messages_pending_ = false;
};

TEST_F(P3ASchedulerTest, NonRandom) {
  SetUpScheduler(false);

  task_environment_.FastForwardBy(base::Minutes(6) + base::Seconds(1));
  EXPECT_EQ(upload_count_, 6u);

  task_environment_.FastForwardBy(base::Minutes(10) + base::Seconds(1));
  EXPECT_EQ(upload_count_, 16u);
}

TEST_F(P3ASchedulerTest, StartWithoutPriorityKeepsInitialInterval) {
  SetUpScheduler(false);

  task_environment_.FastForwardBy(InitialInterval() - base::Seconds(4));
  EXPECT_EQ(upload_count_, 0u);

  task_environment_.FastForwardBy(base::Seconds(4));
  EXPECT_EQ(upload_count_, 1u);
}

TEST_F(P3ASchedulerTest, StartWithPriorityUsesPriorityInterval) {
  SetUpScheduler(false, /*priority_messages_pending=*/true);

  task_environment_.FastForwardBy(kAveragePrepPriorityInterval -
                                  base::Seconds(4));
  EXPECT_EQ(upload_count_, 0u);

  task_environment_.FastForwardBy(base::Seconds(4));
  EXPECT_EQ(upload_count_, 1u);
}

TEST_F(P3ASchedulerTest, StartAppliesPriorityToPendingTask) {
  SetUpScheduler(false);

  task_environment_.FastForwardBy(base::Seconds(1));
  ASSERT_EQ(upload_count_, 0u);

  // The pending initial interval should be replaced with the priority one.
  scheduler_->Start(/*priority_messages_pending=*/true);

  task_environment_.FastForwardBy(kAveragePrepPriorityInterval -
                                  base::Seconds(4));
  EXPECT_EQ(upload_count_, 0u);

  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_EQ(upload_count_, 1u);
}

TEST_F(P3ASchedulerTest, PriorityIntervalRepeatsThenRevertsToStandard) {
  priority_messages_pending_ = true;
  SetUpScheduler(false, /*priority_messages_pending=*/true);

  task_environment_.FastForwardBy(kAveragePrepPriorityInterval * 5 +
                                  base::Seconds(1));
  EXPECT_EQ(upload_count_, 5u);

  // The last priority message was drained by the task above.
  priority_messages_pending_ = false;
  task_environment_.FastForwardBy(kAveragePrepPriorityInterval +
                                  base::Seconds(1));
  ASSERT_EQ(upload_count_, 6u);

  task_environment_.FastForwardBy(kAverageUploadInterval - base::Seconds(4));
  EXPECT_EQ(upload_count_, 6u);
  task_environment_.FastForwardBy(base::Seconds(4));
  EXPECT_EQ(upload_count_, 7u);
}

TEST_F(P3ASchedulerTest, ExpediteShortensPendingInterval) {
  SetUpScheduler(false);

  task_environment_.FastForwardBy(base::Seconds(1));
  scheduler_->ExpediteForPriority();

  task_environment_.FastForwardBy(kAveragePrepPriorityInterval -
                                  base::Seconds(4));
  EXPECT_EQ(upload_count_, 0u);

  task_environment_.FastForwardBy(base::Seconds(4));
  EXPECT_EQ(upload_count_, 1u);
}

TEST_F(P3ASchedulerTest, ExpediteIsNoOpWhenAlreadyExpedited) {
  SetUpScheduler(false);

  scheduler_->ExpediteForPriority();
  // Repeated calls must not keep pushing the pending task out.
  for (int i = 0; i < 5; i++) {
    task_environment_.FastForwardBy(base::Seconds(1));
    scheduler_->ExpediteForPriority();
  }

  task_environment_.FastForwardBy(kAveragePrepPriorityInterval -
                                  base::Seconds(5));
  EXPECT_EQ(upload_count_, 1u);
}

TEST_F(P3ASchedulerTest, ExpediteRevertsToStandardInterval) {
  SetUpScheduler(false);

  scheduler_->ExpediteForPriority();
  task_environment_.FastForwardBy(kAveragePrepPriorityInterval +
                                  base::Seconds(1));
  ASSERT_EQ(upload_count_, 1u);

  // No priority messages were reported, so the standard interval applies again.
  task_environment_.FastForwardBy(kAverageUploadInterval - base::Seconds(4));
  EXPECT_EQ(upload_count_, 1u);
  task_environment_.FastForwardBy(base::Seconds(4));
  EXPECT_EQ(upload_count_, 2u);
}

TEST_F(P3ASchedulerTest, ExpediteIsNoOpWhileStopped) {
  SetUpScheduler(false);
  scheduler_->Stop();

  scheduler_->ExpediteForPriority();
  task_environment_.FastForwardBy(kAverageUploadInterval);
  EXPECT_EQ(upload_count_, 0u);
}

TEST_F(P3ASchedulerTest, BackoffOnFailure) {
  upload_ok_ = false;
  SetUpScheduler(false);

  task_environment_.FastForwardBy(InitialInterval() + base::Seconds(1));
  ASSERT_EQ(upload_count_, 1u);

  // Backoff starts at 5s and doubles on each failure.
  task_environment_.FastForwardBy(base::Seconds(6));
  ASSERT_EQ(upload_count_, 2u);

  task_environment_.FastForwardBy(base::Seconds(11));
  ASSERT_EQ(upload_count_, 3u);

  task_environment_.FastForwardBy(base::Seconds(21));
  ASSERT_EQ(upload_count_, 4u);
}

TEST_F(P3ASchedulerTest, BackoffTakesPrecedenceOverPriority) {
  upload_ok_ = false;
  priority_messages_pending_ = true;
  SetUpScheduler(false, /*priority_messages_pending=*/true);

  task_environment_.FastForwardBy(kAveragePrepPriorityInterval +
                                  base::Seconds(1));
  ASSERT_EQ(upload_count_, 1u);

  // The failure must schedule the retry at the 5s backoff interval, rather
  // than at the priority interval.
  task_environment_.FastForwardBy(base::Seconds(3));
  EXPECT_EQ(upload_count_, 1u);
  task_environment_.FastForwardBy(base::Seconds(1));
  EXPECT_EQ(upload_count_, 2u);
}

TEST_F(P3ASchedulerTest, BackoffResetsAfterSuccess) {
  upload_ok_ = false;
  SetUpScheduler(false);

  task_environment_.FastForwardBy(InitialInterval() + base::Seconds(1));
  ASSERT_EQ(upload_count_, 1u);
  task_environment_.FastForwardBy(base::Seconds(6));
  ASSERT_EQ(upload_count_, 2u);

  upload_ok_ = true;
  task_environment_.FastForwardBy(base::Seconds(11));
  ASSERT_EQ(upload_count_, 3u);

  // The next failure should start over at the initial backoff interval.
  upload_ok_ = false;
  task_environment_.FastForwardBy(kAverageUploadInterval + base::Seconds(1));
  ASSERT_EQ(upload_count_, 4u);
  task_environment_.FastForwardBy(base::Seconds(6));
  EXPECT_EQ(upload_count_, 5u);
}

}  // namespace p3a
