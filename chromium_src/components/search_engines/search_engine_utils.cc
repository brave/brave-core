/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/search_engine_utils.h"

#include "base/compiler_specific.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"

#define GetEngineType GetEngineType_ChromiumImpl
#include "src/components/search_engines/search_engine_utils.cc"
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
      for (const auto* alternate_url : engine->alternate_urls) {
        if (SameDomain(url, GURL(alternate_url))) {
          return engine->type;
        }
      }
    }
  }
  return type;
}

}  // namespace SearchEngineUtils
