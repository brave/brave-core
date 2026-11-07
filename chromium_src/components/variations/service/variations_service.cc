// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "net/http/http_response_headers.h"

// Update the country code from the X-Country header on every 304 Not
// Modified response so that mid-session country changes, such as a VPN
// change, propagate without requiring a browser restart. Routed through
// `VariationsSeedStore::SetSessionCountry` rather than a raw pref write,
// as clients in the `SeedFileTrial` experiment's `SeedFiles_V11` group read
// the country from an in-memory seed cache that a raw pref write does not
// reach.
#define GetDateValue(...)                                                 \
  GetDateValue(__VA_ARGS__);                                              \
  if (response_code == net::HTTP_NOT_MODIFIED) {                          \
    std::string_view country_code =                                       \
        GetHeaderValue(headers.get(), "X-Country");                       \
    if (!country_code.empty()) {                                          \
      field_trial_creator_.seed_store()->SetSessionCountry(country_code); \
    }                                                                     \
  }

#include <components/variations/service/variations_service.cc>

#undef GetDateValue
