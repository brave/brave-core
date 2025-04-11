/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_UTILS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_UTILS_H_

#include "brave/components/search_engines/brave_prepopulated_engines.h"

#include "src/components/regional_capabilities/regional_capabilities_utils.h"  // IWYU pragma: export

namespace regional_capabilities {

TemplateURLPrepopulateData::BravePrepopulatedEngineID GetDefaultEngine(
    country_codes::CountryId country_id,
    PrefService& prefs);

}  // namespace regional_capabilities

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_UTILS_H_
