/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url_prepopulate_data.h"

#include <vector>

#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/search_engines/search_engines_pref_names.h"

#define GetDataVersion GetDataVersion_ChromiumImpl
#define GetPrepopulatedFallbackSearch GetPrepopulatedFallbackSearch_Unused
#define GetPrepopulatedEngine GetPrepopulatedEngine_Unused
#include "src/components/search_engines/template_url_prepopulate_data.cc"
#undef GetPrepopulatedEngine
#undef GetPrepopulatedFallbackSearch
#undef GetDataVersion

namespace TemplateURLPrepopulateData {

// Redefines function with the same name in Chromium. We need to account for
// the version of Brave engines as well: kCurrentDataVersion is defined in
// prepopulated_engines.json and is bumped every time the json file is
// modified. Since we add our own engines we need to keep track of our
// version as well and combine it with Chromium's version.
int GetDataVersion(PrefService* prefs) {
  int data_version = GetDataVersion_ChromiumImpl(prefs);
  // Check if returned version was from preferences override and if so return
  // that version.
  if (prefs && prefs->HasPrefPath(prefs::kSearchProviderOverridesVersion)) {
    return data_version;
  }
  return (data_version + kBraveCurrentDataVersion);
}

// Chromium picks Google (if on the list, otherwise the first prepopulated on
// the list). We should return the default engine by country, or Brave.
std::unique_ptr<TemplateURLData> GetPrepopulatedFallbackSearch(
    BravePrepopulatedEngineID default_engine_id,
    PrefService& prefs,
    std::vector<const TemplateURLPrepopulateData::PrepopulatedEngine*>
        regional_prepopulated_engines) {
  std::vector<std::unique_ptr<TemplateURLData>> prepopulated_engines =
      GetPrepopulatedEngines(prefs, regional_prepopulated_engines);
  if (prepopulated_engines.empty()) {
    return nullptr;
  }

  std::unique_ptr<TemplateURLData> brave_engine;
  for (auto& engine : prepopulated_engines) {
    if (engine->prepopulate_id == static_cast<int>(default_engine_id)) {
      return std::move(engine);
    } else if (engine->prepopulate_id ==
               TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE) {
      brave_engine = std::move(engine);
    }
  }

  // Default engine wasn't found, then return Brave, if found.
  if (brave_engine) {
    return brave_engine;
  }

  // If all else fails, return the first engine on the list.
  return std::move(prepopulated_engines[0]);
}

std::unique_ptr<TemplateURLData> GetPrepopulatedEngine(
    PrefService& prefs,
    std::vector<const TemplateURLPrepopulateData::PrepopulatedEngine*>
        regional_prepopulated_engines,
    int prepopulated_id) {
  auto engines = TemplateURLPrepopulateData::GetPrepopulatedEngines(
      prefs, regional_prepopulated_engines);
  for (auto& engine : engines) {
    if (engine->prepopulate_id == prepopulated_id) {
      return std::move(engine);
    }
  }
  return nullptr;
}

}  // namespace TemplateURLPrepopulateData
