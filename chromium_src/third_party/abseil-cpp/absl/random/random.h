/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_ABSEIL_CPP_ABSL_RANDOM_RANDOM_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_ABSEIL_CPP_ABSL_RANDOM_RANDOM_H_

#include "src/third_party/abseil-cpp/absl/random/random.h"  // IWYU pragma: export

namespace absl {

// Make randen_engine available for direct usage, because
// absl::BitGen/InsecureBitGen uses always-on process-bound salt for seeded
// generators.
template <typename T>
using randen_engine = random_internal::randen_engine<T>;

}  // namespace absl

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_ABSEIL_CPP_ABSL_RANDOM_RANDOM_H_
