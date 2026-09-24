// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commander/ranker.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/browser/ui/commander/command_source.h"
#include "brave/components/commander/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class RankerUnitTest : public testing::Test {
 public:
  RankerUnitTest() : ranker_(&prefs_) {}
  ~RankerUnitTest() override = default;

  void SetUp() override {
    prefs_.registry()->RegisterDictionaryPref(
        commander::prefs::kCommanderFrecencies);
  }

 protected:
  TestingPrefServiceSimple prefs_;
  commander::Ranker ranker_;
};

TEST_F(RankerUnitTest, EqualRankSortsAlphabetically) {
  auto one = std::make_unique<commander::CommandItem>();
  one->title = u"A";
  one->score = 0.5;
  auto two = std::make_unique<commander::CommandItem>();
  two->title = u"B";
  two->score = 0.5;

  std::vector<std::unique_ptr<commander::CommandItem>> items;
  items.push_back(std::move(two));
  items.push_back(std::move(one));

  ranker_.Rank(items, 2);
  ASSERT_EQ(2u, items.size());
  EXPECT_EQ(u"A", items[0]->title);
  EXPECT_EQ(u"B", items[1]->title);
}

TEST_F(RankerUnitTest, OnlyFirstNResultsAreSorted) {
  auto one = std::make_unique<commander::CommandItem>();
  one->title = u"A";
  one->score = 500;

  auto two = std::make_unique<commander::CommandItem>();
  two->title = u"B";
  two->score = 100;

  auto three = std::make_unique<commander::CommandItem>();
  three->title = u"C";
  three->score = 50;

  std::vector<std::unique_ptr<commander::CommandItem>> items;
  items.push_back(std::move(three));
  items.push_back(std::move(two));
  items.push_back(std::move(one));

  ranker_.Rank(items, 1);
  ASSERT_EQ(3u, items.size());
  EXPECT_EQ(u"A", items[0]->title);
}

TEST_F(RankerUnitTest, ScoreIsWeightedByVisits) {
  auto one = std::make_unique<commander::CommandItem>();
  one->title = u"A";
  one->score = 100;

  auto two = std::make_unique<commander::CommandItem>();
  two->title = u"B";
  two->score = 100;

  auto three = std::make_unique<commander::CommandItem>();
  three->title = u"C";
  three->score = 100;

  ranker_.Visit(*one.get());
  ranker_.Visit(*one.get());
  ranker_.Visit(*two.get());

  std::vector<std::unique_ptr<commander::CommandItem>> items;
  items.push_back(std::move(three));
  items.push_back(std::move(two));
  items.push_back(std::move(one));

  ranker_.Rank(items, 3);
  ASSERT_EQ(3u, items.size());
  EXPECT_EQ(u"A", items[0]->title);
  EXPECT_EQ(u"B", items[1]->title);
  EXPECT_EQ(u"C", items[2]->title);
}
