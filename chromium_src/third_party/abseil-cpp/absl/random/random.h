/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_ABSEIL_ABSL_RANDOM_RANDOM_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_ABSEIL_ABSL_RANDOM_RANDOM_H_

#include "src/third_party/abseil-cpp/absl/random/random.h"

namespace brave {
typedef absl::random_internal::randen_engine<uint64_t> FarblingPRNG;
}

#endif
