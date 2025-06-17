// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/chromium_src/components/variations/variations_seed_store.h"

#define UpdateSeedDateAndLogDayChange(...)                                  \
  UpdateSeedDateAndMaybeCountry(is_first_request,                           \
                                GetHeaderValue(headers.get(), "X-Country"), \
                                __VA_ARGS__)

#include "src/components/variations/service/variations_service.cc"

#undef UpdateSeedDateAndLogDayChange
