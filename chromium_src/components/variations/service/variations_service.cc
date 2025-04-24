// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// At browser startup, the country code is updated using the X-Country header
// from the response if the status is `HTTP_NOT_MODIFIED`, avoiding the need
// to wait for the next update, which happens every 5 hours.
#define BRAVE_VARIATIONS_SERVICE_ON_SIMPLE_LOADER_COMPLETE                    \
  std::string_view country_code = GetHeaderValue(headers.get(), "X-Country"); \
  if (is_first_request && !country_code.empty()) {                            \
    local_state_->SetString(prefs::kVariationsCountry, country_code);         \
  }

#include "src/components/variations/service/variations_service.cc"

#undef BRAVE_VARIATIONS_SERVICE_ON_SIMPLE_LOADER_COMPLETE
