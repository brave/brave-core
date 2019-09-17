/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/search_engines/brave_prepopulated_engines.h"

#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/search_engine_type.h"

namespace TemplateURLPrepopulateData {

// IMPORTANT! Make sure to bump this value if you make changes to the
// engines below or add/remove engines.
const int kBraveCurrentDataVersion = 4;

const PrepopulatedEngine duckduckgo = {
  L"DuckDuckGo",
  L":d",
  "https://duckduckgo.com/favicon.ico",
  "https://duckduckgo.com/?q={searchTerms}&t=brave",
  "UTF-8",
  "https://ac.duckduckgo.com/ac/?q={searchTerms}&type=list",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  0,
  SEARCH_ENGINE_OTHER,
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
};

#if defined(OS_ANDROID)
const PrepopulatedEngine duckduckgo_lite = {
  L"DuckDuckGo Lite",
  L":dl",
  "https://duckduckgo.com/favicon.ico",
  "https://duckduckgo.com/lite/?q={searchTerms}&t=brave",
  "UTF-8",
  "https://ac.duckduckgo.com/ac/?q={searchTerms}&type=list",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  0,
  SEARCH_ENGINE_OTHER,
  PREPOPULATED_ENGINE_ID_DUCKDUCKGO_LITE,
};
#endif

const PrepopulatedEngine qwant = {
  L"Qwant",
  L":q",
  "https://www.qwant.com/favicon.ico",
  "https://www.qwant.com/?q={searchTerms}&client=brz-brave",
  "UTF-8",
  "https://api.qwant.com/api/suggest/?q={searchTerms}&client=opensearch",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  0,
  SEARCH_ENGINE_OTHER,
  PREPOPULATED_ENGINE_ID_QWANT,
};

const PrepopulatedEngine startpage = {
    L"StartPage",
    L":sp",
    "https://www.startpage.com/favicon.ico",
    "https://www.startpage.com/do/"
    "dsearch?query={searchTerms}&cat=web&pl=opensearch",
    "UTF-8",
    "https://www.startpage.com/cgi-bin/"
    "csuggest?query={searchTerms}&limit=10&format=json",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    SEARCH_ENGINE_OTHER,
    PREPOPULATED_ENGINE_ID_STARTPAGE,
};

}  // namespace TemplateURLPrepopulateData
