// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "net/http/http_response_headers.h"

// At browser startup, the country code is updated using the X-Country header
// from the response if the status is `HTTP_NOT_MODIFIED`, avoiding the need
// to wait for the next update, which happens every 5 hours.
#define GetDateValue(...)                                               \
  GetDateValue(__VA_ARGS__);                                            \
  if (response_code == net::HTTP_NOT_MODIFIED && is_first_request) {    \
    std::string_view country_code =                                     \
        GetHeaderValue(headers.get(), "X-Country");                     \
    if (!country_code.empty()) {                                        \
      local_state_->SetString(prefs::kVariationsCountry, country_code); \
    }                                                                   \
  }

#include "src/components/variations/service/variations_service.cc"

#undef GetDateValue
