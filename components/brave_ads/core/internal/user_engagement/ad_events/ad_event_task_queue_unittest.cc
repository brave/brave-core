/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_task_queue.h"

#include <utility>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/test/mock_callback.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdEventTaskQueueTest, ProcessTaskWhenAdded) {
  // Arrange
  AdEventTaskQueue queue;

  base::MockCallback<AdEventTask> task;
  base::test::TestFuture<bool> callback;
  EXPECT_CALL(task, Run).WillOnce([](ResultCallback callback) {
    std::move(callback).Run(/*success=*/true);
  });

  // Act
  queue.Add(task.Get(), callback.GetCallback());

  // Assert
  EXPECT_TRUE(callback.IsReady());
  EXPECT_TRUE(callback.Take());
}

TEST(BraveAdsAdEventTaskQueueTest, ProcessTaskAddedAfterPreviousTaskCompletes) {
  // Arrange
  AdEventTaskQueue queue;

  base::MockCallback<AdEventTask> first_task;
  base::test::TestFuture<bool> first_callback;
  EXPECT_CALL(first_task, Run).WillOnce([](ResultCallback callback) {
    std::move(callback).Run(/*success=*/true);
  });
  queue.Add(first_task.Get(), first_callback.GetCallback());
  ASSERT_TRUE(first_callback.Take());

  base::MockCallback<AdEventTask> second_task;
  base::test::TestFuture<bool> second_callback;
  EXPECT_CALL(second_task, Run).WillOnce([](ResultCallback callback) {
    std::move(callback).Run(/*success=*/true);
  });

  // Act
  queue.Add(second_task.Get(), second_callback.GetCallback());

  // Assert
  EXPECT_TRUE(second_callback.IsReady());
  EXPECT_TRUE(second_callback.Take());
}

TEST(BraveAdsAdEventTaskQueueTest,
     ProcessPendingTaskAfterCurrentTaskCompletes) {
  // Arrange
  AdEventTaskQueue queue;

  base::MockCallback<AdEventTask> first_task;
  base::test::TestFuture<ResultCallback> first_callback;
  EXPECT_CALL(first_task, Run)
      .WillOnce([&first_callback](ResultCallback callback) {
        first_callback.SetValue(std::move(callback));
      });
  queue.Add(first_task.Get(), base::DoNothing());

  base::MockCallback<AdEventTask> second_task;
  base::test::TestFuture<bool> second_callback;
  EXPECT_CALL(second_task, Run).WillOnce([](ResultCallback callback) {
    std::move(callback).Run(/*success=*/true);
  });
  queue.Add(second_task.Get(), second_callback.GetCallback());

  // The second task must wait for the first one to complete.
  ASSERT_FALSE(second_callback.IsReady());

  // Act
  first_callback.Take().Run(/*success=*/true);

  // Assert
  EXPECT_TRUE(second_callback.IsReady());
  EXPECT_TRUE(second_callback.Take());
}

TEST(BraveAdsAdEventTaskQueueTest, ProcessPendingTaskAfterCurrentTaskFails) {
  // Arrange
  AdEventTaskQueue queue;

  base::MockCallback<AdEventTask> first_task;
  base::test::TestFuture<ResultCallback> first_callback;
  EXPECT_CALL(first_task, Run)
      .WillOnce([&first_callback](ResultCallback callback) {
        first_callback.SetValue(std::move(callback));
      });
  queue.Add(first_task.Get(), base::DoNothing());

  base::MockCallback<AdEventTask> second_task;
  base::test::TestFuture<bool> second_callback;
  EXPECT_CALL(second_task, Run).WillOnce([](ResultCallback callback) {
    std::move(callback).Run(/*success=*/true);
  });
  queue.Add(second_task.Get(), second_callback.GetCallback());

  // Act
  first_callback.Take().Run(/*success=*/false);

  // Assert
  EXPECT_TRUE(second_callback.IsReady());
  EXPECT_TRUE(second_callback.Take());
}

TEST(BraveAdsAdEventTaskQueueTest, ProcessTasksInTheOrderTheyWereAdded) {
  // Arrange
  AdEventTaskQueue queue;

  std::vector<std::string> processed_tasks;

  base::MockCallback<AdEventTask> first_task;
  base::test::TestFuture<ResultCallback> first_callback;
  EXPECT_CALL(first_task, Run)
      .WillOnce([&first_callback, &processed_tasks](ResultCallback callback) {
        processed_tasks.push_back("task 1");
        first_callback.SetValue(std::move(callback));
      });
  queue.Add(first_task.Get(), base::DoNothing());

  base::MockCallback<AdEventTask> second_task;
  EXPECT_CALL(second_task, Run)
      .WillOnce([&processed_tasks](ResultCallback callback) {
        processed_tasks.push_back("task 2");
        std::move(callback).Run(/*success=*/true);
      });
  queue.Add(second_task.Get(), base::DoNothing());

  base::MockCallback<AdEventTask> third_task;
  EXPECT_CALL(third_task, Run)
      .WillOnce([&processed_tasks](ResultCallback callback) {
        processed_tasks.push_back("task 3");
        std::move(callback).Run(/*success=*/true);
      });
  queue.Add(third_task.Get(), base::DoNothing());

  ASSERT_THAT(processed_tasks, ::testing::ElementsAre("task 1"));

  // Act
  first_callback.Take().Run(/*success=*/true);

  // Assert
  EXPECT_THAT(processed_tasks,
              ::testing::ElementsAre("task 1", "task 2", "task 3"));
}

}  // namespace brave_ads
