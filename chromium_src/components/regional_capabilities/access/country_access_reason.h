/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_ACCESS_COUNTRY_ACCESS_REASON_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_ACCESS_COUNTRY_ACCESS_REASON_H_

namespace brave {
class WithCountryAccessKeyAccess;
}  // namespace brave

namespace brave_ads {
class AdsServiceDelegate;
}  // namespace brave_ads

// Unfortunately this is necessary for now as upstream is vetting access to this
// class through friend class listing, and access to it is necessary to retrieve
// country id.
#define RegionalCapabilitiesService               \
  RegionalCapabilitiesService;                    \
  friend class brave::WithCountryAccessKeyAccess; \
  friend class brave_ads::AdsServiceDelegate
#include "src/components/regional_capabilities/access/country_access_reason.h"  // IWYU pragma: export
#undef RegionalCapabilitiesService

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_ACCESS_COUNTRY_ACCESS_REASON_H_
