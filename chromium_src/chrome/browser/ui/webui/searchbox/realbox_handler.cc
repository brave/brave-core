// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/searchbox/realbox_handler.h"

#include "components/omnibox/browser/autocomplete_input.h"
#include "content/public/browser/web_contents.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"

// We tweak a few AutocompleteInput settings because unlike Chromium we only
// want keyword search results.
#define set_prefer_keyword(prefer)                    \
  set_keyword_mode_entry_method(                      \
      metrics::OmniboxEventProto::KEYBOARD_SHORTCUT); \
  autocomplete_input.set_prefer_keyword(true)
#define set_allow_exact_keyword_match(allow) set_allow_exact_keyword_match(true)

// Unfortunately, plumbing through the source doesn't seem trivial - it looks
// like it should be possible with the {source} part but it seems like it only
// works with Google Search. Additionally, the {source} param treats the NTP
// Realbox and the Omnibox the same, which isn't ideal, so we'd have to patch
// the behavior there too. This seems like the patch of least changes:
// 1. If this is a keyword search with Brave Search
// 2. Then replace &source=desktop with &source=new_tab
#define OpenURL(params, callback)                                             \
  GetURL();                                                                   \
  GURL url = destination_url;                                                 \
  auto* turl =                                                                \
      GetTemplateURLService()->GetTemplateURLForKeyword(match.keyword);       \
  if (turl && turl->prepopulate_id() ==                                       \
                  TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE) { \
    auto raw_gurl = url.spec();                                               \
    base::ReplaceFirstSubstringAfterOffset(&raw_gurl, 0, "&source=desktop",   \
                                           "&source=new_tab");                \
    url = GURL(raw_gurl);                                                     \
  }                                                                           \
  web_contents_->OpenURL(                                                     \
      content::OpenURLParams(url, content::Referrer(), disposition,           \
                             transition, false),                              \
      callback)

#include "src/chrome/browser/ui/webui/searchbox/realbox_handler.cc"

#undef set_prefer_keyword
#undef set_allow_exact_keyword_match
#undef OpenURL
