/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/auto_contribute_calculator.h"

#include <cmath>
#include <string>
#include <vector>

#include "bat/ledger/internal/core/randomizer.h"

namespace ledger {

namespace {

using WeightMap = AutoContributeCalculator::WeightMap;
using VoteMap = AutoContributeCalculator::VoteMap;

}  // namespace

WeightMap AutoContributeCalculator::CalculateWeights(
    const std::vector<PublisherActivity>& publishers,
    int min_visits,
    base::TimeDelta min_duration) {
  WeightMap publisher_map;

  // Get total duration for each qualified publisher.
  for (auto& activity : publishers) {
    if (activity.visits >= min_visits && activity.duration >= min_duration) {
      publisher_map[activity.publisher_id] += activity.duration.InSecondsF();
    }
  }

  // Convert durations into "scores".
  double min_seconds = min_duration.InSecondsF();
  double total_score = 0;
  for (auto& pair : publisher_map) {
    double score = ConvertSecondsToScore(pair.second, min_seconds);
    pair.second = score;
    total_score += score;
  }

  // Convert "scores" into weights.
  for (auto& pair : publisher_map) {
    pair.second = pair.second / total_score;
  }

  return publisher_map;
}

VoteMap AutoContributeCalculator::AllocateVotes(
    const WeightMap& publisher_weights,
    size_t total_votes) {
  VoteMap votes;
  for (auto& pair : publisher_weights) {
    votes[pair.first] = 0;
  }

  size_t votes_remaining = total_votes;
  while (votes_remaining > 0) {
    double random01 = context().Get<Randomizer>().Uniform01();
    double upper_bound = 0;
    for (auto& pair : publisher_weights) {
      upper_bound += pair.second;
      if (upper_bound >= random01) {
        votes[pair.first] += 1;
        votes_remaining -= 1;
        break;
      }
    }
  }

  return votes;
}

double AutoContributeCalculator::ConvertSecondsToScore(double seconds,
                                                       double min_seconds) {
  if (seconds <= 0 || min_seconds < 0) {
    return 0;
  }
  double min = min_seconds * 100;
  double a = 15'000 - min;
  double b = 2 * min - 15'000;
  double c = seconds * 100;
  return (-b + std::sqrt((b * b) + (4 * a * c))) / (2 * a);
}

}  // namespace ledger
