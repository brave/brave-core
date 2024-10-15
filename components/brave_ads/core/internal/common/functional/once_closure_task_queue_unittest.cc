/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/functional/once_closure_task_queue.h"

#include "base/functional/callback.h"
#include "base/test/mock_callback.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOnceClosureTaskQueueTest, Add) {
  // Arrange
  OnceClosureTaskQueue task_queue;
  base::MockCallback<base::OnceClosure> task;
  EXPECT_CALL(task, Run()).Times(0);
  task_queue.Add(task.Get());

  // Act & Assert
  EXPECT_FALSE(task_queue.empty());
  EXPECT_TRUE(task_queue.should_queue());
}

TEST(BraveAdsOnceClosureTaskQueueTest, AddAndFlush) {
  // Arrange
  OnceClosureTaskQueue task_queue;
  base::MockCallback<base::OnceClosure> task;
  EXPECT_CALL(task, Run());
  task_queue.Add(task.Get());

  // Act & Assert
  task_queue.FlushAndStopQueueing();
  EXPECT_TRUE(task_queue.empty());
  EXPECT_FALSE(task_queue.should_queue());
}

}  // namespace brave_ads
