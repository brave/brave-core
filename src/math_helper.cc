/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "math_helper.h"

namespace helper {

size_t Math::Random(size_t maximum_value) {
  std::random_device device;
  std::mt19937_64 generator(device());
  std::uniform_int_distribution<size_t> distribution(0, maximum_value);
  return distribution(generator);
}

}  // namespace helper
