/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/search_engine_utils.h"

#include "brave/components/search_engines/brave_prepopulated_engines.h"

#define GetEngineType GetEngineType_ChromiumImpl
#include "../../../../components/search_engines/search_engine_utils.cc"
#undef GetEngineType

namespace SearchEngineUtils {

SearchEngineType GetEngineType(const GURL& url) {
  SearchEngineType type = GetEngineType_ChromiumImpl(url);
  if (type == SEARCH_ENGINE_OTHER) {
    const auto& brave_engines_map =
        TemplateURLPrepopulateData::GetBraveEnginesMap();
    for (const auto& entry : brave_engines_map) {
      const auto* engine = entry.second;
      if (SameDomain(url, GURL(engine->search_url))) {
        return engine->type;
      }
      for (size_t j = 0; j < engine->alternate_urls_size; ++j) {
        if (SameDomain(url, GURL(engine->alternate_urls[j]))) {
          return engine->type;
        }
      }
    }
  }
  return type;
}

}  // namespace SearchEngineUtils
