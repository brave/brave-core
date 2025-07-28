/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/regional_capabilities/regional_capabilities_service_client_linux.h"

#define RegionalCapabilitiesServiceClientLinux \
  RegionalCapabilitiesServiceClientLinux_ChromiumImpl

#include <chrome/browser/regional_capabilities/regional_capabilities_service_client_linux.cc>
#undef RegionalCapabilitiesServiceClientLinux

namespace regional_capabilities {

RegionalCapabilitiesServiceClientLinux::
    ~RegionalCapabilitiesServiceClientLinux() = default;

void RegionalCapabilitiesServiceClientLinux::FetchCountryId(
    CountryIdCallback country_id_fetched_callback) {
  // Fall back onto the platform neutral implementation that uses device locale.
  return RegionalCapabilitiesServiceClient::FetchCountryId(
      std::move(country_id_fetched_callback));
}

}  // namespace regional_capabilities
