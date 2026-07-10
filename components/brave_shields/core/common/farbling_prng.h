// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_FARBLING_PRNG_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_FARBLING_PRNG_H_

#include "third_party/abseil-cpp/absl/random/random.h"

namespace brave_shields {

// The seeded pseudo-random generator used across Brave's farbling code to
// deterministically derive farbling values for a given origin.
using FarblingPRNG = absl::random_internal::randen_engine<uint64_t>;

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_FARBLING_PRNG_H_
