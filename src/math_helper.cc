/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "math_helper.h"

namespace helper {

int Math::Random() {
  std::random_device device;
  std::mt19937_64 generator(device());
  auto max = std::numeric_limits<int>::max();
  std::uniform_int_distribution<int> distribution(0, max);
  return distribution(generator);
}

}  // namespace helper
