/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/regional_capabilities/regional_capabilities_utils.h"

#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"

// Use Brave's lists of per-country engines.
#define GetPrepopulatedEngines GetPrepopulatedEngines_UnUsed
#include "src/components/regional_capabilities/regional_capabilities_utils.cc"
#undef GetPrepopulatedEngines

namespace regional_capabilities {

namespace {

// ****************************************************************************
// IMPORTANT! If you make changes to any of the search engine mappings below,
// it's critical to also increment the value `kBraveCurrentDataVersion` in
// `brave/components/search_engines/brave_prepopulated_engines.h`.
// ****************************************************************************

// Default order in which engines will appear in the UI.
constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesDefault[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE,
};

// Variations of the order / default options by country.
constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesWithEcosia[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_ECOSIA,
};

constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesWithYandex[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE,
};

constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesDE[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_ECOSIA,
};

constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesFR[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_ECOSIA,
};

constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesAUIE[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_ECOSIA,
};

constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesJP[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YAHOO_JP,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE,
};

constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesKR[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_NAVER,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DAUM,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
};

constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
    kBraveEnginesNZ[] = {
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE,
};

// A map to keep track of a full list of default engines for countries
// that don't use the default list.
constexpr auto kDefaultEnginesByCountryIdMap = base::MakeFixedFlatMap<
    country_codes::CountryId,
    base::span<const TemplateURLPrepopulateData::BravePrepopulatedEngineID>>(
    {{country_codes::CountryId("AM"), kBraveEnginesWithYandex},
     {country_codes::CountryId("AT"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("AU"), kBraveEnginesAUIE},
     {country_codes::CountryId("AZ"), kBraveEnginesWithYandex},
     {country_codes::CountryId("BE"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("BY"), kBraveEnginesWithYandex},
     {country_codes::CountryId("CA"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("CH"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("DE"), kBraveEnginesDE},
     {country_codes::CountryId("DK"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("ES"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("FI"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("FR"), kBraveEnginesFR},
     {country_codes::CountryId("GB"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("GR"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("HU"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("IE"), kBraveEnginesAUIE},
     {country_codes::CountryId("IT"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("JP"), kBraveEnginesJP},
     {country_codes::CountryId("KG"), kBraveEnginesWithYandex},
     {country_codes::CountryId("KR"), kBraveEnginesKR},
     {country_codes::CountryId("KZ"), kBraveEnginesWithYandex},
     {country_codes::CountryId("LU"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("MD"), kBraveEnginesWithYandex},
     {country_codes::CountryId("NL"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("NO"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("NZ"), kBraveEnginesNZ},
     {country_codes::CountryId("PT"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("RU"), kBraveEnginesWithYandex},
     {country_codes::CountryId("SE"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("TJ"), kBraveEnginesWithYandex},
     {country_codes::CountryId("TM"), kBraveEnginesWithYandex},
     {country_codes::CountryId("US"), kBraveEnginesWithEcosia},
     {country_codes::CountryId("UZ"), kBraveEnginesWithYandex}});

// Builds a vector of PrepulatedEngine objects from the given array of
// |engine_ids|.
std::vector<const PrepopulatedEngine*> GetEnginesFromEngineIDs(
    base::span<const TemplateURLPrepopulateData::BravePrepopulatedEngineID>
        engine_ids) {
  std::vector<const PrepopulatedEngine*> engines;
  const auto& brave_engines_map =
      TemplateURLPrepopulateData::GetBraveEnginesMap();
  for (TemplateURLPrepopulateData::BravePrepopulatedEngineID engine_id :
       engine_ids) {
    const PrepopulatedEngine* engine = brave_engines_map.at(engine_id);
    CHECK(engine);
    engines.push_back(engine);
  }
  return engines;
}

// Uses brave_engines_XX localized arrays of engine IDs instead of Chromium's
// localized arrays of PrepopulatedEngines to construct the vector of
// TemplateURLData.
std::vector<const PrepopulatedEngine*> GetBravePrepopulatedEnginesForCountryID(
    country_codes::CountryId country_id) {
  base::span<const TemplateURLPrepopulateData::BravePrepopulatedEngineID>
      brave_engine_ids = kBraveEnginesDefault;

  // Check for a per-country override of this list
  const auto it_country = kDefaultEnginesByCountryIdMap.find(country_id);
  if (it_country != kDefaultEnginesByCountryIdMap.end()) {
    brave_engine_ids = it_country->second;
  }
  DCHECK_GT(brave_engine_ids.size(), 0ul);

  // Build a vector PrepopulatedEngines from
  // TemplateURLPrepopulateData::BravePrepopulatedEngineIDs.
  std::vector<const PrepopulatedEngine*> engines =
      GetEnginesFromEngineIDs(brave_engine_ids);
  DCHECK(engines.size() == brave_engine_ids.size());

  return engines;
}

// A versioned map tracking the singular default search engine per-country.
//
// When a profile is created, the current value for `kBraveCurrentDataVersion`
// in `//brave/components/search_engines/brave_prepopulated_engines.h`
// is stored as a profile preference.
//
// See:
// - `SetDefaultSearchVersion` in `//brave/browser/profiles/profile_util.cc`
// - `//brave/browser/profiles/brave_profile_manager.cc` where it is called
//
// If that person resets the profile using brave://settings/reset, we need to
// set the default search engine back to what it was when the profile was
// originally created. This way, a person doesn't get a new unexpected default
// when they reset the profile; it goes back to the original value.
TemplateURLPrepopulateData::BravePrepopulatedEngineID GetDefaultSearchEngine(
    country_codes::CountryId country_id,
    int version) {
  const TemplateURLPrepopulateData::BravePrepopulatedEngineID default_v6 =
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE;
  static constexpr auto kContentV6 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT},
      {country_codes::CountryId("IE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
      {country_codes::CountryId("NZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
  });
  static constexpr auto kContentV8 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT},
      {country_codes::CountryId("IE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("NZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });
  static constexpr auto kContentV16 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });
  static constexpr auto kContentV17 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("CA"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("GB"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("US"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });

  static constexpr auto kContentV20 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("CA"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("ES"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("GB"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MX"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("US"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });

  static constexpr auto kContentV21 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("CA"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("ES"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("GB"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MX"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("US"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });
  static constexpr auto kContentV22 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("CA"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("ES"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("GB"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("IN"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MX"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("US"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });
  static constexpr auto kContentV25 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("CA"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("ES"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("GB"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("IN"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_NAVER},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MX"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("US"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });
  // Updated default for IT.
  static constexpr auto kContentV26 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("CA"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("ES"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("GB"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("IN"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("IT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_NAVER},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MX"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("US"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });
  // Updated default for AU.
  static constexpr auto kContentV30 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("CA"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("ES"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("GB"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("IN"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("IT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_NAVER},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MX"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("US"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });

  // Updated default for JP.
  static constexpr auto kContentV31 = base::MakeFixedFlatMap<
      country_codes::CountryId,
      TemplateURLPrepopulateData::BravePrepopulatedEngineID>({
      {country_codes::CountryId("AM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("AR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("AZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("BR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("BY"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("CA"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("DE"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("ES"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("FR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("GB"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("IN"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("IT"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("JP"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YAHOO_JP},
      {country_codes::CountryId("KG"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("KR"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_NAVER},
      {country_codes::CountryId("KZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MD"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("MX"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("RU"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TJ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("TM"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
      {country_codes::CountryId("US"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE},
      {country_codes::CountryId("UZ"),
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YANDEX},
  });

  if (version > 30) {
    const auto it = kContentV31.find(country_id);
    if (it == kContentV31.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 29) {
    const auto it = kContentV30.find(country_id);
    if (it == kContentV30.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 25) {
    const auto it = kContentV26.find(country_id);
    if (it == kContentV26.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 24) {
    const auto it = kContentV25.find(country_id);
    if (it == kContentV25.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 21) {
    const auto it = kContentV22.find(country_id);
    if (it == kContentV22.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 20) {
    const auto it = kContentV21.find(country_id);
    if (it == kContentV21.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 19) {
    const auto it = kContentV20.find(country_id);
    if (it == kContentV20.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 16) {
    const auto it = kContentV17.find(country_id);
    if (it == kContentV17.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 15) {
    const auto it = kContentV16.find(country_id);
    if (it == kContentV16.end()) {
      return default_v6;
    }
    return it->second;
  } else if (version > 7) {
    const auto it = kContentV8.find(country_id);
    if (it == kContentV8.end()) {
      return default_v6;
    }
    return it->second;
  } else {
    const auto it = kContentV6.find(country_id);
    if (it == kContentV6.end()) {
      return default_v6;
    }
    return it->second;
  }
}

}  // namespace

std::vector<const PrepopulatedEngine*> GetPrepopulatedEngines(
    CountryId country_id,
    PrefService& prefs) {
  return GetBravePrepopulatedEnginesForCountryID(country_id);
}

TemplateURLPrepopulateData::BravePrepopulatedEngineID GetDefaultEngine(
    CountryId country_id,
    PrefService& prefs) {
  int version = TemplateURLPrepopulateData::kBraveCurrentDataVersion;
  if (prefs.HasPrefPath(::prefs::kBraveDefaultSearchVersion)) {
    version = prefs.GetInteger(::prefs::kBraveDefaultSearchVersion);
  }

  return GetDefaultSearchEngine(country_id, version);
}

}  // namespace regional_capabilities
