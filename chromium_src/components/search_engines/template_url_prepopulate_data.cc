/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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

// Pull in definitions for Brave prepopulated engines. It's ugly but these need
// to be built as part of the search_engines static library.
#include "../../../components/search_engines/brave_prepopulated_engines.cc"
#include "../../../components/search_engines/brave_prepopulated_engines.h"

namespace TemplateURLPrepopulateData {

namespace {

// Maps Brave engine id to PrepopulatedEngine.
const std::map<BravePrepopulatedEngineID, const PrepopulatedEngine*>
    brave_engines_map = {
        {PREPOPULATED_ENGINE_ID_GOOGLE, &google},
        {PREPOPULATED_ENGINE_ID_YAHOO, &yahoo},
        {PREPOPULATED_ENGINE_ID_YAHOO_QC, &yahoo_qc},
        {PREPOPULATED_ENGINE_ID_BING, &bing},
        {PREPOPULATED_ENGINE_ID_YANDEX, &yandex},
        {PREPOPULATED_ENGINE_ID_AMAZON, &amazon},
        {PREPOPULATED_ENGINE_ID_DUCKDUCKGO, &duckduckgo},
        {PREPOPULATED_ENGINE_ID_ECOSIA, &ecosia},
        {PREPOPULATED_ENGINE_ID_FINDX, &findx},
        {PREPOPULATED_ENGINE_ID_GITHUB, &github},
        {PREPOPULATED_ENGINE_ID_INFOGALACTIC, &infogalactic},
        {PREPOPULATED_ENGINE_ID_MDNWEBDOCS, &mdnwebdocs},
        {PREPOPULATED_ENGINE_ID_QWANT, &qwant},
        {PREPOPULATED_ENGINE_ID_SEARX, &searx},
        {PREPOPULATED_ENGINE_ID_SEMANTICSCHOLAR, &semanticscholar},
        {PREPOPULATED_ENGINE_ID_STACKOVERFLOW, &stackoverflow},
        {PREPOPULATED_ENGINE_ID_STARTPAGE, &startpage},
        {PREPOPULATED_ENGINE_ID_TWITTER, &twitter},
        {PREPOPULATED_ENGINE_ID_WIKIPEDIA, &wikipedia},
        {PREPOPULATED_ENGINE_ID_WOLFRAMALPHA, &wolframalpha},
        {PREPOPULATED_ENGINE_ID_YOUTUBE, &youtube},
};

// Engines that have localized versions
const BravePrepopulatedEngineID localized_engines[] = {
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_YANDEX,
};

// Engine ID to use as the default engine.
const BravePrepopulatedEngineID kDefaultEngineID =
    PREPOPULATED_ENGINE_ID_GOOGLE;

// A map to keep track of default engines for countries that don't use the
// regular default engine.
const std::map<int, BravePrepopulatedEngineID>
    default_engine_by_country_id_map = {
        {CODE_TO_ID(D, E), PREPOPULATED_ENGINE_ID_QWANT},
        {CODE_TO_ID(F, R), PREPOPULATED_ENGINE_ID_QWANT},
};

// Default order in which engines will appear in the UI.
const BravePrepopulatedEngineID brave_engines_default[] = {
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_AMAZON,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_ECOSIA,
    PREPOPULATED_ENGINE_ID_FINDX,
    PREPOPULATED_ENGINE_ID_GITHUB,
    PREPOPULATED_ENGINE_ID_INFOGALACTIC,
    PREPOPULATED_ENGINE_ID_MDNWEBDOCS,
    PREPOPULATED_ENGINE_ID_SEARX,
    PREPOPULATED_ENGINE_ID_SEMANTICSCHOLAR,
    PREPOPULATED_ENGINE_ID_STACKOVERFLOW,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
    PREPOPULATED_ENGINE_ID_TWITTER,
    PREPOPULATED_ENGINE_ID_WIKIPEDIA,
    PREPOPULATED_ENGINE_ID_WOLFRAMALPHA,
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_YANDEX,
    PREPOPULATED_ENGINE_ID_YOUTUBE,
};

// Canada - adds Yahoo QC.
const BravePrepopulatedEngineID brave_engines_CA[] = {
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_AMAZON,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_ECOSIA,
    PREPOPULATED_ENGINE_ID_FINDX,
    PREPOPULATED_ENGINE_ID_GITHUB,
    PREPOPULATED_ENGINE_ID_INFOGALACTIC,
    PREPOPULATED_ENGINE_ID_MDNWEBDOCS,
    PREPOPULATED_ENGINE_ID_SEARX,
    PREPOPULATED_ENGINE_ID_SEMANTICSCHOLAR,
    PREPOPULATED_ENGINE_ID_STACKOVERFLOW,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
    PREPOPULATED_ENGINE_ID_TWITTER,
    PREPOPULATED_ENGINE_ID_WIKIPEDIA,
    PREPOPULATED_ENGINE_ID_WOLFRAMALPHA,
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_YAHOO_QC,
    PREPOPULATED_ENGINE_ID_YANDEX,
    PREPOPULATED_ENGINE_ID_YOUTUBE,
};

// Germany - Qwant appears on top.
const BravePrepopulatedEngineID brave_engines_DE[] = {
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_AMAZON,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_ECOSIA,
    PREPOPULATED_ENGINE_ID_FINDX,
    PREPOPULATED_ENGINE_ID_GITHUB,
    PREPOPULATED_ENGINE_ID_INFOGALACTIC,
    PREPOPULATED_ENGINE_ID_MDNWEBDOCS,
    PREPOPULATED_ENGINE_ID_SEARX,
    PREPOPULATED_ENGINE_ID_SEMANTICSCHOLAR,
    PREPOPULATED_ENGINE_ID_STACKOVERFLOW,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
    PREPOPULATED_ENGINE_ID_TWITTER,
    PREPOPULATED_ENGINE_ID_WIKIPEDIA,
    PREPOPULATED_ENGINE_ID_WOLFRAMALPHA,
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_YANDEX,
    PREPOPULATED_ENGINE_ID_YOUTUBE,
};

// France - Qwant appears on top.
const BravePrepopulatedEngineID brave_engines_FR[] = {
    PREPOPULATED_ENGINE_ID_QWANT,
    PREPOPULATED_ENGINE_ID_GOOGLE,
    PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
    PREPOPULATED_ENGINE_ID_AMAZON,
    PREPOPULATED_ENGINE_ID_BING,
    PREPOPULATED_ENGINE_ID_ECOSIA,
    PREPOPULATED_ENGINE_ID_FINDX,
    PREPOPULATED_ENGINE_ID_GITHUB,
    PREPOPULATED_ENGINE_ID_INFOGALACTIC,
    PREPOPULATED_ENGINE_ID_MDNWEBDOCS,
    PREPOPULATED_ENGINE_ID_SEARX,
    PREPOPULATED_ENGINE_ID_SEMANTICSCHOLAR,
    PREPOPULATED_ENGINE_ID_STACKOVERFLOW,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
    PREPOPULATED_ENGINE_ID_TWITTER,
    PREPOPULATED_ENGINE_ID_WIKIPEDIA,
    PREPOPULATED_ENGINE_ID_WOLFRAMALPHA,
    PREPOPULATED_ENGINE_ID_YAHOO,
    PREPOPULATED_ENGINE_ID_YANDEX,
    PREPOPULATED_ENGINE_ID_YOUTUBE,
};

// Retrieves appropriate to the |country_id| localized version of Yahoo
// engine.
const PrepopulatedEngine* GetLocalizedYahooEngineForCountryID(int country_id) {
  switch (country_id) {
    UNHANDLED_COUNTRY(A, R)  // Argentina
    return &yahoo_ar;
    UNHANDLED_COUNTRY(A, T)  // Austria
    return &yahoo_at;
    UNHANDLED_COUNTRY(A, U)  // Australia and countries that default to it
    UNHANDLED_COUNTRY(C, C)  // Cocos Islands
    UNHANDLED_COUNTRY(C, X)  // Christmas Island
    UNHANDLED_COUNTRY(H, M)  // Heard Island and McDonald Islands
    UNHANDLED_COUNTRY(N, F)  // Norfolk Island
    return &yahoo_au;
    UNHANDLED_COUNTRY(B, R)  // Brazil
    return &yahoo_br;
    UNHANDLED_COUNTRY(C, A)  // Canada
    return &yahoo_ca;
    UNHANDLED_COUNTRY(C, H)  // Switzerland
    return &yahoo_ch;
    UNHANDLED_COUNTRY(C, L)  // Chile
    return &yahoo_cl;
    UNHANDLED_COUNTRY(D, E)  // Germany
    return &yahoo_de;
    UNHANDLED_COUNTRY(D, K)  // Denmark and countries that default to it
    UNHANDLED_COUNTRY(G, L)  // Greenland
    return &yahoo_dk;
    UNHANDLED_COUNTRY(E, S)  // Spain and countries that default to it
    UNHANDLED_COUNTRY(A, D)  // Andorra
    return &yahoo_es;
    UNHANDLED_COUNTRY(F, I)  // Finland and countries that default to it
    UNHANDLED_COUNTRY(A, X)  // Aland Islands
    return &yahoo_fi;
    UNHANDLED_COUNTRY(F, R)  // France and countries that default to it
    UNHANDLED_COUNTRY(M, L)  // Mali
    UNHANDLED_COUNTRY(M, Q)  // Martinique
    UNHANDLED_COUNTRY(N, C)  // New Caledonia
    UNHANDLED_COUNTRY(N, E)  // Niger
    UNHANDLED_COUNTRY(P, F)  // French Polynesia
    UNHANDLED_COUNTRY(P, M)  // Saint Pierre and Miquelon
    UNHANDLED_COUNTRY(R, E)  // Reunion
    UNHANDLED_COUNTRY(S, N)  // Senegal
    UNHANDLED_COUNTRY(T, D)  // Chad
    UNHANDLED_COUNTRY(T, F)  // French Southern Territories
    UNHANDLED_COUNTRY(T, G)  // Togo
    UNHANDLED_COUNTRY(W, F)  // Wallis and Futuna
    UNHANDLED_COUNTRY(Y, T)  // Mayotte
    return &yahoo_fr;
    UNHANDLED_COUNTRY(G, R)  // Greece and countries that default to it
    UNHANDLED_COUNTRY(C, Y)  // Cyprus
    return &yahoo_gr;
    UNHANDLED_COUNTRY(H, K)  // Hong Kong
    return &yahoo_hk;
    UNHANDLED_COUNTRY(I, D)  // Indonesia
    return &yahoo_id;
    UNHANDLED_COUNTRY(I, N)  // India
    return &yahoo_in;
    UNHANDLED_COUNTRY(J, P)  // Japan
    return &yahoo_jp;
    UNHANDLED_COUNTRY(A, E)  // United Arab Emirates
    UNHANDLED_COUNTRY(B, H)  // Bahrain
    UNHANDLED_COUNTRY(D, Z)  // Algeria
    UNHANDLED_COUNTRY(E, G)  // Egypt
    UNHANDLED_COUNTRY(I, Q)  // Iraq
    UNHANDLED_COUNTRY(J, O)  // Jordan
    UNHANDLED_COUNTRY(K, W)  // Kuwait
    UNHANDLED_COUNTRY(L, B)  // Lebanon
    UNHANDLED_COUNTRY(L, Y)  // Libya
    UNHANDLED_COUNTRY(M, A)  // Morocco
    UNHANDLED_COUNTRY(O, M)  // Oman
    UNHANDLED_COUNTRY(Q, A)  // Qatar
    UNHANDLED_COUNTRY(S, A)  // Saudi Arabia
    UNHANDLED_COUNTRY(S, Y)  // Syria
    UNHANDLED_COUNTRY(T, N)  // Tunisia
    UNHANDLED_COUNTRY(Y, E)  // Yemen
    return &yahoo_maktoob;
    UNHANDLED_COUNTRY(M, X)  // Mexico
    return &yahoo_mx;
    UNHANDLED_COUNTRY(M, Y)  // Malaysia
    return &yahoo_my;
    UNHANDLED_COUNTRY(N, L)  // Netherlands and countries that default to it
    UNHANDLED_COUNTRY(A, N)  // Netherlands Antilles
    UNHANDLED_COUNTRY(A, W)  // Aruba
    return &yahoo_nl;
    UNHANDLED_COUNTRY(N, Z)  // New Zealand and countries that default to it
    UNHANDLED_COUNTRY(C, K)  // Cook Islands
    UNHANDLED_COUNTRY(N, U)  // Niue
    UNHANDLED_COUNTRY(T, K)  // Tokelau
    return &yahoo_nz;
    UNHANDLED_COUNTRY(P, E)  // Peru
    return &yahoo_pe;
    UNHANDLED_COUNTRY(P, H)  // Philippines
    return &yahoo_ph;
    UNHANDLED_COUNTRY(R, O)  // Romania
    return &yahoo_ro;
    UNHANDLED_COUNTRY(S, E)  // Sweden
    return &yahoo_se;
    UNHANDLED_COUNTRY(S, G)  // Singapore
    return &yahoo_sg;
    UNHANDLED_COUNTRY(T, H)  // Thailand
    return &yahoo_th;
    UNHANDLED_COUNTRY(T, R)  // Turkey
    return &yahoo_tr;
    UNHANDLED_COUNTRY(T, W)  // Taiwan
    return &yahoo_tw;
    UNHANDLED_COUNTRY(G, B)  // United Kingdom and countries that default to it
    UNHANDLED_COUNTRY(B, M)  // Bermuda
    UNHANDLED_COUNTRY(F, K)  // Falkland Islands
    UNHANDLED_COUNTRY(G, G)  // Guernsey
    UNHANDLED_COUNTRY(G, I)  // Gibraltar
    UNHANDLED_COUNTRY(G, S)  // South Georgia and the South Sandwich
                             //   Islands
    UNHANDLED_COUNTRY(I, E)  // Ireland
    UNHANDLED_COUNTRY(I, M)  // Isle of Man
    UNHANDLED_COUNTRY(I, O)  // British Indian Ocean Territory
    UNHANDLED_COUNTRY(J, E)  // Jersey
    UNHANDLED_COUNTRY(K, Y)  // Cayman Islands
    UNHANDLED_COUNTRY(M, S)  // Montserrat
    UNHANDLED_COUNTRY(M, T)  // Malta
    UNHANDLED_COUNTRY(P, N)  // Pitcairn Islands
    UNHANDLED_COUNTRY(S, H)  // Saint Helena, Ascension Island, and Tristan da
                             //   Cunha
    UNHANDLED_COUNTRY(T, C)  // Turks and Caicos Islands
    UNHANDLED_COUNTRY(V, G)  // British Virgin Islands
    return &yahoo_uk;
    UNHANDLED_COUNTRY(V, E)  // Venezuela
    return &yahoo_ve;
    UNHANDLED_COUNTRY(V, N)  // Vietnam
    return &yahoo_vn;
    default:
      return &yahoo;
  }
}

// Retrieves appropriate to the |country_id| localized version of Yandex
// engine.
const PrepopulatedEngine* GetLocalizedYandexEngineForCountryID(int country_id) {
  switch (country_id) {
    UNHANDLED_COUNTRY(B, Y)  // Belarus
    return &yandex_by;
    UNHANDLED_COUNTRY(K, Z)  // Kazakhstan
    return &yandex_kz;
    UNHANDLED_COUNTRY(R, U)  // Russia and countries that default to it
    UNHANDLED_COUNTRY(A, M)  // Armenia
    UNHANDLED_COUNTRY(A, Z)  // Azerbaijan
    UNHANDLED_COUNTRY(K, G)  // Kyrgyzstan
    UNHANDLED_COUNTRY(L, T)  // Lithuania
    UNHANDLED_COUNTRY(L, V)  // Latvia
    UNHANDLED_COUNTRY(T, J)  // Tajikistan
    UNHANDLED_COUNTRY(T, M)  // Turkmenistan
    UNHANDLED_COUNTRY(U, Z)  // Uzbekistan
    return &yandex_ru;
    default:
      return &yandex;
  }
}

// Retrieves appropriate to the |country_id| localized version of a localized
// engine with the given |engine_id|.
const PrepopulatedEngine* GetLocalizedEngineForCountryID(
    BravePrepopulatedEngineID engine_id,
    int country_id) {
  switch (engine_id) {
    case PREPOPULATED_ENGINE_ID_YAHOO:
      return GetLocalizedYahooEngineForCountryID(country_id);
    case PREPOPULATED_ENGINE_ID_YANDEX:
      return GetLocalizedYandexEngineForCountryID(country_id);
    default:
      // Only engines that have localized versions (as listed in
      // localized_engines) should be passed in here. If you want to add a new
      // localized engine then add it into the localized_engines and add a
      // function to get the localized engine for that engine id.
      DCHECK(false);
      LOG(ERROR) << "No localized prepopulated engines defined for engine ID: "
                 << engine_id;
      return nullptr;
  }
}

// Checks if the engine with the given |engine_id| has a localized version
bool EngineHasLocalizedVersion(BravePrepopulatedEngineID engine_id) {
  return std::find(std::begin(localized_engines), std::end(localized_engines),
                   engine_id) != std::end(localized_engines);
}

// Builds a vector of PrepulatedEngine objects from the given array of
// |engine_ids|. Takes into account localized engines and finds localized
// engine for the given |country id|. Also, fills in the default engine
// index for the given |country_id|, if asked.
std::vector<const PrepopulatedEngine*> GetEnginesFromEngineIDs(
    const BravePrepopulatedEngineID engine_ids[],
    size_t num_ids,
    int country_id,
    BravePrepopulatedEngineID default_engine_id,
    size_t* default_search_provider_index = nullptr) {
  DCHECK(engine_ids);
  DCHECK(num_ids);
  std::vector<const PrepopulatedEngine*> engines;
  for (size_t i = 0; i < num_ids; ++i) {
    const PrepopulatedEngine* engine = nullptr;
    if (EngineHasLocalizedVersion(engine_ids[i]))
      engine = GetLocalizedEngineForCountryID(engine_ids[i], country_id);
    else
      engine = brave_engines_map.at(engine_ids[i]);

    DCHECK(engine);
    if (engine) {
      engines.push_back(engine);
      if (default_search_provider_index && default_engine_id == engine_ids[i])
        *default_search_provider_index = i;
    }
  }
  return engines;
}

void UpdateTemplateURLDataKeyword(std::unique_ptr<TemplateURLData>& t_urld) {
  DCHECK(t_urld.get());
  switch (t_urld->prepopulate_id) {
    case PREPOPULATED_ENGINE_ID_GOOGLE:
      t_urld->SetKeyword(base::ASCIIToUTF16(":g"));
      break;
    case PREPOPULATED_ENGINE_ID_YAHOO:
      t_urld->SetKeyword(base::ASCIIToUTF16(":y"));
      break;
    case PREPOPULATED_ENGINE_ID_YAHOO_QC:
      t_urld->SetKeyword(base::ASCIIToUTF16(":yq"));
      break;
    case PREPOPULATED_ENGINE_ID_BING:
      t_urld->SetKeyword(base::ASCIIToUTF16(":b"));
      break;
    case PREPOPULATED_ENGINE_ID_YANDEX:
      t_urld->SetKeyword(base::ASCIIToUTF16(":ya"));
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
  const BravePrepopulatedEngineID* brave_engines;
  size_t num_brave_engines;
  // Check for exceptions from the default list of engines
  if (CountryCharsToCountryID('C', 'A') == country_id) {
    brave_engines = brave_engines_CA;
    num_brave_engines = arraysize(brave_engines_CA);
  } else if (CountryCharsToCountryID('D', 'E') == country_id) {
    brave_engines = brave_engines_DE;
    num_brave_engines = arraysize(brave_engines_DE);
  } else if (CountryCharsToCountryID('F', 'R') == country_id) {
    brave_engines = brave_engines_FR;
    num_brave_engines = arraysize(brave_engines_FR);
  } else {
    brave_engines = brave_engines_default;
    num_brave_engines = arraysize(brave_engines_default);
  }
  DCHECK(brave_engines);
  DCHECK(num_brave_engines);

  // Check for an exception to the default engine
  BravePrepopulatedEngineID default_id = kDefaultEngineID;
  const auto& it = default_engine_by_country_id_map.find(country_id);
  if (it != default_engine_by_country_id_map.end())
    default_id = it->second;

  // Build a vector PrepopulatedEngines from BravePrepopulatedEngineIDs and
  // also get the default engine index
  std::vector<const PrepopulatedEngine*> engines =
      GetEnginesFromEngineIDs(brave_engines, num_brave_engines, country_id,
                              default_id, default_search_provider_index);
  DCHECK(engines.size() == num_brave_engines);

  std::vector<std::unique_ptr<TemplateURLData>> t_urls;
  for (const PrepopulatedEngine* engine : engines) {
    std::unique_ptr<TemplateURLData> t_urld =
        TemplateURLDataFromPrepopulatedEngine(*engine);
    UpdateTemplateURLDataKeyword(t_urld);
    t_urls.push_back(std::move(t_urld));
  }

  return t_urls;
}

};  // namespace

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

  return GetBravePrepopulatedEnginesForCountryID(GetCountryIDFromPrefs(prefs),
                                                 default_search_provider_index);
}

// Redefines function with the same name in Chromium. Modifies the function to
// get search engines defined by Brave.
#if defined(OS_ANDROID)

std::vector<std::unique_ptr<TemplateURLData>> GetLocalPrepopulatedEngines(
    const std::string& locale) {
  int country_id = CountryStringToCountryID(locale);
  if (country_id == kCountryIDUnknown) {
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
