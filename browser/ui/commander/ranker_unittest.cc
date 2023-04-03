// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commander/ranker.h"
#include <memory>
#include <utility>
#include <vector>

#include "brave/browser/ui/commander/commander_service_factory.h"
#include "brave/components/commander/common/prefs.h"
#include "chrome/browser/ui/commander/command_source.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

class RankerUnitTest : public testing::Test {
 public:
  RankerUnitTest() : ranker_(profile_.GetPrefs()) {}
  ~RankerUnitTest() override = default;

  void SetUp() override {
    profile_.GetTestingPrefService()->registry()->RegisterDictionaryPref(
        commander::prefs::kCommanderFrecencies);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;

  TestingProfile profile_;
  commander::Ranker ranker_;
};

TEST_F(RankerUnitTest, EqualRankSortsAlphabetically) {
  auto one = std::make_unique<commander::CommandItem>();
  one->title = u"A";
  auto two = std::make_unique<commander::CommandItem>();
  two->title = u"B";

  std::vector<std::unique_ptr<commander::CommandItem>> items;
  items.push_back(std::move(two));
  items.push_back(std::move(one));

  ranker_.Rank(items, 2);
  ASSERT_EQ(2u, items.size());
  EXPECT_EQ(u"A", items[0]->title);
  EXPECT_EQ(u"B", items[1]->title);
}

TEST_F(RankerUnitTest, OnlyFirstNResultsAreSorted) {}

TEST_F(RankerUnitTest, ScoreIsWeightedByVisits) {}
