/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url_prepopulate_data.h"

#include <map>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "build/build_config.h"
#include "components/country_codes/country_codes.h"
#include "components/search_engines/search_engines_pref_names.h"

// IMPORTANT! If you make changes to any of the search engine mappings below,
// it's critical to also increment the value `kBraveCurrentDataVersion` in
// `//brave/components/search_engines/brave_prepopulated_engines.cc`.

namespace TemplateURLPrepopulateData {

// This redeclaration of the upstream prototype for `GetPrepopulatedEngines` is
// necessary, otherwise the translation unit fails to compile on calls for
// `GetPrepopulatedEngines` where there's an expectation for the use of the
// default value of the last arguent.
std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulatedEngines_Unused(
    PrefService* prefs,
    search_engines::SearchEngineChoiceService* search_engine_choice_service);

}  // namespace TemplateURLPrepopulateData

#define GetDataVersion GetDataVersion_ChromiumImpl
#if BUILDFLAG(IS_ANDROID)
#define GetLocalPrepopulatedEngines GetLocalPrepopulatedEngines_Unused
#endif
#define GetPrepopulatedFallbackSearch GetPrepopulatedFallbackSearch_Unused
#define GetPrepopulatedEngine GetPrepopulatedEngine_Unused
#define GetPrepopulatedEngines GetPrepopulatedEngines_Unused
#include "src/components/search_engines/template_url_prepopulate_data.cc"
#undef GetDataVersion
#if BUILDFLAG(IS_ANDROID)
#undef GetLocalPrepopulatedEngines
#endif
#undef GetPrepopulatedFallbackSearch
#undef GetPrepopulatedEngine
#undef GetPrepopulatedEngines

namespace TemplateURLPrepopulateData {

namespace {

// Default order in which engines will appear in the UI.
constexpr BravePrepopulatedEngineID kBraveEnginesDefault[] = {
    PREPOPULATED_ENGINE_ID_BRAVE,      PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO, PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,       PREPOPULATED_ENGINE_ID_STARTPAGE,
};

// Variations of the order / default options by country.
constexpr BravePrepopulatedEngineID kBraveEnginesWithEcosia[] = {
    PREPOPULATED_ENGINE_ID_BRAVE,      PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO, PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,       PREPOPULATED_ENGINE_ID_STARTPAGE,
    PREPOPULATED_ENGINE_ID_ECOSIA,
};

constexpr BravePrepopulatedEngineID kBraveEnginesWithYandex[] = {
    PREPOPULATED_ENGINE_ID_YANDEX,    PREPOPULATED_ENGINE_ID_BRAVE,
    PREPOPULATED_ENGINE_ID_GOOGLE,    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_QWANT,     PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

constexpr BravePrepopulatedEngineID kBraveEnginesDE[] = {
    PREPOPULATED_ENGINE_ID_BRAVE,  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
    PREPOPULATED_ENGINE_ID_QWANT,  PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_BING,   PREPOPULATED_ENGINE_ID_STARTPAGE,
    PREPOPULATED_ENGINE_ID_ECOSIA,
};

constexpr BravePrepopulatedEngineID kBraveEnginesFR[] = {
    PREPOPULATED_ENGINE_ID_BRAVE,  PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_GOOGLE, PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_BING,   PREPOPULATED_ENGINE_ID_STARTPAGE,
    PREPOPULATED_ENGINE_ID_ECOSIA,
};

constexpr BravePrepopulatedEngineID kBraveEnginesAUIE[] = {
    PREPOPULATED_ENGINE_ID_BRAVE,  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE,
    PREPOPULATED_ENGINE_ID_GOOGLE, PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,   PREPOPULATED_ENGINE_ID_STARTPAGE,
    PREPOPULATED_ENGINE_ID_ECOSIA,
};

constexpr BravePrepopulatedEngineID kBraveEnginesKR[] = {
    PREPOPULATED_ENGINE_ID_BRAVE,
    PREPOPULATED_ENGINE_ID_NAVER,
    PREPOPULATED_ENGINE_ID_DAUM,
    PREPOPULATED_ENGINE_ID_GOOGLE,
};

constexpr BravePrepopulatedEngineID kBraveEnginesNZ[] = {
    PREPOPULATED_ENGINE_ID_BRAVE,  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE,
    PREPOPULATED_ENGINE_ID_GOOGLE, PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_BING,   PREPOPULATED_ENGINE_ID_STARTPAGE,
};

// A map to keep track of a full list of default engines for countries
// that don't use the default list.
constexpr auto kDefaultEnginesByCountryIdMap =
    base::MakeFixedFlatMap<int, base::span<const BravePrepopulatedEngineID>>(
        {{country_codes::CountryCharsToCountryID('A', 'M'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('A', 'T'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('A', 'U'), kBraveEnginesAUIE},
         {country_codes::CountryCharsToCountryID('A', 'Z'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('B', 'E'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('B', 'Y'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('C', 'A'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('C', 'H'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('D', 'E'), kBraveEnginesDE},
         {country_codes::CountryCharsToCountryID('D', 'K'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('E', 'S'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('F', 'I'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('F', 'R'), kBraveEnginesFR},
         {country_codes::CountryCharsToCountryID('G', 'B'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('G', 'R'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('H', 'U'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('I', 'E'), kBraveEnginesAUIE},
         {country_codes::CountryCharsToCountryID('I', 'T'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('K', 'G'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('K', 'R'), kBraveEnginesKR},
         {country_codes::CountryCharsToCountryID('K', 'Z'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('L', 'U'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('M', 'D'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('N', 'L'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('N', 'O'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('N', 'Z'), kBraveEnginesNZ},
         {country_codes::CountryCharsToCountryID('P', 'T'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('R', 'U'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('S', 'E'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('T', 'J'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('T', 'M'),
          kBraveEnginesWithYandex},
         {country_codes::CountryCharsToCountryID('U', 'S'),
          kBraveEnginesWithEcosia},
         {country_codes::CountryCharsToCountryID('U', 'Z'),
          kBraveEnginesWithYandex}});

// A versioned map tracking the singular default search engine per-country.
//
// When a profile is created, the current value for `kBraveCurrentDataVersion`
// in `//brave/components/search_engines/brave_prepopulated_engines.cc` is
// stored as a profile preference.
//
// See:
// - `SetDefaultSearchVersion` in `//brave/browser/profiles/profile_util.cc`
// - `//brave/browser/profiles/brave_profile_manager.cc` where it is called
//
// If that person resets the profile using brave://settings/reset, we need to
// set the default search engine back to what it was when the profile was
// originally created. This way, a person doesn't get a new unexpected default
// when they reset the profile; it goes back to the original value.
BravePrepopulatedEngineID GetDefaultSearchEngine(int country_id, int version) {
  const BravePrepopulatedEngineID default_v6 = PREPOPULATED_ENGINE_ID_GOOGLE;
  static constexpr auto kContentV6 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'U'),
           PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_QWANT},
          {country_codes::CountryCharsToCountryID('I', 'E'),
           PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
          {country_codes::CountryCharsToCountryID('N', 'Z'),
           PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
      });
  static constexpr auto kContentV8 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'U'),
           PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_QWANT},
          {country_codes::CountryCharsToCountryID('I', 'E'),
           PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('N', 'Z'),
           PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });
  static constexpr auto kContentV16 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_QWANT},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });
  static constexpr auto kContentV17 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('C', 'A'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('G', 'B'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });

  static constexpr auto kContentV20 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'T'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('C', 'A'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('E', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('G', 'B'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'X'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });

  static constexpr auto kContentV21 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'T'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('C', 'A'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('E', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('G', 'B'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'X'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });
  static constexpr auto kContentV22 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'T'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('C', 'A'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('E', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('G', 'B'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('I', 'N'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'X'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });
  static constexpr auto kContentV25 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'T'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('C', 'A'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('E', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('G', 'B'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('I', 'N'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'R'),
           PREPOPULATED_ENGINE_ID_NAVER},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'X'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });
  // Updated default for IT.
  static constexpr auto kContentV26 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'T'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('C', 'A'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('E', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('G', 'B'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('I', 'N'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('I', 'T'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'R'),
           PREPOPULATED_ENGINE_ID_NAVER},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'X'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });
  // Updated default for AU.
  static constexpr auto kContentV30 =
      base::MakeFixedFlatMap<int, BravePrepopulatedEngineID>({
          {country_codes::CountryCharsToCountryID('A', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('A', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'T'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'U'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('A', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('B', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('B', 'Y'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('C', 'A'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('D', 'E'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('E', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('F', 'R'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('G', 'B'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('I', 'N'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('I', 'T'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('K', 'G'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('K', 'R'),
           PREPOPULATED_ENGINE_ID_NAVER},
          {country_codes::CountryCharsToCountryID('K', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'D'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('M', 'X'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('R', 'U'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'J'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('T', 'M'),
           PREPOPULATED_ENGINE_ID_YANDEX},
          {country_codes::CountryCharsToCountryID('U', 'S'),
           PREPOPULATED_ENGINE_ID_BRAVE},
          {country_codes::CountryCharsToCountryID('U', 'Z'),
           PREPOPULATED_ENGINE_ID_YANDEX},
      });

  if (version > 29) {
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

// Builds a vector of PrepulatedEngine objects from the given array of
// |engine_ids|.
std::vector<const PrepopulatedEngine*> GetEnginesFromEngineIDs(
    base::span<const BravePrepopulatedEngineID> engine_ids) {
  std::vector<const PrepopulatedEngine*> engines;
  const auto& brave_engines_map =
      TemplateURLPrepopulateData::GetBraveEnginesMap();
  for (BravePrepopulatedEngineID engine_id : engine_ids) {
    const PrepopulatedEngine* engine = brave_engines_map.at(engine_id);
    CHECK(engine);
    engines.push_back(engine);
  }
  return engines;
}

void UpdateTemplateURLDataKeyword(
    const std::unique_ptr<TemplateURLData>& t_urld) {
  DCHECK(t_urld.get());
  switch (t_urld->prepopulate_id) {
    case PREPOPULATED_ENGINE_ID_GOOGLE:
      t_urld->SetKeyword(u":g");
      break;
    case PREPOPULATED_ENGINE_ID_BING:
      t_urld->SetKeyword(u":b");
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
    int version = kBraveCurrentDataVersion) {
  base::span<const BravePrepopulatedEngineID> brave_engine_ids =
      kBraveEnginesDefault;

  // Check for a per-country override of this list
  const auto it_country = kDefaultEnginesByCountryIdMap.find(country_id);
  if (it_country != kDefaultEnginesByCountryIdMap.end()) {
    brave_engine_ids = it_country->second;
  }
  DCHECK_GT(brave_engine_ids.size(), 0ul);

  // Build a vector PrepopulatedEngines from BravePrepopulatedEngineIDs and
  // also get the default engine index
  std::vector<const PrepopulatedEngine*> engines =
      GetEnginesFromEngineIDs(brave_engine_ids);
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

// Redefines function with the same name in Chromium. Modifies the function to
// get search engines defined by Brave.
std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulatedEngines(
    PrefService* prefs,
    search_engines::SearchEngineChoiceService* search_engine_choice_service) {
  // If there is a set of search engines in the preferences file, it overrides
  // the built-in set.
  std::vector<std::unique_ptr<TemplateURLData>> t_urls =
      GetOverriddenTemplateURLData(prefs);
  if (!t_urls.empty()) {
    return t_urls;
  }

  int version = kBraveCurrentDataVersion;
  if (prefs && prefs->HasPrefPath(prefs::kBraveDefaultSearchVersion)) {
    version = prefs->GetInteger(prefs::kBraveDefaultSearchVersion);
  }

  return GetBravePrepopulatedEnginesForCountryID(
      search_engine_choice_service
          ? search_engine_choice_service->GetCountryId()
          : country_codes::GetCountryIDFromPrefs(prefs),
      version);
}

// Redefines function with the same name in Chromium. Modifies the function to
// get search engines defined by Brave.
#if BUILDFLAG(IS_ANDROID)
std::vector<std::unique_ptr<TemplateURLData>> GetLocalPrepopulatedEngines(
    const std::string& country_code,
    PrefService& prefs) {
  int country_id = country_codes::CountryStringToCountryID(country_code);
  if (country_id == country_codes::kCountryIDUnknown) {
    LOG(ERROR) << "Unknown country code specified: " << country_code;
    return std::vector<std::unique_ptr<TemplateURLData>>();
  }

  return GetBravePrepopulatedEnginesForCountryID(country_id);
}
#endif

// Chromium picks Google (if on the list, otherwise the first prepopulated on
// the list). We should return the default engine by country, or Brave.
std::unique_ptr<TemplateURLData> GetPrepopulatedFallbackSearch(
    PrefService* prefs,
    search_engines::SearchEngineChoiceService* search_engine_choice_service) {
  std::vector<std::unique_ptr<TemplateURLData>> prepopulated_engines =
      GetPrepopulatedEngines(prefs, search_engine_choice_service);
  if (prepopulated_engines.empty()) {
    return nullptr;
  }

  // Get the default engine (overridable by country) for this version
  int version = kBraveCurrentDataVersion;
  if (prefs && prefs->HasPrefPath(prefs::kBraveDefaultSearchVersion)) {
    version = prefs->GetInteger(prefs::kBraveDefaultSearchVersion);
  }

  int country_id = search_engine_choice_service
                       ? search_engine_choice_service->GetCountryId()
                       : country_codes::GetCountryIDFromPrefs(prefs);

  BravePrepopulatedEngineID default_id =
      GetDefaultSearchEngine(country_id, version);

  std::unique_ptr<TemplateURLData> brave_engine;
  for (auto& engine : prepopulated_engines) {
    if (engine->prepopulate_id == static_cast<int>(default_id)) {
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
    PrefService* prefs,
    search_engines::SearchEngineChoiceService* search_engine_choice_service,
    int prepopulated_id) {
  auto engines = TemplateURLPrepopulateData::GetPrepopulatedEngines(
      prefs, search_engine_choice_service);
  for (auto& engine : engines) {
    if (engine->prepopulate_id == prepopulated_id) {
      return std::move(engine);
    }
  }
  return nullptr;
}

}  // namespace TemplateURLPrepopulateData
