/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/auto_contribute_calculator.h"

#include <memory>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "bat/ledger/internal/core/randomizer.h"

namespace ledger {

class AutoContributeCalculatorTest : public BATLedgerTest {};

class FakeRandomizer : public Randomizer {
 public:
  double Uniform01() override {
    if (iter_ == values_.end()) {
      return 0;
    }
    return *iter_++;
  }

 private:
  std::vector<double> values_ = {0.1123, 0.8454, 0.534, 0.0324, 0.9787,
                                 0.43,   0.67,   0.22,  0.454,  0.987};
  std::vector<double>::iterator iter_ = values_.begin();
};

template <typename T = double>
class FuzzyFloat {
  static constexpr T kEpsilon = 0.00001;

 public:
  explicit FuzzyFloat(T value) : value_(value) {}

  bool operator==(T other) const {
    return std::abs(value_ - other) <= kEpsilon;
  }

  bool operator==(const FuzzyFloat& other) const {
    return std::abs(value_ - other.value_) <= kEpsilon;
  }

 private:
  T value_;
};

template <typename T>
bool operator==(T lhs, const FuzzyFloat<T>& rhs) {
  return rhs == lhs;
}

TEST_F(AutoContributeCalculatorTest, ConvertSecondsToScore) {
  EXPECT_EQ(AutoContributeCalculator::ConvertSecondsToScore(11, 3),
            FuzzyFloat(1.0508));
  EXPECT_EQ(AutoContributeCalculator::ConvertSecondsToScore(3, 1),
            FuzzyFloat(1.01316));
  EXPECT_EQ(AutoContributeCalculator::ConvertSecondsToScore(3, 0),
            FuzzyFloat(1.01962));
  EXPECT_EQ(AutoContributeCalculator::ConvertSecondsToScore(0, 1), 0.0);
  EXPECT_EQ(AutoContributeCalculator::ConvertSecondsToScore(1, -1), 0.0);
}

TEST_F(AutoContributeCalculatorTest, CalculateWeights) {
  auto& calculator = context().Get<AutoContributeCalculator>();

  std::vector<PublisherActivity> publishers = {
      {.publisher_id = "brave.com", .visits = 4, .duration = base::Seconds(14)},
      {.publisher_id = "any.org", .visits = 2, .duration = base::Seconds(10)}};

  auto weights = calculator.CalculateWeights(publishers, 1, base::Seconds(2));

  ASSERT_EQ(weights.size(), 2ul);
  EXPECT_EQ(weights["brave.com"], FuzzyFloat(0.505583));
  EXPECT_EQ(weights["any.org"], FuzzyFloat(0.494417));
}

TEST_F(AutoContributeCalculatorTest, AllocateVotes) {
  auto& calculator = context().Get<AutoContributeCalculator>();
  context().SetComponentForTesting(std::make_unique<FakeRandomizer>());

  AutoContributeCalculator::WeightMap weights = {{"brave.com", 0.6},
                                                 {"any.org", 0.4}};

  auto votes = calculator.AllocateVotes(weights, 10);
  ASSERT_EQ(votes.size(), 2ul);
  EXPECT_EQ(votes["brave.com"], 7ul);
  EXPECT_EQ(votes["any.org"], 3ul);
}

}  // namespace ledger
