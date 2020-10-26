/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url_prepopulate_data.h"

#include <map>
#include <vector>

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/country_codes/country_codes.h"

#define GetDataVersion GetDataVersion_ChromiumImpl
#if defined(OS_ANDROID)
#define GetLocalPrepopulatedEngines GetLocalPrepopulatedEngines_Unused
#endif
#define GetPrepopulatedDefaultSearch GetPrepopulatedDefaultSearch_Unused
#define GetPrepopulatedEngine GetPrepopulatedEngine_Unused
#define GetPrepopulatedEngines GetPrepopulatedEngines_Unused
#include "../../../../components/search_engines/template_url_prepopulate_data.cc"
#undef GetDataVersion
#if defined(OS_ANDROID)
#undef GetLocalPrepopulatedEngines
#endif
#undef GetPrepopulatedDefaultSearch
#undef GetPrepopulatedEngine
#undef GetPrepopulatedEngines

namespace TemplateURLPrepopulateData {

void LocalizeEngineList(
    int country_id, std::vector<BravePrepopulatedEngineID>* engines);

namespace {

// Default order in which engines will appear in the UI.
const std::vector<BravePrepopulatedEngineID> brave_engines_default = {
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

// Variations of the order / default options by country.
const std::vector<BravePrepopulatedEngineID> brave_engines_with_yahoo = {
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

const std::vector<BravePrepopulatedEngineID> brave_engines_DE = {
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

const std::vector<BravePrepopulatedEngineID> brave_engines_FR = {
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

const std::vector<BravePrepopulatedEngineID> brave_engines_AU_NZ_IE = {
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE,
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE};

// A map to keep track of a full list of default engines for countries
// that don't use the default list.
const std::map<int, const std::vector<BravePrepopulatedEngineID>*>
    default_engines_by_country_id_map = {
        {country_codes::CountryCharsToCountryID('A', 'R'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('A', 'T'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('A', 'U'),
         &brave_engines_AU_NZ_IE},
        {country_codes::CountryCharsToCountryID('B', 'R'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('C', 'A'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('C', 'H'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('C', 'L'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('C', 'O'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('D', 'E'),
         &brave_engines_DE},
        {country_codes::CountryCharsToCountryID('D', 'K'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('E', 'S'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('F', 'I'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('F', 'R'),
         &brave_engines_FR},
        {country_codes::CountryCharsToCountryID('G', 'B'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('H', 'K'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('I', 'D'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('I', 'E'),
         &brave_engines_AU_NZ_IE},
        {country_codes::CountryCharsToCountryID('I', 'N'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('I', 'T'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('M', 'X'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('M', 'Y'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('N', 'L'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('N', 'O'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('N', 'Z'),
         &brave_engines_AU_NZ_IE},
        {country_codes::CountryCharsToCountryID('P', 'E'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('P', 'H'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('S', 'E'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('S', 'G'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('T', 'H'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('T', 'W'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('U', 'S'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('V', 'E'),
         &brave_engines_with_yahoo},
        {country_codes::CountryCharsToCountryID('V', 'N'),
         &brave_engines_with_yahoo}};

// Default search engine. Overridable on a per-country basis.
const BravePrepopulatedEngineID default_engine =
    PREPOPULATED_ENGINE_ID_GOOGLE;

// A map tracking the singular default search engine per-country.
const std::map<int, BravePrepopulatedEngineID>
    default_engine_by_country_id_map = {
        {country_codes::CountryCharsToCountryID('A', 'U'),
         PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
        {country_codes::CountryCharsToCountryID('F', 'R'),
         PREPOPULATED_ENGINE_ID_QWANT},
        {country_codes::CountryCharsToCountryID('D', 'E'),
         PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE},
        {country_codes::CountryCharsToCountryID('I', 'E'),
         PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
        {country_codes::CountryCharsToCountryID('N', 'Z'),
         PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE}};

// A map to keep track of country-specific implementations of Yahoo.
// Used in LocalizeEngineList.
const std::map<int, BravePrepopulatedEngineID>
    yahoo_engines_by_country_id_map = {
        {country_codes::CountryCharsToCountryID('A', 'R'),
         PREPOPULATED_ENGINE_ID_YAHOO_AR},
        {country_codes::CountryCharsToCountryID('A', 'T'),
         PREPOPULATED_ENGINE_ID_YAHOO_AT},
        {country_codes::CountryCharsToCountryID('A', 'U'),
         PREPOPULATED_ENGINE_ID_YAHOO_AU},
        {country_codes::CountryCharsToCountryID('B', 'R'),
         PREPOPULATED_ENGINE_ID_YAHOO_BR},
        {country_codes::CountryCharsToCountryID('C', 'A'),
         PREPOPULATED_ENGINE_ID_YAHOO_CA},
        {country_codes::CountryCharsToCountryID('C', 'H'),
         PREPOPULATED_ENGINE_ID_YAHOO_CH},
        {country_codes::CountryCharsToCountryID('C', 'L'),
         PREPOPULATED_ENGINE_ID_YAHOO_CL},
        {country_codes::CountryCharsToCountryID('C', 'O'),
         PREPOPULATED_ENGINE_ID_YAHOO_CO},
        {country_codes::CountryCharsToCountryID('D', 'E'),
         PREPOPULATED_ENGINE_ID_YAHOO_DE},
        {country_codes::CountryCharsToCountryID('D', 'K'),
         PREPOPULATED_ENGINE_ID_YAHOO_DK},
        {country_codes::CountryCharsToCountryID('E', 'S'),
         PREPOPULATED_ENGINE_ID_YAHOO_ES},
        {country_codes::CountryCharsToCountryID('F', 'I'),
         PREPOPULATED_ENGINE_ID_YAHOO_FI},
        {country_codes::CountryCharsToCountryID('F', 'R'),
         PREPOPULATED_ENGINE_ID_YAHOO_FR},
        {country_codes::CountryCharsToCountryID('G', 'B'),
         PREPOPULATED_ENGINE_ID_YAHOO_UK},
        {country_codes::CountryCharsToCountryID('H', 'K'),
         PREPOPULATED_ENGINE_ID_YAHOO_HK},
        {country_codes::CountryCharsToCountryID('I', 'D'),
         PREPOPULATED_ENGINE_ID_YAHOO_ID},
        {country_codes::CountryCharsToCountryID('I', 'E'),
         PREPOPULATED_ENGINE_ID_YAHOO_IE},
        {country_codes::CountryCharsToCountryID('I', 'N'),
         PREPOPULATED_ENGINE_ID_YAHOO_IN},
        {country_codes::CountryCharsToCountryID('I', 'T'),
         PREPOPULATED_ENGINE_ID_YAHOO_IT},
        {country_codes::CountryCharsToCountryID('M', 'X'),
         PREPOPULATED_ENGINE_ID_YAHOO_MX},
        {country_codes::CountryCharsToCountryID('M', 'Y'),
         PREPOPULATED_ENGINE_ID_YAHOO_MY},
        {country_codes::CountryCharsToCountryID('N', 'L'),
         PREPOPULATED_ENGINE_ID_YAHOO_NL},
        {country_codes::CountryCharsToCountryID('N', 'O'),
         PREPOPULATED_ENGINE_ID_YAHOO_NO},
        {country_codes::CountryCharsToCountryID('N', 'Z'),
         PREPOPULATED_ENGINE_ID_YAHOO_NZ},
        {country_codes::CountryCharsToCountryID('P', 'E'),
         PREPOPULATED_ENGINE_ID_YAHOO_PE},
        {country_codes::CountryCharsToCountryID('P', 'H'),
         PREPOPULATED_ENGINE_ID_YAHOO_PH},
        {country_codes::CountryCharsToCountryID('S', 'E'),
         PREPOPULATED_ENGINE_ID_YAHOO_SE},
        {country_codes::CountryCharsToCountryID('S', 'G'),
         PREPOPULATED_ENGINE_ID_YAHOO_SG},
        {country_codes::CountryCharsToCountryID('T', 'H'),
         PREPOPULATED_ENGINE_ID_YAHOO_TH},
        {country_codes::CountryCharsToCountryID('T', 'W'),
         PREPOPULATED_ENGINE_ID_YAHOO_TW},
        {country_codes::CountryCharsToCountryID('V', 'E'),
         PREPOPULATED_ENGINE_ID_YAHOO_VE},
        {country_codes::CountryCharsToCountryID('V', 'N'),
         PREPOPULATED_ENGINE_ID_YAHOO_VN}};

// Builds a vector of PrepulatedEngine objects from the given array of
// |engine_ids|. Fills in the default engine index for the given |country_id|,
// if asked.
std::vector<const PrepopulatedEngine*> GetEnginesFromEngineIDs(
    const std::vector<BravePrepopulatedEngineID>& engine_ids,
    int country_id,
    BravePrepopulatedEngineID default_engine_id,
    size_t* default_search_provider_index = nullptr) {
  std::vector<const PrepopulatedEngine*> engines;
  const auto& brave_engines_map =
      TemplateURLPrepopulateData::GetBraveEnginesMap();
  for (size_t i = 0; i < engine_ids.size(); ++i) {
    const PrepopulatedEngine* engine = brave_engines_map.at(engine_ids.at(i));
    DCHECK(engine);
    if (engine) {
      engines.push_back(engine);
      if (default_search_provider_index &&
          default_engine_id == engine_ids.at(i))
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
  std::vector<BravePrepopulatedEngineID> brave_engine_ids =
      brave_engines_default;

  // Check for a per-country override of this list
  const auto& it_country = default_engines_by_country_id_map.find(country_id);
  if (it_country != default_engines_by_country_id_map.end()) {
    brave_engine_ids = *it_country->second;
  }
  DCHECK_GT(brave_engine_ids.size(), 0ul);

  // Get the default engine (overridable by country)
  BravePrepopulatedEngineID default_id = default_engine;
  const auto& it_default = default_engine_by_country_id_map.find(country_id);
  if (it_default != default_engine_by_country_id_map.end()) {
    default_id = it_default->second;
  }

  // Allow for per-country overrides
  LocalizeEngineList(country_id, &brave_engine_ids);

  // Build a vector PrepopulatedEngines from BravePrepopulatedEngineIDs and
  // also get the default engine index
  std::vector<const PrepopulatedEngine*> engines =
      GetEnginesFromEngineIDs(brave_engine_ids, country_id,
                              default_id, default_search_provider_index);
  DCHECK(engines.size() == brave_engine_ids.size());

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

// Some engines (like Yahoo) have different URLs per country
// The intention of this function is to find the generic one
// (ex: PREPOPULATED_ENGINE_ID_YAHOO) and then substitute the
// country specific version.
// This function is not in the anonymous namespace because it
// is used in brave_template_url_service_util_unittest.
void LocalizeEngineList(int country_id,
                        std::vector<BravePrepopulatedEngineID>* engines) {
  for (size_t i = 0; i < engines->size(); ++i) {
    if ((*engines)[i] == PREPOPULATED_ENGINE_ID_YAHOO) {
      const auto& it = yahoo_engines_by_country_id_map.find(country_id);
      if (it != yahoo_engines_by_country_id_map.end()) {
        (*engines)[i] = it->second;
      }
    }
  }
}

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
