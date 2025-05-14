/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_PREPOPULATE_DATA_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_PREPOPULATE_DATA_H_

#include "brave/components/search_engines/brave_prepopulated_engines.h"

#define GetPrepopulatedFallbackSearch(...)                                   \
  GetPrepopulatedFallbackSearch(BravePrepopulatedEngineID default_engine_id, \
                                __VA_ARGS__)

#include "src/components/search_engines/template_url_prepopulate_data.h"  // IWYU pragma: export
#undef GetPrepopulatedFallbackSearch

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_PREPOPULATE_DATA_H_
