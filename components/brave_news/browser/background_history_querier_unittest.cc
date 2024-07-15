
// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/background_history_querier.h"

#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace brave_news {

class BraveNewsBackgroundHistoryQuerierTest : public testing::Test {
 public:
  BraveNewsBackgroundHistoryQuerierTest();
  BraveNewsBackgroundHistoryQuerierTest(
      const BraveNewsBackgroundHistoryQuerierTest&) = delete;
  BraveNewsBackgroundHistoryQuerierTest& operator=(
      const BraveNewsBackgroundHistoryQuerierTest&) = delete;
  ~BraveNewsBackgroundHistoryQuerierTest() override;
};

TEST_F(BraveNewsBackgroundHistoryQuerierTest, CanGetHistoryOffMainThread) {}

TEST_F(BraveNewsBackgroundHistoryQuerierTest, CanCancelBackgroundHistory) {}

TEST_F(BraveNewsBackgroundHistoryQuerierTest,
       BackgroundHistoryNotRequestedWhenDestroyed) {}

}  // namespace brave_news
