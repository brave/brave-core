/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_
#define BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_

#include <cstddef>
#include <map>

#include "build/build_config.h"
#include "components/search_engines/prepopulated_engines.h"

namespace TemplateURLPrepopulateData {

extern const int kBraveCurrentDataVersion;
extern const int kBraveFirstTrackedDataVersion;

// See comments on prepopulated engines ids in
// components/search_engines/prepopulated_engines_schema.json above the
// definition of the id field and in
// components/search_engines/prepopulated_engines.json at the top of the file.
// Currently taken ids range under 90, but we'd want to leave room for
// additions by Chromium, so starting our ids from 500. Potential problem:
// Chromium adds one of these engines to their list with a different id.
enum BravePrepopulatedEngineID : unsigned int {
  PREPOPULATED_ENGINE_ID_INVALID = 0,

  // These engine IDs are already defined in prepopulated_engines.json
  PREPOPULATED_ENGINE_ID_GOOGLE = 1,
  PREPOPULATED_ENGINE_ID_BING = 3,
  PREPOPULATED_ENGINE_ID_YANDEX = 15,
  // These engine IDs are not defined in Chromium
  BRAVE_PREPOPULATED_ENGINES_START = 500,
  PREPOPULATED_ENGINE_ID_AMAZON = 500,     // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
#if defined(OS_ANDROID)
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE,  // No longer in defaults (7/2020).
#endif
  PREPOPULATED_ENGINE_ID_ECOSIA,           // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_FINDX,            // No longer exists (11/2018).
  PREPOPULATED_ENGINE_ID_GITHUB,           // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_INFOGALACTIC,     // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_MDNWEBDOCS,       // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_QWANT,
  PREPOPULATED_ENGINE_ID_SEARX,            // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_SEMANTICSCHOLAR,  // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_STACKOVERFLOW,    // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_STARTPAGE,
  PREPOPULATED_ENGINE_ID_TWITTER,          // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_WIKIPEDIA,        // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_WOLFRAMALPHA,     // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_YOUTUBE,          // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE,
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
};

extern const PrepopulatedEngine duckduckgo;
extern const PrepopulatedEngine duckduckgo_de;
extern const PrepopulatedEngine duckduckgo_au_nz_ie;
#if defined(OS_ANDROID)
extern const PrepopulatedEngine duckduckgo_lite;
#endif
extern const PrepopulatedEngine qwant;
extern const PrepopulatedEngine startpage;
extern const PrepopulatedEngine brave_yandex;

// Yahoo has unique URLs per region
extern const PrepopulatedEngine brave_yahoo;
extern const PrepopulatedEngine brave_yahoo_ar;
extern const PrepopulatedEngine brave_yahoo_at;
extern const PrepopulatedEngine brave_yahoo_au;
extern const PrepopulatedEngine brave_yahoo_br;
extern const PrepopulatedEngine brave_yahoo_ca;
extern const PrepopulatedEngine brave_yahoo_ch;
extern const PrepopulatedEngine brave_yahoo_cl;
extern const PrepopulatedEngine brave_yahoo_co;
extern const PrepopulatedEngine brave_yahoo_de;
extern const PrepopulatedEngine brave_yahoo_dk;
extern const PrepopulatedEngine brave_yahoo_es;
extern const PrepopulatedEngine brave_yahoo_fi;
extern const PrepopulatedEngine brave_yahoo_fr;
extern const PrepopulatedEngine brave_yahoo_hk;
extern const PrepopulatedEngine brave_yahoo_id;
extern const PrepopulatedEngine brave_yahoo_ie;
extern const PrepopulatedEngine brave_yahoo_in;
extern const PrepopulatedEngine brave_yahoo_it;
extern const PrepopulatedEngine brave_yahoo_mx;
extern const PrepopulatedEngine brave_yahoo_my;
extern const PrepopulatedEngine brave_yahoo_nl;
extern const PrepopulatedEngine brave_yahoo_no;
extern const PrepopulatedEngine brave_yahoo_nz;
extern const PrepopulatedEngine brave_yahoo_pe;
extern const PrepopulatedEngine brave_yahoo_ph;
extern const PrepopulatedEngine brave_yahoo_qc;
extern const PrepopulatedEngine brave_yahoo_se;
extern const PrepopulatedEngine brave_yahoo_sg;
extern const PrepopulatedEngine brave_yahoo_th;
extern const PrepopulatedEngine brave_yahoo_tw;
extern const PrepopulatedEngine brave_yahoo_uk;
extern const PrepopulatedEngine brave_yahoo_ve;
extern const PrepopulatedEngine brave_yahoo_vn;

const std::map<BravePrepopulatedEngineID, const PrepopulatedEngine*>&
GetBraveEnginesMap();

}  // namespace TemplateURLPrepopulateData

#endif  // BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_
