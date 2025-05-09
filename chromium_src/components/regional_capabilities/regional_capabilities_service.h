/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_SERVICE_H_

#include "brave/components/search_engines/brave_prepopulated_engines.h"

#define GetRegionalPrepopulatedEngines \
  GetRegionalPrepopulatedEngines();    \
  TemplateURLPrepopulateData::BravePrepopulatedEngineID GetRegionalDefaultEngine

#include "src/components/regional_capabilities/regional_capabilities_service.h"  // IWYU pragma: export
#undef GetRegionalPrepopulatedEngines

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_REGIONAL_CAPABILITIES_REGIONAL_CAPABILITIES_SERVICE_H_
