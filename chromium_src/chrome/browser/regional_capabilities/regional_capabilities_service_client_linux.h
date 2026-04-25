/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_SERVICE_CLIENT_LINUX_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_SERVICE_CLIENT_LINUX_H_

// Subclass RegionalCapabilitiesServiceClientLinux so that we can override
// FetchCountryId. On other platforms this methods retrives locale from the
// device, but on Linux upstream only relies on the variations service to get
// the current locale. This doesn't work right since, for example, search
// engines selection switches based on your locale.
#define RegionalCapabilitiesServiceClientLinux \
  RegionalCapabilitiesServiceClientLinux_ChromiumImpl

#include <chrome/browser/regional_capabilities/regional_capabilities_service_client_linux.h>  // IWYU pragma: export
#undef RegionalCapabilitiesServiceClientLinux

namespace regional_capabilities {

class RegionalCapabilitiesServiceClientLinux
    : public RegionalCapabilitiesServiceClientLinux_ChromiumImpl {
 public:
  using RegionalCapabilitiesServiceClientLinux_ChromiumImpl::
      RegionalCapabilitiesServiceClientLinux_ChromiumImpl;
  ~RegionalCapabilitiesServiceClientLinux() override;

  void FetchCountryId(CountryIdCallback country_id_fetched_callback) override;
};

}  // namespace regional_capabilities

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_SERVICE_CLIENT_LINUX_H_
