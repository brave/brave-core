/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/sponsored_content_country_code.h"

#include "base/strings/string_util.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/service/variations_service_utils.h"

namespace sponsored_content {

std::string GetVariationsCountryCode(
    variations::VariationsService* variations_service) {
  // The variations service derives the country code from the client's IP
  // address.
  std::string country_code;

  if (variations_service) {
    country_code = variations_service->GetLatestCountry();
  }

  if (country_code.empty()) {
    // May be empty on first run after a fresh install, so fall back to the
    // permanently stored variations or device country code on first run.
    country_code = variations::GetCurrentCountryCode(variations_service);
  }

  // Convert the country code to an ISO 3166-1 alpha-2 format. This ensures
  // the country code is in uppercase, as required by the standard.
  return base::ToUpperASCII(country_code);
}

}  // namespace sponsored_content
