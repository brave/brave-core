/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_
#define BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_

#include <map>

#include "build/build_config.h"
#include "third_party/search_engines_data/resources/definitions/prepopulated_engines.h"

namespace TemplateURLPrepopulateData {

// ****************************************************************************
// IMPORTANT! Make sure to bump the value of kBraveCurrentDataVersion in
// here if you add, remove, or make changes to the engines in here or
// brave_prepopulated_engines.cc or to mappings in
// chromium_src/components/regional_capabilities/regional_capabilities_utils.cc.
// ****************************************************************************

// The version is important to increment because Chromium will cache the list
// of search engines that are shown. When the version is incremented, Chromium
// will conditionally merge changes from the new version of the list.
//
// For more info, see:
// ComputeMergeEnginesRequirements in components/search_engines/util.cc;

inline constexpr int kBraveCurrentDataVersion = 32;

// DO NOT CHANGE THIS ONE. Used for backfilling kBraveDefaultSearchVersion.
inline constexpr int kBraveFirstTrackedDataVersion = 6;

// See comments on prepopulated engines ids in
// components/search_engines/prepopulated_engines_schema.json above the
// definition of the id field and in
// third_party/search_engines_data/resources/definitions/prepopulated_engines.json
// at the top of the file. Currently taken ids range under 120, but we'd want to
// leave room for additions by Chromium, so starting our ids from 500. If
// Chromium adds one of these engines to their list with a different id we need
// to either patch theirs out or use ModifyEngineParams function to match our
// previous definition (likely including the engine id).
enum BravePrepopulatedEngineID : unsigned int {
  PREPOPULATED_ENGINE_ID_INVALID = 0,

  // These engine IDs are already defined in prepopulated_engines.json
  PREPOPULATED_ENGINE_ID_GOOGLE = 1,
  PREPOPULATED_ENGINE_ID_YAHOO_JP = 2,
  PREPOPULATED_ENGINE_ID_BING = 3,
  PREPOPULATED_ENGINE_ID_YANDEX = 15,
  PREPOPULATED_ENGINE_ID_NAVER = 67,
  PREPOPULATED_ENGINE_ID_DAUM = 68,
  PREPOPULATED_ENGINE_ID_ECOSIA = 101,
  // These engine IDs are not defined in Chromium
  // When adding a new engine, also add it to kBraveAddedEngines in
  // chromium_src/components/search_engines/
  //   brave_template_url_prepopulate_data_unittest.cc, so that we would know if
  // Chromium adds the same engine in the future.
  BRAVE_PREPOPULATED_ENGINES_START = 500,
  PREPOPULATED_ENGINE_ID_AMAZON = 500,  // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
#if BUILDFLAG(IS_ANDROID)
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE,  // No longer in defaults (7/2020).
#endif
  PREPOPULATED_ENGINE_ID_FINDX,         // No longer exists (11/2018).
  PREPOPULATED_ENGINE_ID_GITHUB,        // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_INFOGALACTIC,  // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_MDNWEBDOCS,    // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_QWANT,
  PREPOPULATED_ENGINE_ID_SEARX,            // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_SEMANTICSCHOLAR,  // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_STACKOVERFLOW,    // No longer in defaults (2/2019).

  PREPOPULATED_ENGINE_ID_STARTPAGE,  // This ID was used before Chromium added
                                     // startpage to their prepopulated engines
                                     // (with id 113). We modify their engine
                                     // to use our id so that we don't have to
                                     // replace engines saved in user prefs.

  PREPOPULATED_ENGINE_ID_TWITTER,       // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_WIKIPEDIA,     // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_WOLFRAMALPHA,  // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_YOUTUBE,       // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE,

  // Yahoo deprecated (12/2020)
  PREPOPULATED_ENGINE_ID_YAHOO,
  PREPOPULATED_ENGINE_ID_YAHOO_AR,
  PREPOPULATED_ENGINE_ID_YAHOO_AT,
  PREPOPULATED_ENGINE_ID_YAHOO_AU,
  PREPOPULATED_ENGINE_ID_YAHOO_BR,
  PREPOPULATED_ENGINE_ID_YAHOO_CA,
  PREPOPULATED_ENGINE_ID_YAHOO_CH,
  PREPOPULATED_ENGINE_ID_YAHOO_CL,
  PREPOPULATED_ENGINE_ID_YAHOO_CO,
  PREPOPULATED_ENGINE_ID_YAHOO_DE,
  PREPOPULATED_ENGINE_ID_YAHOO_DK,
  PREPOPULATED_ENGINE_ID_YAHOO_ES,
  PREPOPULATED_ENGINE_ID_YAHOO_FI,
  PREPOPULATED_ENGINE_ID_YAHOO_FR,
  PREPOPULATED_ENGINE_ID_YAHOO_HK,
  PREPOPULATED_ENGINE_ID_YAHOO_ID,
  PREPOPULATED_ENGINE_ID_YAHOO_IE,
  PREPOPULATED_ENGINE_ID_YAHOO_IN,
  PREPOPULATED_ENGINE_ID_YAHOO_IT,
  PREPOPULATED_ENGINE_ID_YAHOO_MX,
  PREPOPULATED_ENGINE_ID_YAHOO_MY,
  PREPOPULATED_ENGINE_ID_YAHOO_NL,
  PREPOPULATED_ENGINE_ID_YAHOO_NO,
  PREPOPULATED_ENGINE_ID_YAHOO_NZ,
  PREPOPULATED_ENGINE_ID_YAHOO_PE,
  PREPOPULATED_ENGINE_ID_YAHOO_PH,
  PREPOPULATED_ENGINE_ID_YAHOO_SE,
  PREPOPULATED_ENGINE_ID_YAHOO_SG,
  PREPOPULATED_ENGINE_ID_YAHOO_TH,
  PREPOPULATED_ENGINE_ID_YAHOO_TW,
  PREPOPULATED_ENGINE_ID_YAHOO_UK,
  PREPOPULATED_ENGINE_ID_YAHOO_VE,
  PREPOPULATED_ENGINE_ID_YAHOO_VN,

  PREPOPULATED_ENGINE_ID_BRAVE,
  PREPOPULATED_ENGINE_ID_BRAVE_TOR,
};

extern const PrepopulatedEngine duckduckgo;
extern const PrepopulatedEngine duckduckgo_de;
extern const PrepopulatedEngine duckduckgo_au_nz_ie;
#if BUILDFLAG(IS_ANDROID)
extern const PrepopulatedEngine duckduckgo_lite;
#endif
extern const PrepopulatedEngine brave_ecosia;
extern const PrepopulatedEngine qwant;
extern const PrepopulatedEngine brave_startpage;
extern const PrepopulatedEngine brave_yandex;
extern const PrepopulatedEngine brave_search;
extern const PrepopulatedEngine brave_search_tor;
extern const PrepopulatedEngine brave_bing;
extern const PrepopulatedEngine brave_yahoo_jp;
extern const PrepopulatedEngine brave_google;

const std::map<BravePrepopulatedEngineID, const PrepopulatedEngine*>&
GetBraveEnginesMap();

}  // namespace TemplateURLPrepopulateData

#endif  // BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_
