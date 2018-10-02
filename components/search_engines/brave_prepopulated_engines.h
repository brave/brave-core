/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_
#define BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_

#include <cstddef>

#include "components/search_engines/search_engine_type.h"

namespace TemplateURLPrepopulateData {

extern const int kBraveCurrentDataVersion;

// See comments on prepopulated engines ids in 
// components/search_engines/prepopulated_engines_schema.json above the
// definition of the id field and in
// components/search_engines/prepopulated_engines.json at the top of the file.
// Currently taken ids range under 90, but we'd want to leave room for
// additions by Chromium, so starting our ids from 500. Potential problem:
// Chromium adds one of these engines to their list with a different id.
enum BravePrepopulatedEngineID : unsigned int {
  // These engine IDs are already defined in prepopulated_engines.json
  PREPOPULATED_ENGINE_ID_GOOGLE = 1,
  PREPOPULATED_ENGINE_ID_YAHOO = 2,
  PREPOPULATED_ENGINE_ID_YAHOO_QC = 5,
  PREPOPULATED_ENGINE_ID_BING = 3,
  PREPOPULATED_ENGINE_ID_YANDEX = 15,
  // These engine IDs are not defined in Chromium
  PREPOPULATED_ENGINE_ID_AMAZON = 500,
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
  PREPOPULATED_ENGINE_ID_ECOSIA,
  PREPOPULATED_ENGINE_ID_FINDX,
  PREPOPULATED_ENGINE_ID_GITHUB,
  PREPOPULATED_ENGINE_ID_INFOGALACTIC,
  PREPOPULATED_ENGINE_ID_MDNWEBDOCS,
  PREPOPULATED_ENGINE_ID_QWANT,
  PREPOPULATED_ENGINE_ID_SEARX,
  PREPOPULATED_ENGINE_ID_SEMANTICSCHOLAR,
  PREPOPULATED_ENGINE_ID_STACKOVERFLOW,
  PREPOPULATED_ENGINE_ID_STARTPAGE,
  PREPOPULATED_ENGINE_ID_TWITTER,
  PREPOPULATED_ENGINE_ID_WIKIPEDIA,
  PREPOPULATED_ENGINE_ID_WOLFRAMALPHA,
  PREPOPULATED_ENGINE_ID_YOUTUBE,
};

extern const PrepopulatedEngine amazon;
extern const PrepopulatedEngine duckduckgo;
extern const PrepopulatedEngine ecosia;
extern const PrepopulatedEngine findx;
extern const PrepopulatedEngine github;
extern const PrepopulatedEngine infogalactic;
extern const PrepopulatedEngine mdnwebdocs;
extern const PrepopulatedEngine qwant;
extern const PrepopulatedEngine searx;
extern const PrepopulatedEngine semanticscholar;
extern const PrepopulatedEngine stackoverflow;
extern const PrepopulatedEngine startpage;
extern const PrepopulatedEngine twitter;
extern const PrepopulatedEngine wikipedia;
extern const PrepopulatedEngine wolframalpha;
// Yandex is already defined in chromium's prepopulated engines, but
// only as yandex_ua, _tr, _by, _ru, and _kz. Adding a generic one here.
extern const PrepopulatedEngine yandex;
extern const PrepopulatedEngine youtube;

}  // namespace TemplateURLPrepopulateData

#endif  // BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_
