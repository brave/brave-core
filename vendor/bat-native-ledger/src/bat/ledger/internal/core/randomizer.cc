/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/randomizer.h"

#include "brave_base/random.h"

namespace ledger {

double Randomizer::Uniform01() {
  return brave_base::random::Uniform_01();
}

uint64_t Randomizer::Geometric(double period) {
  return brave_base::random::Geometric(period);
}

}  // namespace ledger
