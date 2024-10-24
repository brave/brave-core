/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/hash_detection.h"

#include <cmath>

#include "base/containers/fixed_flat_map.h"
#include "brave/components/web_discovery/browser/hash_detection_matrix.h"
#include "brave/components/web_discovery/browser/util.h"

namespace web_discovery {

namespace {

constexpr auto kTokenMap = base::MakeFixedFlatMap<char, size_t>(
    {{'1', 1},  {'0', 0},  {'3', 3},  {'2', 2},  {'5', 5},  {'4', 4},
     {'7', 7},  {'6', 6},  {'9', 9},  {'8', 8},  {'A', 36}, {'C', 38},
     {'B', 37}, {'E', 40}, {'D', 39}, {'G', 42}, {'F', 41}, {'I', 44},
     {'H', 43}, {'K', 46}, {'J', 45}, {'M', 48}, {'L', 47}, {'O', 50},
     {'N', 49}, {'Q', 52}, {'P', 51}, {'S', 54}, {'R', 53}, {'U', 56},
     {'T', 55}, {'W', 58}, {'V', 57}, {'Y', 60}, {'X', 59}, {'Z', 61},
     {'a', 10}, {'c', 12}, {'b', 11}, {'e', 14}, {'d', 13}, {'g', 16},
     {'f', 15}, {'i', 18}, {'h', 17}, {'k', 20}, {'j', 19}, {'m', 22},
     {'l', 21}, {'o', 24}, {'n', 23}, {'q', 26}, {'p', 25}, {'s', 28},
     {'r', 27}, {'u', 30}, {'t', 29}, {'w', 32}, {'v', 31}, {'y', 34},
     {'x', 33}, {'z', 35}});

constexpr double kClassifierThreshold = 0.015;

}  // namespace

bool IsHashLikely(std::string value, double threshold_multiplier) {
  TransformToAlphanumeric(value);

  double log_prob_sum = 0.0;
  size_t add_count = 0;
  for (size_t i = 0; i < value.length() - 1; i++) {
    auto matrix_pos_a = kTokenMap.find(value[i]);
    auto matrix_pos_b = kTokenMap.find(value[i + 1]);
    if (matrix_pos_a == kTokenMap.end() || matrix_pos_b == kTokenMap.end()) {
      continue;
    }

    log_prob_sum +=
        kClassifierTransitionMatrix[matrix_pos_a->second][matrix_pos_b->second];
    add_count++;
  }

  if (add_count == 0) {
    return 1.0;
  }

  double prob = std::exp(log_prob_sum / add_count);
  return prob < (threshold_multiplier * kClassifierThreshold);
}

}  // namespace web_discovery
