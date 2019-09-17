/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_
#define BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_

#include <cstddef>

#include "build/build_config.h"
#include "components/search_engines/prepopulated_engines.h"

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
  PREPOPULATED_ENGINE_ID_INVALID = 0,

  // These engine IDs are already defined in prepopulated_engines.json
  PREPOPULATED_ENGINE_ID_GOOGLE = 1,
  PREPOPULATED_ENGINE_ID_BING = 3,
  // These engine IDs are not defined in Chromium
  BRAVE_PREPOPULATED_ENGINES_START = 500,
  PREPOPULATED_ENGINE_ID_AMAZON = 500,     // No longer in defaults (2/2019).
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
#if defined(OS_ANDROID)
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE,
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
};

extern const PrepopulatedEngine duckduckgo;
#if defined(OS_ANDROID)
extern const PrepopulatedEngine duckduckgo_lite;
#endif
extern const PrepopulatedEngine qwant;
extern const PrepopulatedEngine startpage;

}  // namespace TemplateURLPrepopulateData

#endif  // BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_H_
