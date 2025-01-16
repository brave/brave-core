/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/hash_detection.h"

#include <cmath>

#include "brave/components/web_discovery/browser/hash_detection_matrix.h"

namespace web_discovery {

namespace {

size_t CharToToken(char c) {
  if (c >= 'a') {
    return c - 'a' + 10;
  }
  if (c >= 'A') {
    return c - 'A' + 36;
  }
  return c - '0';
}

constexpr double kClassifierThreshold = 0.015;

}  // namespace

bool IsHashLikely(std::string_view value, double threshold_multiplier) {
  if (value.empty()) {
    return false;
  }

  double log_prob_sum = 0.0;
  size_t add_count = 0;
  for (size_t i = 0; i < value.length() - 1; i++) {
    if (!std::isalnum(value[i])) {
      continue;
    }
    size_t next_char_i;
    for (next_char_i = i + 1;
         next_char_i < value.length() && !std::isalnum(value[next_char_i]);
         next_char_i++) {
    }
    if (!std::isalnum(value[next_char_i])) {
      break;
    }
    auto matrix_pos_a = CharToToken(value[i]);
    auto matrix_pos_b = CharToToken(value[next_char_i]);

    log_prob_sum += kClassifierTransitionMatrix[matrix_pos_a][matrix_pos_b];
    add_count++;
  }

  if (add_count == 0) {
    return 1.0;
  }

  double prob = std::exp(log_prob_sum / add_count);
  return prob < (threshold_multiplier * kClassifierThreshold);
}

}  // namespace web_discovery
