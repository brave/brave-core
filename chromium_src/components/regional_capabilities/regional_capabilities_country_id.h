/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_COUNTRY_ID_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_COUNTRY_ID_H_

#define GetForTesting     \
  GetCountryCode() const; \
  country_codes::CountryId GetForTesting

#include "src/components/regional_capabilities/regional_capabilities_country_id.h"  // IWYU pragma: export

#undef GetForTesting

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_COUNTRY_ID_H_
