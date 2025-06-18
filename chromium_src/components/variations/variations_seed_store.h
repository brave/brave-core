/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_SEED_STORE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_SEED_STORE_H_

#define UpdateSeedDateAndLogDayChange(...)                                   \
  UpdateSeedDateAndMaybeCountry(bool is_first_request,                       \
                                std::string_view country_code, __VA_ARGS__); \
  void UpdateSeedDateAndLogDayChange(__VA_ARGS__)

#include "src/components/variations/variations_seed_store.h"  // IWYU pragma: export

#undef UpdateSeedDateAndLogDayChange

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_SEED_STORE_H_
