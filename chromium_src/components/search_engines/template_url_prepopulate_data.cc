/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/country_codes/country_codes.h"

// Pull in definitions for Brave prepopulated engines. It's ugly but these need
// to be built as part of the search_engines static library.
#include "../../../components/search_engines/brave_prepopulated_engines.cc"  // NOLINT
#include "../../../components/search_engines/brave_prepopulated_engines.h"

#define GetDataVersion GetDataVersion_ChromiumImpl
#if defined(OS_ANDROID)
#define GetLocalPrepopulatedEngines GetLocalPrepopulatedEngines_Unused
#endif
#define GetPrepopulatedDefaultSearch GetPrepopulatedDefaultSearch_Unused
#define GetPrepopulatedEngine GetPrepopulatedEngine_Unused
#define GetPrepopulatedEngines GetPrepopulatedEngines_Unused
#include "../../../../components/search_engines/template_url_prepopulate_data.cc"  // NOLINT
#undef GetDataVersion
#if defined(OS_ANDROID)
#undef GetLocalPrepopulatedEngines
#endif
#undef GetPrepopulatedDefaultSearch
#undef GetPrepopulatedEngine
#undef GetPrepopulatedEngines

namespace TemplateURLPrepopulateData {

namespace {

// Maps BravePrepopulatedEngineID to Chromium's PrepopulatedEngine.
const std::map<BravePrepopulatedEngineID, const PrepopulatedEngine*>
    brave_engines_map = {
        {PREPOPULATED_ENGINE_ID_GOOGLE, &google},
        {PREPOPULATED_ENGINE_ID_BING, &bing},
        {PREPOPULATED_ENGINE_ID_DUCKDUCKGO, &duckduckgo},
        {PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE, &duckduckgo_de},
        {PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE, &duckduckgo_au_nz_ie},
#if defined(OS_ANDROID)
        {PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE, &duckduckgo_lite},
#endif
        {PREPOPULATED_ENGINE_ID_QWANT, &qwant},
        {PREPOPULATED_ENGINE_ID_STARTPAGE, &startpage},
};

// Default order in which engines will appear in the UI.
std::vector<BravePrepopulatedEngineID> brave_engines_default = {
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
#if defined(OS_ANDROID)
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE,
#endif
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

// Germany - DDG appears on top.
std::vector<BravePrepopulatedEngineID> brave_engines_DE = {
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
#if defined(OS_ANDROID)
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE,
#endif
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

// France - Qwant appears on top.
std::vector<BravePrepopulatedEngineID> brave_engines_FR = {
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
#if defined(OS_ANDROID)
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE,
#endif
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

// Australia / NZ / IE - DDG appears on top.
std::vector<BravePrepopulatedEngineID> brave_engines_AU_NZ_IE = {
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE,
#if defined(OS_ANDROID)
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE,
#endif
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE};

// A map to keep track of default engines for countries that don't use the
// regular default engine.
const std::map<int, std::vector<BravePrepopulatedEngineID>>
    default_engines_by_country_id_map = {
        {country_codes::CountryCharsToCountryID('D', 'E'), brave_engines_DE},
        {country_codes::CountryCharsToCountryID('F', 'R'), brave_engines_FR},
        {country_codes::CountryCharsToCountryID('A', 'U'),
         brave_engines_AU_NZ_IE},
        {country_codes::CountryCharsToCountryID('N', 'Z'),
         brave_engines_AU_NZ_IE},
        {country_codes::CountryCharsToCountryID('I', 'E'),
         brave_engines_AU_NZ_IE}};

// Builds a vector of PrepulatedEngine objects from the given array of
// |engine_ids|. Fills in the default engine index for the given |country_id|,
// if asked.
std::vector<const PrepopulatedEngine*> GetEnginesFromEngineIDs(
    const std::vector<BravePrepopulatedEngineID> engine_ids,
    int country_id,
    BravePrepopulatedEngineID default_engine_id,
    size_t* default_search_provider_index = nullptr) {
  std::vector<const PrepopulatedEngine*> engines;
  for (size_t i = 0; i < engine_ids.size(); ++i) {
    const PrepopulatedEngine* engine = brave_engines_map.at(engine_ids[i]);
    DCHECK(engine);
    if (engine) {
      engines.push_back(engine);
      if (default_search_provider_index && default_engine_id == engine_ids[i])
        *default_search_provider_index = i;
    }
  }
  return engines;
}

void UpdateTemplateURLDataKeyword(
    const std::unique_ptr<TemplateURLData>& t_urld) {
  DCHECK(t_urld.get());
  switch (t_urld->prepopulate_id) {
    case PREPOPULATED_ENGINE_ID_GOOGLE:
      t_urld->SetKeyword(base::ASCIIToUTF16(":g"));
      break;
    case PREPOPULATED_ENGINE_ID_BING:
      t_urld->SetKeyword(base::ASCIIToUTF16(":b"));
      break;
  }
}

// Uses brave_engines_XX localized arrays of engine IDs instead of Chromium's
// localized arrays of PrepopulatedEngines to construct the vector of
// TemplateURLData. Also, fills in the default engine index for the given
// |country_id|.
std::vector<std::unique_ptr<TemplateURLData>>
GetBravePrepopulatedEnginesForCountryID(
    int country_id,
    size_t* default_search_provider_index = nullptr) {
  std::vector<BravePrepopulatedEngineID> brave_engines;

  // Check for exceptions from the default list of engines
  const auto& it = default_engines_by_country_id_map.find(country_id);
  if (it != default_engines_by_country_id_map.end()) {
    brave_engines = it->second;
  } else {
    brave_engines = brave_engines_default;
  }
  DCHECK_GT(brave_engines.size(), 0);

  // Default engine is the first in the list
  BravePrepopulatedEngineID default_id = brave_engines.front();

  // Build a vector PrepopulatedEngines from BravePrepopulatedEngineIDs and
  // also get the default engine index
  std::vector<const PrepopulatedEngine*> engines =
      GetEnginesFromEngineIDs(brave_engines, country_id,
                              default_id, default_search_provider_index);
  DCHECK(engines.size() == brave_engines.size());

  std::vector<std::unique_ptr<TemplateURLData>> t_urls;
  for (const PrepopulatedEngine* engine : engines) {
    std::unique_ptr<TemplateURLData> t_urld =
        TemplateURLDataFromPrepopulatedEngine(*engine);
    UpdateTemplateURLDataKeyword(t_urld);
    t_urls.push_back(std::move(t_urld));
  }

  return t_urls;
}

}  // namespace

// Redefines function with the same name in Chromium. We need to account for
// the version of Brave engines as well: kCurrentDataVersion is defined in
// prepopulated_engines.json and is bumped every time the json file is
// modified. Since we add our own engines we need to keep track of our
// version as well and combine it with Chromium's version.
int GetDataVersion(PrefService* prefs) {
  int dataVersion = GetDataVersion_ChromiumImpl(prefs);
  // Check if returned version was from preferences override and if so return
  // that version.
  if (prefs && prefs->HasPrefPath(prefs::kSearchProviderOverridesVersion))
    return dataVersion;
  return (dataVersion + kBraveCurrentDataVersion);
}

// Redefines function with the same name in Chromium. Modifies the function to
// get search engines defined by Brave.
std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulatedEngines(
    PrefService* prefs,
    size_t* default_search_provider_index) {
  // If there is a set of search engines in the preferences file, it overrides
  // the built-in set.
  if (default_search_provider_index)
    *default_search_provider_index = 0;
  std::vector<std::unique_ptr<TemplateURLData>> t_urls =
      GetPrepopulatedTemplateURLData(prefs);
  if (!t_urls.empty())
    return t_urls;

  return GetBravePrepopulatedEnginesForCountryID(
      country_codes::GetCountryIDFromPrefs(prefs),
      default_search_provider_index);
}

// Redefines function with the same name in Chromium. Modifies the function to
// get search engines defined by Brave.
#if defined(OS_ANDROID)

std::vector<std::unique_ptr<TemplateURLData>> GetLocalPrepopulatedEngines(
    const std::string& locale) {
  int country_id = country_codes::CountryStringToCountryID(locale);
  if (country_id == country_codes::kCountryIDUnknown) {
    LOG(ERROR) << "Unknown country code specified: " << locale;
    return std::vector<std::unique_ptr<TemplateURLData>>();
  }

  return GetBravePrepopulatedEnginesForCountryID(country_id);
}

#endif

// Functions below are copied verbatim from
// components\search_engines\template_url_prepopulate_data.cc because they
// need to call our versions of redefined Chromium's functions.

std::unique_ptr<TemplateURLData> GetPrepopulatedEngine(PrefService* prefs,
                                                       int prepopulated_id) {
  size_t default_index;
  auto engines =
      TemplateURLPrepopulateData::GetPrepopulatedEngines(prefs, &default_index);
  for (auto& engine : engines) {
    if (engine->prepopulate_id == prepopulated_id)
      return std::move(engine);
  }
  return nullptr;
}

std::unique_ptr<TemplateURLData> GetPrepopulatedDefaultSearch(
    PrefService* prefs) {
  size_t default_search_index;
  // This could be more efficient.  We are loading all the URLs to only keep
  // the first one.
  std::vector<std::unique_ptr<TemplateURLData>> loaded_urls =
      GetPrepopulatedEngines(prefs, &default_search_index);

  return (default_search_index < loaded_urls.size())
             ? std::move(loaded_urls[default_search_index])
             : nullptr;
}

}  // namespace TemplateURLPrepopulateData
