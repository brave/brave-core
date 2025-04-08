/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url_prepopulate_data_resolver.h"

#include "components/search_engines/template_url_prepopulate_data.h"

#define GetPrepopulatedFallbackSearch(...) \
  GetPrepopulatedFallbackSearch(           \
      regional_capabilities_->GetRegionalDefaultEngine(), __VA_ARGS__)

#include "src/components/search_engines/template_url_prepopulate_data_resolver.cc"
#undef GetPrepopulatedFallbackSearch
